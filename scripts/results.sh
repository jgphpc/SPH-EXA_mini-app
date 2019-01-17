#!/bin/bash

in=$1


nmpi=`echo $in |cut -d. -f2`
nomp=`echo $in |cut -d. -f4`
ntasks=`expr $nmpi \* $nomp`

slurm_nmpi=`grep "SLURM_NTASKS =" $in |awk '{print $4}'`
slurm_mpipercn=`grep "SLURM_NTASKS_PER_NODE =" $in |awk '{print $4}'`
slurm_nomp=`grep "SLURM_CPUS_PER_TASK =" $in |awk '{print $4}'`
slurm_nomp2=`grep "OMP_NUM_THREADS =" $in |awk '{print $4}'`
slurm_nht=`grep "SLURM_NTASKS_PER_CORE =" $in |awk '{print $4}'`
mpichv=`grep "CRAY MPICH version" $in |awk '{print $7}'`

avgt_per_iteration=`grep "Total time for iteration" $in |awk '{s=s+$6}END{print s/NR}'`
iterations=`grep Iteration: $in |wc -l`

echo "$nmpi|$nomp| $ntasks|$avgt_per_iteration|$iterations"

#echo "$nmpi|$nomp| $ntasks\
#|$slurm_nmpi|$slurm_mpipercn\
#|$slurm_nomp|$slurm_nomp2\
#|$slurm_nht\
#|$mpichv|$iterations|$avgt_per_iteration\
#|$seconds"
