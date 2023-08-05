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
template <int PAGE_SIZE>
class DataManager
{
private:
    /// path were files related to the DB should be saved
    std::filesystem::path base_path = "./db";

    std::shared_ptr<spdlog::logger> logger;

    StorageManager *storage_manager;
    BufferManager *buffer_manager;

    BPlusTree<PAGE_SIZE> *bplus_tree;
    RadixTree<PAGE_SIZE> *radix_tree = nullptr;

public:
    friend class Debuger;

    /**
     * @brief Constructor for the DataManager
     * @param buffer_size_arg The size of the buffer
     * @param cache_arg Whether cache is enabled
     * @param radix_tree_size_arg The size of the radix tree
     */
    DataManager(uint64_t buffer_size_arg, bool cache_arg, uint64_t radix_tree_size_arg)
    {
        logger = spdlog::get("logger");
        storage_manager = new StorageManager(base_path, PAGE_SIZE);
        buffer_manager = new BufferManager(storage_manager, buffer_size_arg, PAGE_SIZE);
        if (cache_arg)
        {
            radix_tree = new RadixTree<PAGE_SIZE>(radix_tree_size_arg, buffer_manager);
        }
        bplus_tree = new BPlusTree<PAGE_SIZE>(buffer_manager, radix_tree);
    }

    /**
     * @brief Constructor for the DataManager
     * @param storage_manager_arg Storage manager to handle file access
     * @param buffer_manager_arg Buffer manager to handle main memory
     * @param bplus_tree_arg Index structure for file access
     * @param radix_tree_arg Index structure for main memory
     */
    DataManager(StorageManager *storage_manager_arg, BufferManager *buffer_manager_arg, BPlusTree<PAGE_SIZE> *bplus_tree_arg, RadixTree<PAGE_SIZE> *radix_tree_arg) : storage_manager(storage_manager_arg), buffer_manager(buffer_manager_arg), bplus_tree(bplus_tree_arg), radix_tree(radix_tree_arg)
    {
    }

    /**
     * @brief Function that needs to be called before exiting the program, saved all pages to the disc, important to be called before the storage manager is destroyed
     */
    void destroy()
    {
        buffer_manager->destroy();
        storage_manager->destroy();
        if (radix_tree)
        {
            radix_tree->destroy();
            delete radix_tree;
        }
        delete storage_manager;
        delete buffer_manager;
        delete bplus_tree;
    }

    /**
     * @brief Delete an element from the tree
     * @param key The key that will be deleted
     */
    void delete_value(int64_t key)
    {
        if (radix_tree)
        {
            if (radix_tree->delete_value(key))
                return;
        }
        // automatically deleted in bplustree
        bplus_tree->delete_value(key);
    }

    /**
     * @brief Insert an element into the tree
     * @param key The key that will be inserted
     * @param value The value that will be inserted
     */
    void insert(int64_t key, int64_t value)
    {
        // will be automatically added to cache if radix_tree object is passed
        bplus_tree->insert(key, value);
    }

    /**
     * @brief Get a value corresponding to the key
     * @param key The key corresponding a value
     * @return The value
     */
    int64_t get_value(int64_t key)
    {
        if (radix_tree)
        {
            int64_t value = radix_tree->get_value(key);
            if (value != INT64_MIN)
                return value;
        }
        return bplus_tree->get_value(key);
    }

    /**
     * @brief Start at element key and get range consecutive elements
     * @param key The key corresponding to a value
     * @param range The number of elements that are scanned
     * @return The sum of the elements
     */
    int64_t scan(int64_t key, int range)
    {
        if (radix_tree)
        {
            int64_t value = radix_tree->scan(key, range);
            if (value != INT64_MIN)
                return value;
        }
        return bplus_tree->scan(key, range);
    }

    /**
     * @brief Update the value for a key
     * @param key The key corresponding to a value
     * @param value The value corresponding to the key
     */
    void update(int64_t key, int64_t value)
    {
        if (radix_tree)
        {
            if (radix_tree->update(key, value))
                return;
        }
        bplus_tree->update(key, value);
    }

    /**
     * @brief Checks if the b+ tree and the radix tree fullfill all invariants
     * @param num_elements The number of elements that should be in the tree
     * @return if trees are valid
     */
    bool validate(int num_elements)
    {
        std::cout << "Validating b+ tree: " << bplus_tree->validate(num_elements) << std::endl;
        if (radix_tree)
        {
            bool validated = radix_tree->valdidate();
            std::cout << "Validating radix tree: " << validated << std::endl;
        }
        return true;
    }

    /**
     * @brief Returns the current size of the cache
     * @return the size of the cache
     */
    uint64_t get_cache_size()
    {
        if (radix_tree)
        {
            return radix_tree->get_cache_size();
        }
        return 0;
    }

    /**
     * @brief Returns the current size of the buffer
     * @return the size of the buffer
     */
    uint64_t get_current_buffer_size()
    {
        return buffer_manager->get_current_buffer_size();
    }
};