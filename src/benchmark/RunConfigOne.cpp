#include "RunConfigOne.h"

#include "../data/DBManager.h"
#include "./btree/BPlus.h"
#include "./debug/Debuger.h"

#include <iostream>
#include <unistd.h>

void RunConfigOne::execute(bool benchmark)
{
    auto run = [this]
    {
        std::random_device rd;
        std::uniform_int_distribution<int64_t> dist = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX);

        auto debuger = Debuger(db_manager.buffer_manager);
        auto tree = BPlus(db_manager.buffer_manager);
        int64_t values[10];
        for (int i = 0; i < 10; i++)
        {
            int64_t value = dist(rd);
            values[i] = value;
            logger->info("Inserting {}", value);
            tree.insert(value, value);
            debuger.traverse_tree(&tree);
        }

        for (int i = 0; i < 10; i++)
        {
            auto value = tree.get_value(values[i]);
            logger->info("Get value is equal {}, actual {}, expected {}", value == values[i], value, values[i]);
        }
    };
    this->benchmark.measure(run, benchmark);
}
