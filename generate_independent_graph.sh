qVals=(0.001 0.01 0.1 0.5)

for q in ${qVals[@]}; do
    ./generateGraphs -n 1000 -m 20 -q $q -i;
    sleep 3
done
