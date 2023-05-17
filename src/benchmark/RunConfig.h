/**
 * @file    RunConfig.h
 * 
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
*/

#pragma once

#include "Benchmark.h"
#include "../data/DBManager.h"
#include "spdlog/spdlog.h"

/**
 * @brief A class that can execute a specific scenario for the database
*/
class RunConfig
{
public:
    Benchmark benchmark;
    DBManager db_manager;
    std::shared_ptr<spdlog::logger> logger;

    /**
     * @brief Destructor
    */
    virtual ~RunConfig() {}

    /**
     * @brief Constructor
    */
    RunConfig() : benchmark(), db_manager()
    {
        logger = spdlog::get("logger");
    }

    /**
     * @brief Execute a specific run with different operations on the database
     * @param benchmark If the run should be benchmarked or not
    */
    virtual void execute(bool benchmark) = 0;
};