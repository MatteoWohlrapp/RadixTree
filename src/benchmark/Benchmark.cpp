#include "Benchmark.h"

#include <ctime>
#include <iostream>
#include <list>
#include <chrono>

template <typename Func>
void Benchmark::measure(Func func)
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start_point, end_point;
    auto total_time = std::chrono::microseconds(0).count();

    start_point = std::chrono::high_resolution_clock::now();
    func();
    end_point = std::chrono::high_resolution_clock::now();

    auto start = std::chrono::time_point_cast<std::chrono::microseconds>(start_point).time_since_epoch().count();
    auto end = std::chrono::time_point_cast<std::chrono::microseconds>(end_point).time_since_epoch().count();

    total_time += (end - start);

    std::cout << "Benchmark results - Average Runtime: " << total_time << std::endl;
}
