#include "RunConfigOne.h"

#include "../data/DataManager.h"
#include "./btree/BPlus.h"
#include "./debug/Debuger.h"

#include <iostream>
#include <unistd.h>
#include <unordered_set>

void RunConfigOne::execute(bool benchmark)
{
    auto run = [this]
    {
        std::mt19937 generator(42); // 42 is the seed value
        std::uniform_int_distribution<int64_t> dist(0, 255);
        std::unordered_set<int64_t> unique_values;
        int64_t values[100];

        auto debuger = Debuger(data_manager.buffer_manager);

        for (int i = 0; i < 40; i++)
        {
            logger->info("Inserting");
            int64_t value = dist(generator); // generate a random number

            // Ensure we have a unique value, if not, generate another one
            while (unique_values.count(value))
            {
                value = dist(generator);
            }

            unique_values.insert(value);
            values[i] = value;
            data_manager.insert(value, value);
        }

        for (int i = 0; i < 40; i++)
        {
            logger->info("Deleting {} at index {}", values[i], i);
            logger->flush();
            data_manager.delete_pair(values[i]);
            debuger.traverse_btree(data_manager.btree);
            debuger.traverse_rtree(data_manager.rtree);
        }
    };
    this->benchmark.measure(run, benchmark);
}
