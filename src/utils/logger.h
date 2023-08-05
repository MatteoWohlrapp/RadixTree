/**
 * @file    logger.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>
#include <ctime>
#include <list>
#include <iostream>
#include <filesystem>
#include "time.h"

/**
 * @brief namespace for the logger, includes functions for flushing and initialization of spdlog loggers
 */
namespace Logger
{
    /**
     * @brief Initialises the logger, must be called before objects are created because otherwise segfaults occur
     * @param level gives the level which is set for all loggers
     * @param log_mode sets the mode for logging, either 'c' (console) or 'f' (file)
     */
    inline const void initialize_loggers(spdlog::level::level_enum level, char log_mode)
    {
        try
        {
            if (log_mode == 'c' || level == spdlog::level::off)
            {
                auto logger = spdlog::stdout_color_mt("logger");
                logger->set_level(level);
            }
            else
            {
                auto path = "../logs/";
                // check if folder and files exist
                if (!std::filesystem::exists(path))
                {
                    std::filesystem::create_directories(path);
                }
                std::string prefix = Time::getDateTime();

                std::string log_filename = path + prefix + "log.txt";
                auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_filename, false);
                auto logger = std::make_shared<spdlog::logger>("logger", sink);
                logger->set_level(level);
                spdlog::register_logger(logger);
            }
        }
        catch (const spdlog::spdlog_ex &ex)
        {
            std::cout << "Log initialization failed: " << ex.what() << std::endl;
        }
    }
}