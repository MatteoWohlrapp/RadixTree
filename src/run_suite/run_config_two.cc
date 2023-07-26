
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
        data_manager.insert(144115188075855872, 144115188075855872);
        data_manager.insert(216172782113783808, 216172782113783808);
        data_manager.insert(288230376151711744, 288230376151711744);
        logger->debug("Getting value"); 
        data_manager.get_value(288230376151711744);
        debuger.traverse_bplus_tree();
        debuger.traverse_radix_tree();
    };
    this->benchmark.measure(run, benchmark);
}
