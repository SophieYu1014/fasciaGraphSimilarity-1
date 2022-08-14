#!/bin/bash
#SBATCH -J openmp-test
#SBATCH -e slurmcenter_%A_%a.err
#SBATCH -o slurmcenter_1000n_p0_001_s_0_9_%A_%a.out
#SBATCH -c 16
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./fascia -W -m $SLURM_ARRAY_TASK_ID -n 1000 -p 0.001 -s 0.9 -j 6 -k 6 -i 1000 -D
