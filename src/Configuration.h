/**
 * @file    Configuration.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

/**
 * @brief namespace that contains all important variables for the configuration of the database
 */
namespace Configuration
{
    /// sets the overall page_size for the pages written to memory. Subject to constraints: [((PAGE_SIZE - 20) / 2) / 4] > 2
    constexpr int page_size = 52;
    /// sets the number of pages that are be stored in memory by the buffer manager
    constexpr int max_buffer_size = 100;
}
