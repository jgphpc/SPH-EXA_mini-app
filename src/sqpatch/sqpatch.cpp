#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "sphexa.hpp"
#include "SqPatchDataGenerator.hpp"
#include "SqPatchFileWriter.hpp"

#include <fenv.h>
#pragma STDC FENV_ACCESS ON


using namespace sphexa;

void printHelp(char *binName, int rank);

int main(int argc, char **argv)
{
    std::feclearexcept(FE_ALL_EXCEPT);

    const int rank = initAndGetRankId();

    const ArgParser parser(argc, argv);

    if (parser.exists("-h") || parser.exists("--h") || parser.exists("-help") || parser.exists("--help"))
    {
        printHelp(argv[0], rank);
        return exitSuccess();
    }

    const size_t cubeSide = parser.getInt("-n", 50);
    const size_t maxStep = parser.getInt("-s", 10);
    const int writeFrequency = parser.getInt("-w", -1);
    const bool quiet = parser.exists("--quiet");
    const std::string outDirectory = parser.getString("--outDir");
    const bool oldAV = parser.exists("--oldAV");
    const bool gVE = parser.exists("--gVE");
    const size_t hNRStart = parser.getInt("--hNRStart", maxStep);
    const size_t hNgBMStop = parser.getInt("--hNgBMStop", maxStep);
    const size_t ngmax_cli = std::max(parser.getInt("--ngmax", 750), 0);
    const size_t ngmin_cli = std::max(parser.getInt("--ngmin", 50), 0);
    const size_t ng0 = parser.getInt("--ng0", 250);
    const size_t hackyNgMinMaxFixTries = parser.getInt("--hackyNgMinMaxFixTries", 5);

    std::ofstream nullOutput("/dev/null");
    std::ostream &output = quiet ? nullOutput : std::cout;

    using Real = double;
    using Dataset = ParticlesData<Real>;
    using Tree = Octree<Real>;

#ifdef USE_MPI
    DistributedDomain<Real, Dataset, Tree> domain;
    const IFileWriter<Dataset> &fileWriter = SqPatchMPIFileWriter<Dataset>();
#else
    Domain<Real, Dataset, Tree> domain;
    const IFileWriter<Dataset> &fileWriter = SqPatchFileWriter<Dataset>();
#endif

    auto d = SqPatchDataGenerator<Real>::generate(cubeSide);
    d.oldAV = oldAV;
    const Printer<Dataset> printer(d);

    MasterProcessTimer timer(output, d.rank), totalTimer(output, d.rank);

    std::ofstream constantsFile(outDirectory + "constants.txt");

    Tree::bucketSize = 64;
    Tree::minGlobalBucketSize = 512;
    Tree::maxGlobalBucketSize = 2048;
    domain.create(d);

    const size_t nTasks = 1;
//    const size_t ngmax = 750; // increased to fight bug
    const size_t ngmax = std::max(ng0 + 50, ngmax_cli);
    TaskList taskList = TaskList(domain.clist, nTasks, ngmax, ng0);

    // want to dump on floating point exceptions
    bool fpe_raised = false;
    totalTimer.start();
    for (d.iteration = 0; d.iteration <= maxStep; d.iteration++)
    {
        timer.start();

        domain.update(d);
        timer.step("domain::distribute");
        domain.synchronizeHalos(&d.x, &d.y, &d.z, &d.h, &d.xmass);
        timer.step("mpi::synchronizeHalos");
        domain.buildTree(d);
        timer.step("domain::buildTree");
        taskList.update(domain.clist);
        timer.step("updateTasks");
        sph::findNeighbors(domain.octree, taskList.tasks, d);
        timer.step("FindNeighbors");

        size_t maxNeighbors, minNeighbors;
        std::tie(minNeighbors, maxNeighbors) = sph::neighborsStats<Real>(taskList.tasks, d); // AllReduce        const int maxRetries = 10;
        size_t tries = 0;
        while (tries < hackyNgMinMaxFixTries && (maxNeighbors > ngmax || minNeighbors < ngmin_cli)) {
            tries++;
            if (d.rank == 0) output << "-- minNeighbors " << minNeighbors << " maxNeighbors " << maxNeighbors << " try: " << tries <<  std::endl;
            sph::updateSmoothingLengthForExceeding<Real>(taskList.tasks, d, ngmin_cli);
            timer.step("HackyUpdateSmoothingLengthForThoseWithTooManyOrTooFewNeighbors");
            domain.update(d);
            timer.step("domain::distribute");
            domain.synchronizeHalos(&d.x, &d.y, &d.z, &d.h, &d.xmass);
            timer.step("mpi::synchronizeHalos");
            domain.buildTree(d);
            timer.step("domain::buildTree");
            taskList.update(domain.clist);
            timer.step("updateTasks");
            sph::findNeighbors(domain.octree, taskList.tasks, d);
            timer.step("FindNeighbors");
            std::tie(minNeighbors, maxNeighbors) = sph::neighborsStats<Real>(taskList.tasks, d); // AllReduce        const int maxRetries = 10;
        }

        sph::computeDensity<Real>(taskList.tasks, d);
        timer.step("Density");
        if (d.iteration == 0) { sph::initFluidDensityAtRest<Real>(taskList.tasks, d); }
        if (d.iteration > hNRStart) {
            sph::newtonRaphson<Real>(taskList.tasks, d);
            timer.step("hNR");
            for (int iterNR = 0; iterNR < 2; iterNR++) {
                sph::computeDensity<Real>(taskList.tasks, d);
                timer.step("Density");
                sph::newtonRaphson<Real>(taskList.tasks, d);
                timer.step("hNR");
            }
        }
        sph::calcGradhTerms<Real>(taskList.tasks, d);
        timer.step("calcGradhTerms");
        sph::computeEquationOfStateSphynxWater<Real>(taskList.tasks, d);
        timer.step("EquationOfState");
        domain.synchronizeHalos(&d.vx, &d.vy, &d.vz, &d.ro, &d.p, &d.c, &d.sumkx, &d.gradh, &d.h, &d.vol);
        timer.step("mpi::synchronizeHalos");
        sph::computeIAD<Real>(taskList.tasks, d);
        timer.step("IAD");
        domain.synchronizeHalos(&d.c11, &d.c12, &d.c13, &d.c22, &d.c23, &d.c33);
        timer.step("mpi::synchronizeHalos");
        sph::computeMomentumAndEnergyIAD<Real>(taskList.tasks, d);
        timer.step("MomentumEnergyIAD");
        sph::computeTimestep<Real, sph::TimestepPress2ndOrder<Real, Dataset>>(taskList.tasks, d);
        timer.step("Timestep"); // AllReduce(min:dt)
        sph::computePositions<Real, sph::computeAcceleration<Real, Dataset>>(taskList.tasks, d);
        timer.step("UpdateQuantities");
        sph::computeTotalEnergy<Real>(taskList.tasks, d);
        timer.step("EnergyConservation"); // AllReduce(sum:ecin,ein)
        if (d.iteration < hNgBMStop) {
            sph::updateSmoothingLength<Real>(taskList.tasks, d);
            timer.step("UpdateSmoothingLength");
        }

        if (gVE && d.iteration > hNRStart - 5)
            sph::updateVEEstimator<Real, sph::XmassSPHYNXVE<Real, Dataset>>(taskList.tasks, d);
        else
            sph::updateVEEstimator<Real, sph::XmassStdVE<Real, Dataset>>(taskList.tasks, d);
        timer.step("UpdateVEEstimator");

        const size_t totalNeighbors = sph::neighborsSum(taskList.tasks);
        if (d.rank == 0)
        {
            printer.printCheck(d.count, domain.octree.globalNodeCount, d.x.size() - d.count, totalNeighbors,
                    minNeighbors, maxNeighbors, ngmax, output);
            printer.printConstants(d.iteration, totalNeighbors, minNeighbors, maxNeighbors, ngmax, constantsFile);
        }

        fpe_raised = all_check_FPE("after print, rank " + std::to_string(d.rank));
        if (fpe_raised) break;

        if ((writeFrequency > 0 && d.iteration % writeFrequency == 0) || writeFrequency == 0)
        {
            fileWriter.dumpParticleDataToAsciiFile(d, domain.clist, outDirectory + "dump_sqpatch" + std::to_string(d.iteration) + ".txt");
            // fileWriter.dumpParticleDataToBinFile(d, outDirectory + "dump_sqpatch" + std::to_string(d.iteration) + ".bin");
            timer.step("writeFile");
        }

        timer.stop();

        if (d.rank == 0) printer.printTotalIterationTime(timer.duration(), output);
    }

    if (fpe_raised) {
        fileWriter.dumpParticleDataToAsciiFile(d, domain.clist, outDirectory + "fperrordump_sqpatch" + std::to_string(d.iteration) + "_" + std::to_string(std::time(0)) + ".txt");
    }

    totalTimer.step("Total execution time for " + std::to_string(d.iteration) + " iterations of SqPatch");

    constantsFile.close();

    return exitSuccess();
}

void printHelp(char *name, int rank)
{
    if (rank == 0)
    {
        printf("\nUsage:\n\n");
        printf("%s [OPTIONS]\n", name);
        printf("\nWhere possible options are:\n");
        printf("\t-n NUM \t\t\t NUM^3 Number of particles\n");
        printf("\t-s NUM \t\t\t NUM Number of iterations (time-steps)\n");
        printf("\t-w NUM \t\t\t Dump particles data every NUM iterations (time-steps)\n\n");

        printf("\t--quiet \t\t Don't print anything to stdout\n\n");

        printf("\t--outDir PATH \t\t Path to directory where output will be saved.\
                    \n\t\t\t\t Note that directory must exist and be provided with ending slash.\
                    \n\t\t\t\t Example: --outDir /home/user/folderToSaveOutputFiles/\n");
    }
}
