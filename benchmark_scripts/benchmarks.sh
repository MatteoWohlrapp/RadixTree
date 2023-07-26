#!/bin/bash

# Define parameters
workloads=("a" "b" "c" "e" "x")
cache_options=("-c" "")
measure_per_operation_options=("--measure_per_operation" "")
operation_counts=(20000000 40000000 60000000 80000000 100000000)
coefficients=$(echo "scale=3; for(i=1; i<=10; i++) print i/1000" | bc)
memory_distribution=("1073741824 786432" "2147483648 524288" "3221225472 262144")
distributions=("uniform" "geometric")
record_counts=(4000000 8000000 12000000 16000000 20000000)

# Redirection file
output_file="results.txt"

# Clear the output file
>"$output_file"

# Start the tests
echo "Starting the tests..."

# Test 1: Vary records-size
echo "Vary records-size tests started..." >>"$output_file"
for cache in "${cache_options[@]}"; do
    for workload in "${workloads[@]}"; do
        for measure_operation in "${measure_per_operation_options[@]}"; do
            for record_count in "${record_counts[@]}"; do
                echo "Test 1: Running test with record_count=$record_count, distribution=geometric, workload=$workload, cache=$cache, measure_operation=$measure_operation" >>"$output_file"
                ../build/RadixTree -w$workload -d -b --record_count=$record_count --operation_count=10000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric $cache $measure_operation >>"$output_file"
                if [ $? -ne 0 ]; then
                    echo "Error occurred, stopping script."
                    exit 1
                fi
            done
        done
    done
done
echo "Vary records-size tests completed..." >>"$output_file"

# Test 2: Vary operations-size
echo "Vary operations-size tests started..." >>"$output_file"
for cache in "${cache_options[@]}"; do
    for workload in "${workloads[@]}"; do
        for measure_operation in "${measure_per_operation_options[@]}"; do
            for operation_count in "${operation_counts[@]}"; do
                echo "Test 2: Running test with operation_count=$operation_count, distribution=geometric, workload=$workload, cache=$cache, measure_operation=$measure_operation" >>"$output_file"
                ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=$operation_count --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric -b $cache $measure_operation >>"$output_file"
                if [ $? -ne 0 ]; then
                    echo "Error occurred, stopping script."
                    exit 1
                fi
            done
        done
    done
done
echo "Vary operations-size tests completed..." >>"$output_file"

# Test 3: Vary geometric distribution coefficient
echo "Vary geometric distribution coefficient tests started..." >>"$output_file"
for cache in "${cache_options[@]}"; do
    for workload in "${workloads[@]}"; do
        for measure_operation in "${measure_per_operation_options[@]}"; do
            echo "Test 3: Running test with varying coefficients, workload=$workload, cache=$cache, measure_operation=$measure_operation" >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.001 -b $cache $measure_operation >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.002 -b $cache $measure_operation >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.003 -b $cache $measure_operation >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.004 -b $cache $measure_operation >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.005 -b $cache $measure_operation >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.006 -b $cache $measure_operation >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.007 -b $cache $measure_operation >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.008 -b $cache $measure_operation >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.009 -b $cache $measure_operation >>"$output_file"
            ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=524288 --radix_tree_size=2147483648 --distribution=geometric --coefficient=0.01 -b $cache $measure_operation >>"$output_file"
            if [ $? -ne 0 ]; then
                echo "Error occurred, stopping script."
                exit 1
            fi
        done
    done
done
echo "Vary geometric distribution coefficient tests completed..." >>"$output_file"

# Test 4: Vary memory distribution coefficient
echo "Vary memory distribution coefficient tests started..." >>"$output_file"
for distribution in "${distributions[@]}"; do
    for memory in "${memory_distribution[@]}"; do
        for workload in "${workloads[@]}"; do
            for measure_operation in "${measure_per_operation_options[@]}"; do
                radix_tree_size=${memory% *}
                buffer_size=${memory#* }
                echo "Test 4: Running test with radix_tree_size=$radix_tree_size, buffer_size=$buffer_size, distribution=$distribution, workload=$workload, measure_operation=$measure_operation" >>"$output_file"
                ../build/RadixTree -w$workload -d --record_count=15000000 --operation_count=15000000 --buffer_size=$buffer_size --radix_tree_size=$radix_tree_size --distribution=$distribution -b -c $measure_operation >>"$output_file"
                if [ $? -ne 0 ]; then
                    echo "Error occurred, stopping script."
                    exit 1
                fi
            done
        done
    done
done
echo "Vary memory distribution coefficient tests completed..." >>"$output_file"

echo "All tests completed!" >>"$output_file"
