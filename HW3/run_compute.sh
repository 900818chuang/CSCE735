#!/bin/bash





k=12



for q in 0 1 2 4 6 8 10; do


    echo "Running with q = $q"

    ./sort_list_openmp.exe $k $q

done
