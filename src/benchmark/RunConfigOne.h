/**
 * @file    RunConfigOne.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "RunConfig.h"

/**
 * @brief A class that can executes scenario
 */
class RunConfigOne : public RunConfig
{
public:
    RunConfigOne(bool cache = false) : RunConfig(cache) {}
    
    /**
     * @brief Execute a specific run with different operations on the database
     * @param benchmark If the run should be benchmarked or not
     */
    void execute(bool benchmark) override;
};