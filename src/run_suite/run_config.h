/**
 * @file    run_config.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "benchmark.h"
#include "../data/data_manager.h"
#include "spdlog/spdlog.h"

/**
 * @brief A class that can execute a specific scenario for the database
 */
class RunConfig
{
protected:
    Benchmark benchmark;
    DataManager<Configuration::page_size> data_manager;
    std::shared_ptr<spdlog::logger> logger;

    int buffer_size; 
    int radix_tree_size; 
    bool cache; 

public:
    /**
     * @brief Constructor
     */
    RunConfig(int buffer_size_arg, bool cache_arg, int radix_tree_size_arg) : benchmark(), data_manager(buffer_size_arg, cache_arg, radix_tree_size_arg), buffer_size(buffer_size_arg), radix_tree_size(radix_tree_size_arg), cache(cache_arg)
    {
        logger = spdlog::get("logger");
    }

    /**
     * @brief Destructor
     */
    virtual ~RunConfig()
    {
        data_manager.destroy();
    }

    /**
     * @brief Execute a specific run with different operations on the database
     * @param benchmark If the run should be benchmarked or not
     */
    virtual void execute(bool benchmark) = 0;
};