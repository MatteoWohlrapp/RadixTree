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
public:
    Benchmark benchmark;
    DataManager data_manager;
    std::shared_ptr<spdlog::logger> logger;

    /**
     * @brief Constructor
    */
    RunConfig(bool cache) : benchmark(), data_manager(cache)
    {
        logger = spdlog::get("logger");
    }

    /**
     * @brief Destructor
    */
    virtual ~RunConfig(){
        data_manager.destroy(); 
    }

    /**
     * @brief Execute a specific run with different operations on the database
     * @param benchmark If the run should be benchmarked or not
    */
    virtual void execute(bool benchmark) = 0;
};