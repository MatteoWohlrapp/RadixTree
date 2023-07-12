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

/**
 * @brief Makes sure that the DB is initialized correctly and hold all variables necessary to run the system
 */
class DataManager
{
private:
    /// path were files related to the DB should be saved
    std::filesystem::path base_path = "./db";

    std::shared_ptr<spdlog::logger> logger;

public:
    StorageManager *storage_manager;
    BufferManager *buffer_manager;

    BPlusTree<Configuration::page_size> *bplus_tree;
    RadixTree<Configuration::page_size> *radix_tree = nullptr;

    /**
     * @brief Constructor for the DataManager
     */
    DataManager(bool cache_arg = false);

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
};