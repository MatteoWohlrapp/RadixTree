#pragma once 

#include "RunConfig.h"

class RunConfigOne : public RunConfig{
private: 
    void run() override; 

public: 
    RunConfigOne() : RunConfig() {}
    
    void execute(bool benchmark) override; 
}; 