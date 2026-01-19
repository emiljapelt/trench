#!/bin/bash

# Prepare log file
> trench.log

./trench ./tests/test.trg

if [ $? != 0 ]; then 
    echo "Failed to run!"
    exit
fi

failure_points=$(cat ./trench.log | grep "tester#0:" | awk -F: '{ print $3 }')
failure_count=$(cat ./trench.log | grep -c "tester#0:")

if [ $failure_count -gt 0 ]; then
    echo "$failure_count test(s) failed!"
    echo "Failure points:"
    echo "$failure_points"
else
    echo "No failures"
fi