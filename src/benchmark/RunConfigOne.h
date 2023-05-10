#pragma once

#include "RunConfig.h"

class RunConfigOne : public RunConfig
{
public:
    RunConfigOne() : RunConfig() {}

    void execute(bool benchmark) override;
};