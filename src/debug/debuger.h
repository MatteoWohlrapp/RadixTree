/**
 * @file    debuger.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "../bplus_tree/bplus_tree.h"
#include "../radix_tree/radix_tree.h"
#include "../data/data_manager.h"
#include "../configuration.h"
#include "spdlog/spdlog.h"

/**
 * @brief Helps to debug the code
 */
class Debuger
{
private:
    std::shared_ptr<spdlog::logger> logger;
    DataManager<Configuration::page_size> *data_manager;
    BPlusTree<Configuration::page_size> *bplus_tree;
    RadixTree<Configuration::page_size> *radix_tree;
    BufferManager *buffer_manager;

public:
    friend class DataManager<Configuration::page_size>;
    friend class BPlusTree<Configuration::page_size>;
    friend class RadixTree<Configuration::page_size>;
    /**
     * @brief Constructor for the Debuger
     * @param data_manager_arg Reference to the data manager that contains all the information about data
     */
    Debuger(DataManager<Configuration::page_size> *data_manager_arg = nullptr);

    /**
     * @brief traverses the bplus tree with BFS
     */
    void traverse_bplus_tree();

    /**
     * @brief traverses the radix_treee with BFS
     */
    void traverse_radix_tree();

    /**
     * @brief Traverses the tree and checks if all saved page_ids are unique
     */
    bool are_all_child_ids_unique();

    /**
     * @brief Traverses the tree and checks if a key is contained
     * @param key The key that is searched for
     */
    bool contains_key(int64_t key);
};