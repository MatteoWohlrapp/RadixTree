/*
 *   main.cc
 *
 *  Created on: 16.05.2023
 *      Author: Matteo Wohlrapp
 */

/**
 * Codebase for the Thesis: Evaluation of a Radix Tree Cache for Database Management Systems
 * This file marks the entry point into the program
 */

#include "run_suite/run_config_one.h"
#include "run_suite/run_config_two.h"
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
#include "utils/logger.h"
#include "configuration.h"
#include "run_suite/workload.h"
#include "run_suite/workloads_script.h"
#include <boost/dynamic_bitset.hpp>
#include <getopt.h>

// configuration that will be executed, default is configuration one
std::unique_ptr<RunConfig> run;

// workload that will be executed
std::unique_ptr<Workload> workload;

// contains information about all parameters
Configuration::Configuration configuration;

// options for the command line
static struct option long_options[] = {
    {"buffer_size", required_argument, 0, 0},
    {"record_count", required_argument, 0, 0},
    {"operation_count", required_argument, 0, 0},
    {"distribution", required_argument, 0, 0},
    {"insert_proportion", required_argument, 0, 0},
    {"read_proportion", required_argument, 0, 0},
    {"update_proportion", required_argument, 0, 0},
    {"scan_proportion", required_argument, 0, 0},
    {"delete_proportion", required_argument, 0, 0},
    {"cache", required_argument, 0, 0},
    {"radix_tree_size", required_argument, 0, 0},
    {"measure_per_operation", no_argument, 0, 0},
    {"benchmark", no_argument, 0, 'b'},
    {"run_config", required_argument, 0, 'r'},
    {"verbosity_level", required_argument, 0, 'v'},
    {"log_mode", required_argument, 0, 'l'},
    {"delete", no_argument, 0, 'd'},
    {"workload", optional_argument, 0, 'w'},
    {"help", no_argument, 0, 'h'},
    {"coefficient", required_argument, 0, 0},
    {"script", no_argument, 0, 's'},
    {0, 0, 0, 0}};

void print_help()
{
    printf(" -r, --run_config <run config> ........... Select which run configuration you want to choose. Currently available: 1, 2\n");
    printf(" -w, --workload .......................... Select the workload (a, b, c, e, x), If no argument is specified, the general workload with the configured parameters is executed. Be aware that because the parameter is optional, it must in the same argv element, e.g. -we.\n");
    printf(" -s, ..................................... Runs the workload script.\n");
    printf(" -c, --cache  ............................ Activate cache. Creates a radix tree that is placed in front of the b+ tree to act as a cache.\n");
    printf(" -b, --benchmark ......................... Activate benchmark mode. Overwrites any log-level specification to turn all loggers off\n");
    printf(" -v, --verbosity_level <verbosity_level> . Sets the verbosity level for the program: 'o' (off), 'e' (error), 'c' (critical), 'w' (warn), 'i' (info), 'd' (debug), 't' (trace). By default info is used\n");
    printf(" -l, --log_mode <log_mode> ............... Specifies where the logs for the program are written to: 'f' (file), 'c' (console). By default, logs are written to the console when opening the menu\n");
    printf(" -d, --delete ............................ Deletes files from previous runs and resets the db\n");
    printf("--buffer_size <buffer_size>............... Set the buffer size.\n");
    printf("--radix_tree_size <radix_tree_size>....... Set the size of the cache.\n");
    printf("--record_count <record_count>............. Set the record count for a workload.\n");
    printf("--operation_count <operation_count>....... Set the operation count for a workload.\n");
    printf("--distribution <distribution> ............ Set the distribution for a workload.\n");
    printf("--insert_proportion <insert_proportion>... Set the insert proportion for the general workload.\n");
    printf("--read_proportion <read_proportion>....... Set the read proportion for the general workload.\n");
    printf("--update_proportion <update_proportion>... Set the update proportion for the general workload.\n");
    printf("--scan_proportion <scan_proportion>....... Set the scan proportion for the general workload.\n");
    printf("--delete_proportion <delete_proportion>... Set the delete proportion for the general workload.\n");
    printf("--measure_per_operation .................. Select if you want to measure each individual operation of the workload individually, or if you want to measure the throughput.\n");
    printf("-h, --help ...................................... Help\n");
}

const void handle_logging(int argc, char *argsv[])
{
    int option_index = 0;
    spdlog::level::level_enum level = spdlog::level::info;
    char log_mode = 'c';
    while (1)
    {
        int result = getopt_long(argc, argsv, "hbcr:v:l:dw::s", long_options, &option_index);
        if (result == -1)
        {
            break;
        }
        switch (result)
        {
        case 'b':
            level = spdlog::level::off;
            configuration.benchmark = true;
            break;
        case 'v':
        {
            if (configuration.benchmark)
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
                std::cerr << "Error: Please specify a valid log level" << std::endl;
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
                std::cerr << "Error: Please specify a valid log mode" << std::endl;
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
    int option_index = 0;

    while (true)
    {
        int result = getopt_long(argc, argsv, "hbcr:v:l:dw::s", long_options, &option_index);
        if (result == -1)
        {
            break;
        }

        switch (result)
        {
        case 0:
            if (std::string(long_options[option_index].name) == "buffer_size")
                configuration.buffer_size = atoll(optarg);
            else if (std::string(long_options[option_index].name) == "record_count")
                configuration.record_count = atoll(optarg);
            else if (std::string(long_options[option_index].name) == "operation_count")
                configuration.operation_count = atoll(optarg);
            else if (std::string(long_options[option_index].name) == "distribution")
                configuration.distribution = std::string(optarg);
            else if (std::string(long_options[option_index].name) == "insert_proportion")
                configuration.insert_proportion = atof(optarg);
            else if (std::string(long_options[option_index].name) == "read_proportion")
                configuration.read_proportion = atof(optarg);
            else if (std::string(long_options[option_index].name) == "update_proportion")
                configuration.update_proportion = atof(optarg);
            else if (std::string(long_options[option_index].name) == "scan_proportion")
                configuration.scan_proportion = atof(optarg);
            else if (std::string(long_options[option_index].name) == "delete_proportion")
                configuration.delete_proportion = atof(optarg);
            else if (std::string(long_options[option_index].name) == "cache")
                configuration.cache = atoi(optarg);
            else if (std::string(long_options[option_index].name) == "radix_tree_size")
                configuration.radix_tree_size = atoll(optarg);
            else if (std::string(long_options[option_index].name) == "measure_per_operation")
                configuration.measure_per_operation = true;
            else if (std::string(long_options[option_index].name) == "coefficient")
                configuration.coefficient = atof(optarg);
            break;
        case 'c':
            configuration.cache = true;
            break;
        case 'd':
            std::filesystem::remove("./db/data.bin");
            std::filesystem::remove("./db/bitmap.bin");
            break;
        case '?':
            print_help();
            exit(1);
        case 'h':
            print_help();
            exit(1);
        default:
            break;
        }
    }

    optind = 1;
    bool run_option_set = false;

    while (1)
    {
        int result = getopt_long(argc, argsv, "hbcr:v:l:dw::s", long_options, &option_index);

        if (result == -1)
        {
            break;
        }
        switch (result)
        {
        case 'r':
        {
            run_option_set = true;
            if (std::isdigit(*optarg))
            {
                int config = std::stoi(optarg);
                switch (config)
                {
                case 1:
                    run.reset(new RunConfigOne(configuration.buffer_size, configuration.cache, configuration.radix_tree_size));
                    break;
                case 2:
                    run.reset(new RunConfigTwo(configuration.buffer_size, configuration.cache, configuration.radix_tree_size));
                default:
                    break;
                }
            }
            else
            {
                std::cerr << "Error: Argument not correct, must be a digit" << std::endl;
                print_help();
                exit(1);
            }
        }
        break;
        case 'w':
        {
            configuration.run_workload = true;
            run_option_set = true;
            if (optarg)
            {
                char arg = optarg[0];
                switch (arg)
                {
                case 'a':
                    workload.reset(new WorkloadA(configuration.buffer_size, configuration.record_count, configuration.operation_count, configuration.distribution, configuration.coefficient, configuration.cache, configuration.radix_tree_size, configuration.measure_per_operation));
                    break;
                case 'b':
                    workload.reset(new WorkloadB(configuration.buffer_size, configuration.record_count, configuration.operation_count, configuration.distribution, configuration.coefficient, configuration.cache, configuration.radix_tree_size, configuration.measure_per_operation));
                    break;
                case 'c':
                    workload.reset(new WorkloadC(configuration.buffer_size, configuration.record_count, configuration.operation_count, configuration.distribution, configuration.coefficient, configuration.cache, configuration.radix_tree_size, configuration.measure_per_operation));
                    break;
                case 'e':
                    workload.reset(new WorkloadE(configuration.buffer_size, configuration.record_count, configuration.operation_count, configuration.distribution, configuration.coefficient, configuration.cache, configuration.radix_tree_size, configuration.measure_per_operation));
                    break;
                case 'x':
                    workload.reset(new WorkloadX(configuration.buffer_size, configuration.record_count, configuration.operation_count, configuration.distribution, configuration.coefficient, configuration.cache, configuration.radix_tree_size, configuration.measure_per_operation));
                    break;
                }
            }
            else
            {
                workload.reset(new Workload(configuration.buffer_size, configuration.record_count, configuration.operation_count, configuration.distribution, configuration.coefficient, configuration.insert_proportion, configuration.read_proportion, configuration.update_proportion, configuration.scan_proportion, configuration.delete_proportion, configuration.cache, configuration.radix_tree_size, configuration.measure_per_operation));
                break;
            }
        }
        break;
        case 's':
        {
            run_option_set = true;
            configuration.run_workload = true;
            configuration.script = true;
        }
        break; 
        }
    }

    if (!run_option_set)
    {
        std::cerr << "Error: You need to specify a run option, either -w or -r." << std::endl;
        print_help();
        exit(1);
    }
}

int main(int argc, char *argsv[])
{
    handle_logging(argc, argsv);
    handle_arguments(argc, argsv);
    if (configuration.run_workload)
    {
        if (configuration.script)
        {
            auto script = WorkloadScript();
            script.execute();
        }
        else
        {
            workload->execute();
        }
    }
    else
    {
        run->execute(configuration.benchmark);
    }
}