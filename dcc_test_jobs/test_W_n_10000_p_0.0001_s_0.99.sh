#!/bin/bash
#SBATCH -J openmp-test
#SBATCH -e slurmcenter_%A_%a.err
#SBATCH -o slurmcenter_10000n_p0_0001_0_99_%A_%a.out
#SBATCH -c 16
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./fascia -W -m $SLURM_ARRAY_TASK_ID -n 10000 -p 0.0001 -s 0.99 -j 6 -k 6 -i 1000 -D
