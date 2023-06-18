/**
 * @file    RadixTree.h
 *
 * @author  Matteo Wohlrapp
 * @date    17.06.2023
 */

#pragma once

#include "../model/BHeader.h"
#include "spdlog/spdlog.h"

class RadixTree
{
private:
    std::shared_ptr<spdlog::logger> logger;

public:
    // Root Node

    RadixTree(); 

    /**
     * @brief Insert an element into the radixtree
     * @param key The key that will be inserted
     * @param header The pointer to the header where the key is located
     */
    void insert(int64_t key, uint64_t page_id, BHeader *header);

    /**
     * @brief Delete the reference from the tree
     * @param key The key that will be deleted
     */
    void delete_reference(int64_t key);

    /**
     * @brief Get a value corresponding to the key
     * @param key The key corresponding to the value
     * @return The value
     */
    int64_t get_value(int64_t key); 
};