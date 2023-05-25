/**
 * @file    Header.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include <stdint.h>

/**
 * @brief Data Structure that acts as the header for the pages that are saved on memory.
 */
struct Header
{
    /// id of the page
    uint64_t page_id;
    /// specifies if inner or outer node
    bool inner = false;
    /// padding to align to 4 byte
    char padding[3];

    /**
     * @brief Constructor for the header
     * @param page_id_arg unique id for the page
     * @param inner_arg specifies if it will be an inner or outer node - all pages are nodes in this implementation
     */
    Header(uint64_t page_id_arg, bool inner_arg) : page_id(page_id_arg), inner(inner_arg){};

    /**
     * @brief Copy Constructor for the header
     * @param other Header to copy from
     */
    Header(Header *other) : page_id(other->page_id), inner(other->inner) {}

    /**
     * @brief Constructor that does not change anything, can be used when correct values are already in the right memory position
    */
    Header() {}
};