## Graph Generation

to generate graph independently, run the following function:

    ./generateGraphs -n $NODE_COUNT -m $GRAPH_COUNT -r $RELATEDNESS -q $DENSITY -i $GENERATE_INDEPENDENT_GRAPH

The graphs will then be generated in `independent_graphs/` and `correlated_graphs` in the following format:

    3_100B_rho1.00000_q0.01000_corr.txt

if -i is specified, then ONLY independent graphs will be generated, -r in this case would not be needed.

    2_100A_q0.01000_ind.txt 

The main software has been updated to run tree counting and matching on these file, run as normal:

    ./fascia -W -m 3 -n 1000 -p 1 -s 0.01 -j 5 -k 5 -i 200 -D;

This need to be correlated with file previously generated.
