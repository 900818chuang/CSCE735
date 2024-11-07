#!/bin/bash





k=28



for q in 0 1 2 4 6 8 10; do


    echo "Running with q = $q"

    ./sort_list_new.exe $k $q

done
