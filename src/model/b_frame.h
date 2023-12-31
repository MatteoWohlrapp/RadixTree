/**
 * @file    b_frame.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "./b_header.h"
#include <stdint.h>

/**
 * @brief Frame that wraps around a header to store additional information used by the buffer manager
 */
struct BFrame
{
    /// used to fix and unfix page and protect memory access
    uint16_t fix_count = 0;
    /// specifies if it needs to be written to memory
    bool dirty = false;
    /// used to implement a simple heuristic for the eviction of pages
    bool marked = false;
    /// contains the data of the page
    BHeader header;
};
