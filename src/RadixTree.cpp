
#include "benchmark/RunConfigOne.h"

#include <iostream>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

// gives information if benchmark should be run
bool benchmark = false;
// run that will be executed, default is configuration one
std::shared_ptr<RunConfig> run;

void print_help()
{
    printf(" -b ................. Activate benchmark mode, compute mean transaction time for given benchmark. Overwrites any log-level specification to turn all loggers off\n");
    printf(" -c <run config> .... Select which run configuration you want to choose. Currently available: 1\n");
    printf(" -h ................. Help\n");
}

void handle_arguments(int argc, char *argsv[])
{
    while (1)
    {
        int result = getopt(argc, argsv, "hbc:");

        if (result == -1)
        {
            break;
        }
        switch (result)
        {
        case '?':
            print_help();
            exit(1);
        case 'h':
            print_help();
            exit(0);
        case 'b':
        {
            benchmark = true;
        }
        break;
        case 'c':
        {
            if (std::isdigit(*optarg))
            {
                int config = std::stoi(optarg);
                switch (config)
                {
                case 1:
                    run.reset(new RunConfigOne());
                    break;
                default:
                    break;
                }
            }
            else
            {
                std::cout << "Argument not correct, must be a digit" << std::endl;
                print_help();
                exit(1);
            }
        }
        break;
        default:
            break;
        }
    }
}

int main(int argc, char *argsv[])
{
    run.reset(new RunConfigOne());
    std::cout << "Welcome to your thesis." << std::endl;
    handle_arguments(argc, argsv);
    run->execute(benchmark);
}