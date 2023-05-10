#pragma once

#include "../btree/BPlus.h"

class Debuger
{
private:
    std::shared_ptr<BufferManager> buffer_manager;

public:
    Debuger(std::shared_ptr<BufferManager> buffer_manager_arg);

    void traverse_tree(BPlus *tree);
};