#!/bin/bash
#SBATCH -J openmp-test
#SBATCH -e slurmcenter_%A_%a.err
#SBATCH -o slurmcenter_1000n_p0_1_s_0.95_%A_%a_K_5.out
#SBATCH -c 16
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./fascia -W -m $SLURM_ARRAY_TASK_ID -n 1000 -p 0.1 -s 0.95 -j 5 -k 5 -i 1000 -D
