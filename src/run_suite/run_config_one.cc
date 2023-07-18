
#include "run_config_one.h"
#include "../data/data_manager.h"
#include "./bplus_tree/bplus_tree.h"
#include "./radix_tree/radix_tree.h"
#include "./debug/debuger.h"

#include <iostream>
#include <unistd.h>
#include <unordered_set>

void RunConfigOne::execute(bool benchmark)
{
    auto run = [this]
    {
        std::mt19937 generator(42); // 42 is the seed value
        std::uniform_int_distribution<int64_t> dist(-1000, 1000);
        std::unordered_set<int64_t> unique_values;
        int64_t values[15];

        auto debuger = Debuger(&data_manager);

        for (int i = 0; i < 15; i++)
        {
            logger->debug("Inserting");
            int64_t value = dist(generator);

            while (unique_values.count(value))
            {
                value = dist(generator);
            }

            unique_values.insert(value);
            values[i] = value;
            data_manager.insert(value, value);

            debuger.traverse_bplus_tree();
            debuger.traverse_radix_tree();
        }
    };
    this->benchmark.measure(run, benchmark);
}
