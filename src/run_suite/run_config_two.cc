
#include "run_config_two.h"
#include "../data/data_manager.h"
#include "./bplus_tree/bplus_tree.h"
#include "./debug/debuger.h"
#include <iostream>
#include <unistd.h>
#include <unordered_set>

void RunConfigTwo::execute(bool benchmark)
{
    auto run = [this]
    {
        auto debuger = Debuger(&data_manager);

        /*
                1
                256
                65536
                16777216
                4294967296
                1099511627776
                281474976710656
                72057594037927936
        */
        data_manager.insert(-1, -1);
        data_manager.insert(-2, -2);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();
        data_manager.insert(-3, -3);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();
        data_manager.insert(4, 4);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();
        data_manager.insert(5, 5);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();

        logger->info("value : {}", data_manager.get_value(-1));
    };
    this->benchmark.measure(run, benchmark);
}
