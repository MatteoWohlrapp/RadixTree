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
        //             int64_t value = dist(rd);

        auto debuger = Debuger(db_manager.buffer_manager);
        auto tree = BPlus(db_manager.buffer_manager);
        for (int i = 1; i < 6; i++)
            tree.insert(i * 2, i * 2);

        debuger.traverse_tree(&tree);

        logger->info("Deleting");
        tree.delete_pair(4);
        debuger.traverse_tree(&tree);
        tree.delete_pair(6);
        debuger.traverse_tree(&tree);
    };
    this->benchmark.measure(run, benchmark);
}
