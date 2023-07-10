#include "RunConfigTwo.h"

#include "../data/DataManager.h"
#include "./btree/BPlus.h"
#include "./debug/Debuger.h"

#include <iostream>
#include <unistd.h>
#include <unordered_set>

void RunConfigTwo::execute(bool benchmark)
{
    auto run = [this]
    {
        auto debuger = Debuger(data_manager.buffer_manager);

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
        data_manager.insert(65536, 65536);
        data_manager.insert(256, 256);
        data_manager.insert(0, 0);
        data_manager.insert(1, 1);
        data_manager.insert(2, 2);
        data_manager.insert(3, 3);
        data_manager.insert(4, 4);

        debuger.traverse_btree(data_manager.btree);
        debuger.traverse_rtree(data_manager.rtree);

        data_manager.delete_pair(2);
        data_manager.delete_pair(0);
        data_manager.delete_pair(1);

        debuger.traverse_btree(data_manager.btree);
        debuger.traverse_rtree(data_manager.rtree);

        data_manager.delete_pair(3);

        debuger.traverse_btree(data_manager.btree);
        debuger.traverse_rtree(data_manager.rtree);

        data_manager.delete_pair(4);

        debuger.traverse_btree(data_manager.btree);
        debuger.traverse_rtree(data_manager.rtree);
    };
    this->benchmark.measure(run, benchmark);
}
