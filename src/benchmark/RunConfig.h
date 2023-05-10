#pragma once

#include "Benchmark.h"

class RunConfig
{
private:
    Benchmark benchmark;

    virtual void run() = 0;

public:
    virtual ~RunConfig() {} 

    RunConfig()
    {
        benchmark = Benchmark();
    }

    virtual void execute(bool benchmark) = 0;
};