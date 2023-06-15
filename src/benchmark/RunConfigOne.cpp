#include "RunConfigOne.h"

#include "../data/DBManager.h"
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
        std::uniform_int_distribution<int64_t> dist(-1000, 1000);
        std::unordered_set<int64_t> unique_values;
        int64_t values[100];

        auto debuger = Debuger(db_manager.buffer_manager);
        auto tree = BPlus(db_manager.buffer_manager);

        for (int i = 0; i < 100; i++)
        {
            int64_t value = dist(generator); // generate a random number

            // Ensure we have a unique value, if not, generate another one
            while (unique_values.count(value))
            {
                value = dist(generator);
            }

            unique_values.insert(value);
            values[i] = value;
            tree.insert(value, value);
        }

        for (int i = 0; i < 100; i++)
        {
            debuger.traverse_tree(&tree);
            logger->info("Deleting {} at index {}", values[i], i);
            tree.delete_pair(values[i]);
        }
    };
    this->benchmark.measure(run, benchmark);
}
