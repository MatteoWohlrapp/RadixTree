#pragma once

#include <ctime>
#include <iostream>
#include <list>
#include <chrono>

class Benchmark
{
public:
    template <typename Run>
    void measure(Run measure, bool benchmark)
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> start_point, end_point;
        auto total_time = std::chrono::microseconds(0).count();

        start_point = std::chrono::high_resolution_clock::now();
        measure();
        end_point = std::chrono::high_resolution_clock::now();

        auto start = std::chrono::time_point_cast<std::chrono::microseconds>(start_point).time_since_epoch().count();
        auto end = std::chrono::time_point_cast<std::chrono::microseconds>(end_point).time_since_epoch().count();

        total_time += (end - start);

        std::cout << "Benchmark results - Runtime: " << total_time << std::endl;
    }
};