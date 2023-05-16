#pragma once

#include "Benchmark.h"
#include "../data/DBManager.h"
#include "spdlog/spdlog.h"

class RunConfig
{
public:
    Benchmark benchmark;
    DBManager db_manager;
    std::shared_ptr<spdlog::logger> logger;

    virtual ~RunConfig() {}

    RunConfig(const int page_size) : benchmark(), db_manager(page_size)
    {
        logger = spdlog::get("logger");
    }

    virtual void execute(bool benchmark) = 0;
};