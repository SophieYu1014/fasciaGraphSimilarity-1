rVals=(0.9 0.95 0.99 1)
qVals=(0.001 0.01 0.1 0.5)

for q in ${qVals[@]}; do
    for r in ${rVals[@]}; do
        ./generateGraphs -n 1000 -m 20 -q $q -r $r;
        sleep 3;
    done
done
