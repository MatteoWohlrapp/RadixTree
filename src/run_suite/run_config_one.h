/**
 * @file    run_config_one.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "run_config.h"

/**
 * @brief A class that can executes scenario
 */
class RunConfigOne : public RunConfig
{
public:
    RunConfigOne(int buffer_size_arg, bool cache_arg, int radix_tree_size_arg) : RunConfig(buffer_size_arg, cache_arg, radix_tree_size_arg) {}

    /**
     * @brief Execute a specific run with different operations on the database
     * @param benchmark If the run should be benchmarked or not
     */
    void execute(bool benchmark) override;
};