/**
 * @file    run_config_two.h
 *
 * @author  Matteo Wohlrapp
 * @date    20.06.2023
 */

#pragma once

#include "run_config.h"

/**
 * @brief A class that can executes scenario
 */
class RunConfigTwo : public RunConfig
{
public:
    RunConfigTwo(bool cache = false) : RunConfig(cache) {}
    
    /**
     * @brief Execute a specific run with different operations on the database
     * @param benchmark If the run should be benchmarked or not
     */
    void execute(bool benchmark) override;
};