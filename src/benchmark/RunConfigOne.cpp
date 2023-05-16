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
        auto debuger = Debuger(db_manager.buffer_manager);
        auto tree = BPlus(db_manager.buffer_manager);
        logger->info("Radix Tree Insert");
        tree.insert(1, 1);
        debuger.traverse_tree(&tree);
        logger->info("Radix Tree Insert");
        tree.insert(2, 4);
        debuger.traverse_tree(&tree);
        logger->info("Radix Tree Insert");
        tree.insert(3, 9);
        debuger.traverse_tree(&tree);
        logger->info("Radix Tree Insert");
        tree.insert(4, 16);
        debuger.traverse_tree(&tree);
        logger->info("Radix Tree Insert");
        tree.insert(5, 25);
        debuger.traverse_tree(&tree);
        logger->info("Radix Tree Insert");
        tree.insert(6, 36);
        debuger.traverse_tree(&tree);
        logger->info("Radix Tree Insert");
        tree.insert(7, 49);
        debuger.traverse_tree(&tree);
        logger->info("Radix Tree Insert");
        tree.insert(8, 64);
        debuger.traverse_tree(&tree);

        int square = tree.get_value(3);
        logger->info("Squared value is: {}", square);
    };
    this->benchmark.measure(run, benchmark);
}
