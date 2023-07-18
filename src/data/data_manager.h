/**
 * @file    data_manager.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "buffer_manager.h"
#include "storage_manager.h"
#include "../radix_tree/radix_tree.h"
#include "../configuration.h"
#include "../bplus_tree/bplus_tree.h"

class Debuger;

/**
 * @brief Makes sure that the DB is initialized correctly and hold all variables necessary to run the system
 */
class DataManager
{
private:
    /// path were files related to the DB should be saved
    std::filesystem::path base_path = "./db";

    std::shared_ptr<spdlog::logger> logger;

    StorageManager *storage_manager;
    BufferManager *buffer_manager;

    BPlusTree<Configuration::page_size> *bplus_tree;
    RadixTree<Configuration::page_size> *radix_tree = nullptr;

public:
    friend class Debuger;

    /**
     * @brief Constructor for the DataManager
     */
    DataManager(int buffer_size_arg, bool cache_arg, int radix_tree_size_arg);

    /**
     * @brief Function that needs to be called before exiting the program, saved all pages to the disc, important to be called before the storage manager is destroyed
     */
    void destroy();

    /**
     * @brief Delete an element from the tree
     * @param key The key that will be deleted
     */
    void delete_value(int64_t key);

    /**
     * @brief Insert an element into the tree
     * @param key The key that will be inserted
     * @param value The value that will be inserted
     */
    void insert(int64_t key, int64_t value);

    /**
     * @brief Get a value corresponding to the key
     * @param key The key corresponding a value
     * @return The value
     */
    int64_t get_value(int64_t key);

    /**
     * @brief Start at element key and get range consecutive elements
     * @param key The key corresponding to a value
     * @param range The number of elements that are scanned
     * @return The sum of the elements
     */
    int64_t scan(int64_t key, int range); 

    /**
     * @brief Update the value for a key
     * @param key The key corresponding to a value
     * @param value The value corresponding to the key
     */
    void update(int64_t key, int64_t value); 
};