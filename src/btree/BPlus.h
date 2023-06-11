/**
 * @file    BPlus.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "../data/BufferManager.h"
#include "Nodes.h"
#include <array>
#include <math.h>
#include <iostream>
#include <cassert>
#include "spdlog/spdlog.h"

/// forward declaration
class BPlusTest;

/**
 * @brief Implements the B+ Tree of the Database
 */
class BPlus
{
private:
    std::shared_ptr<spdlog::logger> logger;

    std::shared_ptr<BufferManager> buffer_manager;

    /**
     * @brief Inserts recursively into the tree
     * @param header The header of the current node
     * @param key The key to insert
     * @param value The value to insert
     */
    void recursive_insert(Header *header, int64_t key, int64_t value);

    /**
     * @brief Delete recursively from the tree
     * @param header The header of the current node
     * @param key The key to delete
     */
    void recursive_delete(Header *header, int64_t key);

    /**
     * @brief Get the value to a key recursively
     * @param header The header of the current node
     * @param key The key corresponding to the value
     */
    int64_t recursive_get_value(Header *header, int64_t key);

    /**
     * @brief Splits the outer node and copies values
     * @param header The header of the current node
     * @param index_to_split The index where the node needs to be split
     * @return The page_id of the new node containing the higher elements
     */
    uint64_t split_outer_node(Header *header, int index_to_split);

    /**
     * @brief Splits the inner node and copies values
     * @param header The header of the current node
     * @param index_to_split The index where the node needs to be split
     * @return The page_id of the new node containing the higher elements
     */
    uint64_t split_inner_node(Header *header, int index_to_split);

    /**
     * @brief Returns the index where to split depending on the size
     * @param max_size The maximum capacity of the node
     * @return The index where to split the node
     */
    int get_split_index(int max_size);

    bool substitute(Header* header, Header* child_header); 

    void merge(Header* header, Header* child_header); 

    int64_t find_biggest_smallest(Header* header); 

    int64_t recursive_find_biggest_smallest(Header* header); 

public:
    friend class BPlusTest;
    /// root of tree
    uint64_t root_id;

    /**
     * @brief Constructor for the B+ tree
     * @param buffer_manager_arg The buffer manager
     */
    BPlus(std::shared_ptr<BufferManager> buffer_manager_arg);

    /**
     * @brief Insert an element into the tree
     * @param key The key that will be inserted
     * @param value The value that will be inserted
     */
    void insert(int64_t key, int64_t value);

    /**
     * @brief Delete an element from the tree
     * @param key The key that will be deleted
     */
    void delete_pair(int64_t key);

    /**
     * @brief Get a value corresponding to the key
     * @param key The key corresponding a value
     * @return The value
     */
    int64_t get_value(int64_t key);
};