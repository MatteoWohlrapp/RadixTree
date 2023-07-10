/**
 * @file    BHeader.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include <stdint.h>
#include <iostream>
#include <cassert>

/**
 * @brief Data Structure that acts as the header for the pages that are saved on memory.
 */
struct RHeader
{
    /// type of the node
    uint16_t type;

    /// is leaf or inner
    bool leaf;

    /// number of levels skipped, necessary because of path compression
    uint8_t depth;

    /// partial key that gives info about the key in the compressed path
    int64_t key;

    /// how many elements are in the node
    uint16_t current_size = 0;

    /// fix and unfix
    uint8_t fix_count = 0;

    /**
     * @brief Constructor for the header
     * @param type of the node
     */
    RHeader(uint16_t type_arg, bool leaf_arg) : type(type_arg), leaf(leaf_arg){};

    /**
     * @brief Constructor for the header
     * @param type of the node
     */
    RHeader(uint16_t type_arg, bool leaf_arg, uint8_t depth_arg, int64_t key_arg, uint16_t current_size_arg) : type(type_arg), leaf(leaf_arg), depth(depth_arg), key(key_arg), current_size(current_size_arg){};

    /**
     * @brief Constructor that does not change anything, can be used when correct values are already in the right memory position
     */
    RHeader() {}

    /**
     * @brief Fixes a node by increasing the fix count
     */
    void fix_node()
    {
        assert(fix_count == 0 && "Trying to fix rnode that is already fixed.");

        fix_count++;
    }

    /**
     * @brief Unfixes a node by decreasing the fix count
     */
    void unfix_node()
    {
        assert(fix_count == 1 && "Trying to unfix rnode that is not fixed.");

        fix_count--;
    }
};