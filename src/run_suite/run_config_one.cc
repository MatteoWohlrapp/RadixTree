
#include "run_config_one.h"
#include "../data/data_manager.h"
#include "./bplus_tree/bplus_tree.h"
#include "./radix_tree/radix_tree.h"
#include "./debug/debugger.h"

#include <iostream>
#include <unistd.h>
#include <unordered_set>

void RunConfigOne::execute(bool benchmark)
{
    auto run = [this]
    {
        std::mt19937 generator(42); // 42 is the seed value
        std::uniform_int_distribution<int64_t> dist(INT64_MIN + 1, INT64_MAX);
        std::unordered_set<int64_t> unique_values;
        int64_t values[10000];

        auto debuger = Debuger(&data_manager);

        for (int i = 0; i < 10000; i++)
        {
            int64_t value = dist(generator);

            while (unique_values.count(value))
            {
                value = dist(generator);
            }

            unique_values.insert(value);
            values[i] = value;
            logger->debug("Inserting: {}", i);
            logger->flush();
            data_manager.insert(value, value);
        }
        std::geometric_distribution<int> geom(0.01);

        for (int i = 0; i < 1000; i++)
        {
            int num = geom(generator);
            if (num >= 10000)
                num = 9999;
            logger->debug("Reading i: {}", i);
            logger->flush();
            data_manager.get_value(values[num]);
        }
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();
    };
    this->benchmark.measure(run, benchmark);
}
