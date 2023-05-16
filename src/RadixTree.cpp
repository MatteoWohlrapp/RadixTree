
#include "benchmark/RunConfigOne.h"

#include <iostream>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <memory>
#include <regex>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "utils/Logger.h"
#include "Configuration.h"

// gives information if benchmark should be run
bool benchmark = false;
// configuration that will be executed, default is configuration one
std::shared_ptr<RunConfig> run;

void print_help()
{
    printf(" -b ..................... Activate benchmark mode, compute mean transaction time for given benchmark. Overwrites any log-level specification to turn all loggers off\n");
    printf(" -c <run config> ........ Select which run configuration you want to choose. Currently available: 1\n");
    printf(" -v <verbosity_level> ... Sets the verbosity level for the program: 'o' (off), 'e' (error), 'c' (critical), 'w' (warn), 'i' (info), 'd' (debug), 't' (trace). By default info is used\n");
    printf(" -l <log_mode> .......... Specifies where the logs for the program are written to: 'f' (file), 'c' (console). By default, logs are written to the console when opening the menu\n");
    printf(" -h ..................... Help\n");
}

const void handle_logging(int argc, char *argsv[])
{
    spdlog::level::level_enum level = spdlog::level::info;
    char log_mode = 'c';
    while (1)
    {
        int result = getopt(argc, argsv, "hbc:l:v:");
        if (result == -1)
        {
            break;
        }
        switch (result)
        {
        case 'b':
            level = spdlog::level::off;
            benchmark = true;
            break;
        case 'v':
        {
            if (benchmark)
                break;
            if (std::regex_match(optarg, std::regex("[oecwidt]")))
            {
                char arg = optarg[0];
                switch (arg)
                {
                case 'o':
                    level = spdlog::level::off;
                    break;
                case 'e':
                    level = spdlog::level::err;
                    break;
                case 'c':
                    level = spdlog::level::critical;
                    break;
                case 'w':
                    level = spdlog::level::warn;
                    break;
                case 'i':
                    level = spdlog::level::info;
                    break;
                case 'd':
                    level = spdlog::level::debug;
                    break;
                case 't':
                    level = spdlog::level::trace;
                    break;
                }
            }
            else
            {
                std::cout << "Error: Please specify a valid log level" << std::endl;
                print_help();
                exit(1);
            }
        }
        break;
        case 'l':
            if (std::regex_match(optarg, std::regex("[cf]")))
            {
                log_mode = optarg[0];
            }
            else
            {
                std::cout << "Error: Please specify a valid log mode" << std::endl;
                print_help();
                exit(1);
            }
            break;
        }
    }
    optind = 1;
    Logger::initialize_loggers(level, log_mode);
}

void handle_arguments(int argc, char *argsv[])
{

    while (1)
    {
        int result = getopt(argc, argsv, "hbc:l:v:");

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
            exit(1);
        case 'c':
        {
            if (std::isdigit(*optarg))
            {
                int config = std::stoi(optarg);
                switch (config)
                {
                case 1:
                    run.reset(new RunConfigOne(Configuration::page_size));
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
    // first turn the logging on or off
    handle_logging(argc, argsv);
    run.reset(new RunConfigOne(Configuration::page_size));
    handle_arguments(argc, argsv);
    run->execute(benchmark);
}