#pragma once

#include "../btree/BPlus.h"
#include "spdlog/spdlog.h"

class Debuger
{
private:
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<BufferManager> buffer_manager;

public:
    Debuger(std::shared_ptr<BufferManager> buffer_manager_arg);

    void traverse_tree(BPlus *tree);
};