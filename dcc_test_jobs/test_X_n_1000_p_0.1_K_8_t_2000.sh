#!/bin/bash
#SBATCH -J openmp-test
#SBATCH -e slurmcenter_%A_%a.err
#SBATCH -o slurmcenter_1000n_p0_1_%A_%a_K_8.out
#SBATCH -c 16
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./fascia -X -m $SLURM_ARRAY_TASK_ID -n 1000 -p 0.1 -s 1 -j 8 -k 8 -i 2000 -D
