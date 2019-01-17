# sqpatch.cpp

## daint-mc

mpi | openmp |mpi*omp |time (sec) | iterations |
--- | --- | --- | --- | ---
1|1| 1|114.803|5
1|2| 2|58.2817|5
1|4| 4|31.9712|5
1|12| 12|12.5211|5
1|18| 18|8.46071|5
1|24| 24|6.91286|5
1|36| 36|4.68511|5

mpi*omp |speedup|// efficiency|time (sec) |
--- | --- | --- | --- |
| 1 | 1.00/1 | 100.0% | 114.8030 |
| 2 | 1.97/2 |  98.5% |  58.2817 |
| 4 | 3.59/4 |  89.8% |  31.9712 |
| 12 | 9.17/12 |  76.4% |  12.5211 |
| 18 | 13.57/18 |  75.4% |   8.4607 |
| 24 | 16.61/24 |  69.2% |   6.9129 |
| 36 | 24.50/36 |  68.1% |   4.6851 |


## compilers OpenMP version

The `_OPENMP` macro name is defined to have the decimal value yyyymm where yyyy
and mm are the year and month designations of the version of the OpenMP API
that the implementation supports.

* https://www.openmp.org/resources/openmp-compilers-tools/

compiler | OpenMP | flag |
--- | --- | --- |
| intel/18.0.2   | 201611/pre5.0 | -qopenmp |
| intel/19.0.1   | 201611/pre5.0 | -qopenmp |
| cce/8.7.6      | 201511/4.5    | -homp |
| gcc/6.2.0      | 201511/4.5    | -fopenmp | 
| gcc/7.3.0      | 201511/4.5    | -fopenmp |
| GCC/8.2.0-2.30 | 201511/4.5    | -fopenmp |
| pgi/18.5       | 201307/4.0    | -mp |
| pgi/18.10      | 201307/4.0    | -mp |
| clang/7.0      | 201107/3.1    | -fopenmp |
| --- | --- | --- |
