
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
        data_manager.insert(1, 1);
        data_manager.insert(256, 256);
        data_manager.insert(65536, 65536);
        data_manager.insert(16777216, 16777216);
        data_manager.insert(4294967296, 4294967296);
        data_manager.insert(1099511627776, 1099511627776);
        data_manager.insert(281474976710656, 281474976710656);
        data_manager.insert(72057594037927936, 72057594037927936);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();

        data_manager.insert(0, 0);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();

        data_manager.delete_value(65536);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();

        data_manager.delete_value(72057594037927936);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();

        data_manager.insert(8589934592, 8589934592);
        data_manager.insert(12884901888, 12884901888);
        data_manager.insert(17179869184, 17179869184);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();

        data_manager.delete_value(8589934592);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();
    };
    this->benchmark.measure(run, benchmark);
}
