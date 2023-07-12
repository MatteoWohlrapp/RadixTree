/**
 * @file    debuger.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "../bplus_tree/bplus_tree.h"
#include "spdlog/spdlog.h"

/**
 * @brief Helps to debug the code
 */
class Debuger
{
private:
    std::shared_ptr<spdlog::logger> logger;
    BufferManager *buffer_manager;

public:
    /**
     * @brief Constructor for the Debuger
     * @param buffer_manager_arg Reference to the buffer manager that implements the tree
     */
    Debuger(BufferManager *buffer_manager_arg);

    /**
     * @brief traverses the bplus tree with BFS
     * @param tree The tree that should be traverses, root node must be accesible
     */
    void traverse_bplus_tree(BPlusTree<Configuration::page_size> *tree);

    /**
     * @brief traverses the radix_treee with BFS
     * @param tree The tree that should be traverses, root node must be accesible
     */
    void traverse_radix_tree(RadixTree *tree);

    /**
     * @brief Traverses the tree and checks if all saved page_ids are unique
     * @param tree The tree that should be traverses, root node must be accesible
     */
    bool are_all_child_ids_unique(BPlusTree<Configuration::page_size> *tree);

    /**
     * @brief Traverses the tree and checks if a key is contained
     * @param tree The tree that should be traverses, root node must be accesible
     * @param key The key that is searched for
     */
    bool contains_key(BPlusTree<Configuration::page_size> *tree, uint64_t key);
};