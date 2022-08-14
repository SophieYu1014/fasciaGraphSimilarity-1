#!/bin/bash
#SBATCH -J openmp-test
#SBATCH -e slurmcenter_%A_%a.err
#SBATCH -o slurmcenter_100000n_p0_00001_%A_%a.out
#SBATCH -c 16
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./fascia -X -m $SLURM_ARRAY_TASK_ID -n 100000 -p 0.00001 -s 1 -j 5 -k 5 -i 1000 -D
