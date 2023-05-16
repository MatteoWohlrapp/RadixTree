#pragma once

#include <ctime>
#include <iostream>
#include <list>
#include <chrono>
#include "spdlog/spdlog.h"

class Benchmark
{
private:
    std::shared_ptr<spdlog::logger> logger;

public:
    Benchmark()
    {
        logger = spdlog::get("logger");
    }

    template <typename Run>
    void measure(Run measure, bool benchmark)
    {
        if (benchmark)
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
        else
        {
            measure();
        }
    }
};