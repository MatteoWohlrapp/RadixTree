#include "RunConfigOne.h"

#include "./btree/BPlus.h"
#include "./debug/Debuger.h"

#include <iostream>
#include <unistd.h>

void RunConfigOne::execute(bool benchmark)
{
    auto run = []
    {
        auto buffer_manager = std::shared_ptr<BufferManager>(new BufferManager(NODE_SIZE));
        auto debuger = Debuger(buffer_manager);
        auto tree = BPlus(buffer_manager);
        std::cout << "Radix Tree Insert" << std::endl;
        tree.insert(1, 1);
        debuger.traverse_tree(&tree);
        std::cout << "Radix Tree Insert" << std::endl;
        tree.insert(2, 4);
        debuger.traverse_tree(&tree);
        std::cout << "Radix Tree Insert" << std::endl;
        tree.insert(3, 9);
        debuger.traverse_tree(&tree);
        std::cout << "Radix Tree Insert" << std::endl;
        tree.insert(4, 16);
        debuger.traverse_tree(&tree);
        std::cout << "Radix Tree Insert" << std::endl;
        tree.insert(5, 25);
        debuger.traverse_tree(&tree);
        std::cout << "Radix Tree Insert" << std::endl;
        tree.insert(6, 36);
        debuger.traverse_tree(&tree);
        std::cout << "Radix Tree Insert" << std::endl;
        tree.insert(7, 49);
        debuger.traverse_tree(&tree);
        std::cout << "Radix Tree Insert" << std::endl;
        tree.insert(8, 64);
        debuger.traverse_tree(&tree);

        int square = tree.get_value(3);
        std::cout << "Squared value is: " << square << std::endl;
    };

    this->benchmark.measure(run, benchmark);
}
