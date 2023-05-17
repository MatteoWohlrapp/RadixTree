/**
 * @file    Benchmark.h
 * 
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
*/


#pragma once

#include <ctime>
#include <iostream>
#include <list>
#include <chrono>
#include "spdlog/spdlog.h"

/**
 * @brief A class the abstracts the benchmarking
*/
class Benchmark
{
private:
    std::shared_ptr<spdlog::logger> logger;

public:
    /**
     * @brief Constructor
    */
    Benchmark()
    {
        logger = spdlog::get("logger");
    }

    /**
     * @brief Measures the speed of function call and prints the execution time
     * @param func The function that is measured
     * @param benchmark Specifying if the funtion execution should be benchmarked or not
    */
    template <typename Run>
    void measure(Run func, bool benchmark)
    {
        if (benchmark)
        {
            std::chrono::time_point<std::chrono::high_resolution_clock> start_point, end_point;
            auto total_time = std::chrono::microseconds(0).count();

            start_point = std::chrono::high_resolution_clock::now();
            func();
            end_point = std::chrono::high_resolution_clock::now();

            auto start = std::chrono::time_point_cast<std::chrono::microseconds>(start_point).time_since_epoch().count();
            auto end = std::chrono::time_point_cast<std::chrono::microseconds>(end_point).time_since_epoch().count();

            total_time += (end - start);

            std::cout << "Benchmark results - Runtime: " << total_time << std::endl;
        }
        else
        {
            func();
        }
    }
};