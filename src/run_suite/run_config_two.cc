
#include "run_config_two.h"
#include "../data/data_manager.h"
#include "./bplus_tree/bplus_tree.h"
#include "./debug/debugger.h"
#include <iostream>
#include <unistd.h>
#include <unordered_set>

void RunConfigTwo::execute(bool benchmark)
{
    auto run = [this]
    {
        auto debuger = Debuger(&data_manager);

        for (int i = 0; i < 20; i++)
            data_manager.insert(i, i);

#ifdef _OPENMP
#pragma omp parallel for
#endif
        for (int i = 0; i < 20; i++)
        {
            int64_t value = data_manager.get_value(i);
#pragma omp critical
            {
                std::cout << value << std::endl;
            }
        }
    };
    this->benchmark.measure(run, benchmark);
}
