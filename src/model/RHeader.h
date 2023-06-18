/**
 * @file    BHeader.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include <stdint.h>

/**
 * @brief Data Structure that acts as the header for the pages that are saved on memory.
 */
struct RHeader
{
    /// type of the node
    uint8_t type;

    /// TODO: somehting if fixed? 

    /**
     * @brief Constructor for the header
     * @param type of the node
     */
    RHeader(uint8_t type_arg) : type(type_arg) {};

    /**
     * @brief Constructor that does not change anything, can be used when correct values are already in the right memory position
    */
    RHeader() {}
};