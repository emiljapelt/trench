#!/bin/bash

# Prepare log file
> trench.log

./trench ./tests/test.trg

total_count=$(cat ./trench.log | grep -c "tester#0:")
successes_count=$(cat ./trench.log | grep -c "tester#0: 1")
failures_count=$(cat ./trench.log | grep -c "tester#0: 0")

echo "Test result: $successes_count / $total_count tests succeded"

if [ $failures_count -gt 0 ]; then
    echo "Failing assertions:"
    cat ./trench.log | grep "tester#0:" | nl | grep "tester#0: 0" | cut -c1-6
fi