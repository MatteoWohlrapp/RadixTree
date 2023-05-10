#pragma once

#include "Benchmark.h"

class RunConfig
{
public:
    Benchmark benchmark;

    virtual ~RunConfig() {} 

    RunConfig()
    {
        benchmark = Benchmark();
    }

    virtual void execute(bool benchmark) = 0;
};