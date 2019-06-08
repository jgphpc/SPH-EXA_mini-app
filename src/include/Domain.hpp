#pragma once

#include <vector>

#ifdef USE_MPI
	#include "mpi.h"
#endif

#include "config.hpp"
#include "Octree.hpp"

namespace sphexa
{

template<typename T, class Tree = Octree<T>>
class Domain
{
public:
	Domain(int ngmin, int ng0, int ngmax, unsigned int bucketSize = 128) : 
		ngmin(ngmin), ng0(ng0), ngmax(ngmax), bucketSize(bucketSize) {}

    template<class Dataset>
	void findNeighbors(const std::vector<int> &clist, Dataset &d)
	{
		int n = clist.size();
		d.neighbors.resize(n*ngmax);
		d.neighborsCount.resize(n);

                #ifdef SPEC_OPENMP
		#pragma omp parallel for schedule(guided)
                #endif
		for(int pi=0; pi<n; pi++)
		{
			int i = clist[pi];

            d.neighborsCount[pi] = 0;
            tree.findNeighbors(i, d.x[i], d.y[i], d.z[i], 2*d.h[i], ngmax, &d.neighbors[pi*ngmax], d.neighborsCount[pi], d.bbox.PBCx, d.bbox.PBCy, d.bbox.PBCz);
            
            #ifndef NDEBUG
	            if(d.neighbors[pi].size() == 0)
	            	printf("ERROR::FindNeighbors(%d) x %f y %f z %f h = %f ngi %zu\n", i, x[i], y[i], z[i], h[i], neighborsCount[pi]);
	        #endif
	    }
	}

	template<class Dataset>
	long long int neighborsSum(const std::vector<int> &clist, const Dataset &d)
	{
	    long long int sum = 0;
            #ifdef SPEC_OPENMP
	    #pragma omp parallel for reduction (+:sum)
            #endif
	    for(unsigned int i=0; i<clist.size(); i++)
	        sum += d.neighborsCount[i];

	    #ifdef USE_MPI
	        MPI_Allreduce(MPI_IN_PLACE, &sum, 1, MPI_LONG_LONG_INT, MPI_SUM, MPI_COMM_WORLD);
	    #endif

	    return sum;
	}

	template<class Dataset>
    inline void buildTree(const Dataset &d)
    {
    	tree.build(d.bbox, d.x, d.y, d.z, d.h, bucketSize);
    }

private:
	Tree tree;
	const int ngmin, ng0, ngmax;
	const unsigned int bucketSize;
};

/*    void reorderSwap(const std::vector<int> &ordering, Array<T> &data)
    {
        std::vector<T> tmp(ordering.size());
        for(unsigned int i=0; i<ordering.size(); i++)
            tmp[i] = data[ordering[i]];
        tmp.swap(data);
    }

	void reorder(std::vector<Array<T>*> &data)
    {
        for(unsigned int i=0; i<data.size(); i++)
            reorderSwap(*tree.ordering, *data[i]);
    }*/

/*	inline T update_smoothing_length(const int ng0, const int ngi, const T hi)
	{
	    const T c0 = 7.0;
	    const T exp = 1.0/3.0;

	    T ka = pow((1.0 + c0 * ng0 / ngi), exp);

	    return hi * 0.5 * ka;
	}
*/
/*	void updateSmoothingLength(const std::vector<int> &clist, const std::vector<std::vector<int>> &neighbors, Array<T> &h)
	{
		const T c0 = 7.0;
		const T exp = 1.0/3.0;

		int n = clist.size();

                #ifdef SPEC_OPENMP
		#pragma omp parallel for
                #endif
		for(int pi=0; pi<n; pi++)
		{
			int i = clist[pi];
			int ngi = std::max((int)neighbors[pi].size(),1);
			
		    h[i] = h[i] * 0.5 * pow((1.0 + c0 * ng0 / ngi), exp);

		    #ifndef NDEBUG
		        if(std::isinf(h[i]) || std::isnan(h[i]))
		        	printf("ERROR::h(%d) ngi %d h %f\n", i, ngi, h[i]);
		    #endif
	    }
	}*/


}

