#pragma once

#include "RunConfig.h"

class RunConfigOne : public RunConfig
{
public:
    RunConfigOne(const int page_size) : RunConfig(page_size) {}

    void execute(bool benchmark) override;
};