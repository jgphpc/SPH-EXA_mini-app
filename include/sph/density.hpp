#pragma once

#include <vector>

#include "../ParticlesData.hpp"
#include "kernels.hpp"
#include "Task.hpp"
#include "lookupTables.hpp"
#include "cuda/sph.cuh"

#if defined(USE_CUDA) || defined(USE_ACC) || defined(USE_OMP_TARGET)
#error "The code was refactored to support General Volume Elements, but accelerator code has not been addressed yet."
#endif


namespace sphexa
{
namespace sph
{

template <typename T, class Dataset>
void computeDensityImpl(const Task &t, Dataset &d)
{
    const size_t n = t.clist.size();
    const size_t ngmax = t.ngmax;
    const int *clist = t.clist.data();
    const int *neighbors = t.neighbors.data();
    const int *neighborsCount = t.neighborsCount.data();

    const T *h = d.h.data();
    const T *m = d.m.data();
    const T *x = d.x.data();
    const T *y = d.y.data();
    const T *z = d.z.data();

    T *ro = d.ro.data();
    // general VE
    const T *xmass = d.xmass.data(); // the VE estimator function vals. Only updated at end of iteration
    T *sumkx = d.sumkx.data(); // kernel-weighted sum of VE estimators
    T *sumwh = d.sumwh.data(); // sum of VE estimators weighted by derivative of kernel wrt. h
    T *vol = d.vol.data();

    const BBox<T> bbox = d.bbox;

    const T K = d.K;
    const T sincIndex = d.sincIndex;

#if defined(USE_OMP_TARGET)
    // Apparently Cray with -O2 has a bug when calling target regions in a loop. (and computeDensityImpl can be called in a loop).
    // A workaround is to call some method or allocate memory to either prevent buggy optimization or other side effect.
    // with -O1 there is no problem
    // Tested with Cray 8.7.3 with NVIDIA Tesla P100 on PizDaint
    std::vector<T> imHereBecauseOfCrayCompilerO2Bug(4, 10);

    const size_t np = d.x.size();
    const size_t allNeighbors = n * ngmax;
// clang-format off
#pragma omp target map(to                                                                                                                  \
                       : clist [0:n], neighbors [:allNeighbors], neighborsCount [:n], m [0:np], h [0:np], x [0:np], y [0:np], z [0:np])    \
                   map(from                                                                                                                \
                       : ro [:n])
#pragma omp teams distribute parallel for
// clang-format on
#elif defined(USE_ACC)
    const size_t np = d.x.size();
    const size_t allNeighbors = n * ngmax;
#pragma acc parallel loop copyin(n, clist [0:n], neighbors [0:allNeighbors], neighborsCount [0:n], m [0:np], h [0:np], x [0:np], y [0:np], \
                                 z [0:np]) copyout(ro[:n])
#else
#pragma omp parallel for
#endif
    for (size_t pi = 0; pi < n; pi++)
    {
        const int i = clist[pi];
        const int nn = neighborsCount[pi];

        T roloc = 0.0;
        // general VE
        T sumkx_loc = 0.0;
        T sumwh_loc = 0.0;

        // int converstion to avoid a bug that prevents vectorization with some compilers
        for (int pj = 0; pj < nn; pj++)
        {
            const int j = neighbors[pi * ngmax + pj];

            // later can be stores into an array per particle
            T dist = distancePBC(bbox, h[i], x[i], y[i], z[i], x[j], y[j], z[j]); // store the distance from each neighbor

            // calculate the v as ratio between the distance and the smoothing length
            T vloc = dist / h[i];

#ifndef NDEBUG
            if (vloc > 2.0 * 5.0 || vloc < 0.0)
                printf("ERROR:Density or Distance (%d,%d) vloc %f -- x %f %f %f -- %f %f %f -- dist %f -- hi %f\n", int(d.id[i]), int(d.id[j]), vloc, x[i], y[i], z[i], x[j],
                       y[j], z[j], dist, h[i]);
#endif

            const T w = K * math_namespace::pow(wharmonic(vloc), (int)sincIndex);
            const T value = w / (h[i] * h[i] * h[i]);
            roloc += value * m[j];
            // general VE
            sumkx_loc += value * xmass[j];
            // summation part of derivative of density wrt. h[i] (needed for NR)
            sumwh_loc += - xmass[j] * value / h[i] * (3.0 + vloc * (int)sincIndex * wharmonic_derivative(vloc)); // updated 25.2.20
            // checked 2.4.2020: identical to sphynx with sphynx kernel = 2...
        }

        // general VE
        sumkx_loc += xmass[i] * K / (h[i] * h[i] * h[i]);  // self contribution. no need for kernel (dist = 0 -> 1)
        sumwh_loc += - xmass[i] * K / (h[i] * h[i] * h[i] * h[i]) * 3.0; // self-contribution

        vol[i] = xmass[i] / sumkx_loc;  // calculate volume element
        // new density
        ro[i] = m[i] / vol[i];
//        double diff = roloc + m[i] * K / (h[i] * h[i] * h[i]) - ro[i];
//        if (abs(diff) > 1.11e-16) printf("Roloc[%d] - ro[%d] = %.5e\n", i, i, diff);
        sumkx[i] = sumkx_loc;
        sumwh[i] = m[i] / xmass[i] * sumwh_loc;

#ifndef NDEBUG
        if (std::isnan(ro[i]) || std::isinf(ro[i])) printf("ERROR::Density(%d) density %f, position: (%f %f %f), h: %f\n", int(d.id[i]), ro[i], x[i], y[i], z[i], h[i]);
#endif
    }
}

template <typename T, class Dataset>
void computeDensity(const std::vector<Task> &taskList, Dataset &d)
{
#if defined(USE_CUDA)
# error "General Volume elements have not been implemented in CUDA yet (sumkx, sumwh, xmass, ...)"
    cuda::computeDensity<T>(taskList, d); // utils::partition(l, d.noOfGpuLoopSplits), d);
#else
    for (const auto &task : taskList)
    {
        computeDensityImpl<T>(task, d);
    }
#endif
}

template <typename T, class Dataset>
void initFluidDensityAtRestImpl(const Task &t, Dataset &d)
{
    const size_t n = t.clist.size();
    const int *clist = t.clist.data();

    const T *ro = d.ro.data();
    T *ro_0 = d.ro_0.data();

#pragma omp parallel for
    for (size_t pi = 0; pi < n; ++pi)
    {
        const int i = clist[pi];
        ro_0[i] = ro[i];
    }
}

template <typename T, class Dataset>
void initFluidDensityAtRest(const std::vector<Task> &taskList, Dataset &d)
{
    for (const auto &task : taskList)
    {
        initFluidDensityAtRestImpl<T>(task, d);
    }
}

} // namespace sph
} // namespace sphexa
