/**
 * @file    RFrame.h
 *
 * @author  Matteo Wohlrapp
 * @date    18.06.2023
 */

#pragma once

#include "./BHeader.h"
#include <stdint.h>

/**
 * @brief Frame that stores the information in the cache
 */
struct RFrame
{
    /// page id of the page that is saved on here
    uint64_t page_id = 0;
    
    /// contains the data
    BHeader *header;
};
