/**
 * @file    Debuger.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "../btree/BPlus.h"
#include "spdlog/spdlog.h"

/**
 * @brief Helps to debug the code
 */
class Debuger
{
private:
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<BufferManager> buffer_manager;

public:
    /**
     * @brief Constructor for the Debuger
     * @param buffer_manager_arg Reference to the buffer manager that implements the tree
     */
    Debuger(std::shared_ptr<BufferManager> buffer_manager_arg);

    /**
     * @brief traverses the tree with BFS
     * @param tree The tree that should be traverses, root node must be accesible
     */
    void traverse_tree(BPlus<Configuration::page_size> *tree);

    /**
     * @brief Traverses the tree and checks if all saved page_ids are unique
     * @param tree The tree that should be traverses, root node must be accesible
     */
    bool are_all_child_ids_unique(BPlus<Configuration::page_size> *tree);

    /**
     * @brief Traverses the tree and checks if a key is contained
     * @param tree The tree that should be traverses, root node must be accesible
     * @param key The key that is searched for
     */
    bool contains_key(BPlus<Configuration::page_size> *tree, uint64_t key);
    };