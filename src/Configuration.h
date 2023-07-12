/**
 * @file    cofiguration.h
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
    /// sets the overall page_size for the pages written to memory. Subject to constraints: [((PAGE_SIZE - 32) / 2) / 8] > 2, also dividable by 16 for header alginment
    constexpr int page_size = 96;
    /// sets the number of pages that are be stored in memory by the buffer manager
    constexpr int max_buffer_size = 5;
}
