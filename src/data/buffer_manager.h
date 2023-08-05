/**
 * @file    buffer_manager.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "../model/b_frame.h"
#include "storage_manager.h"
#include <stdint.h>
#include <vector>
#include <map>
#include <random>
#include "spdlog/spdlog.h"

/// forward declaration
class BufferManagerTest;
class BPlusTreeTest;

/**
 * @brief Handles the pages currently stored in memory
 */
class BufferManager
{

private:
    std::shared_ptr<spdlog::logger> logger;
    /// write and read to disc
    StorageManager *storage_manager;
    /// data structure for page id mapping
    std::map<uint64_t, BFrame *> page_id_map;
    /// information about how full the buffer is right now
    uint64_t current_buffer_size = 0;

    /// random number generator used for eviction of pages
    std::random_device rd;
    /// distributen associated with random number generator
    std::uniform_int_distribution<int> dist;

    /// how many pages will be stored in the buffer manager
    uint64_t buffer_size;

    /// the size of the page
    int page_size;

    /**
     * @brief Get a specific page from disc
     * @param page_id The page id of the page that should be retreived
     */
    void fetch_page_from_disk(uint64_t page_id);

    /**
     * @brief Removes pages from memory to free space for further pages
     * @returns A free frame that can be used to write again
     */
    BFrame *evict_page();

public:
    friend class BufferManagerTest;
    friend class BPlusTreeTest;

    /**
     * @brief Constructor for the Buffer Manager
     * @param storage_manager_arg A reference to the storage manager
     * @param buffer_size_arg The size of the buffer
     * @param page_size_arg The size of the page that needs to be allocated
     */
    BufferManager(StorageManager *storage_manager_arg, uint64_t buffer_size_arg, int page_size_arg);

    /**
     * @brief Request a page
     * @param page_id The identifier of the page to retrieve
     * @return A pointer to the page
     */
    BHeader *request_page(uint64_t page_id);

    /**
     * @brief Creates a new page
     * @return A pointer to the page
     */
    BHeader *create_new_page();

    /**
     * @brief Deletes a page from the buffer
     * @param page_id The page id from the page that should be deleted
     */
    void delete_page(uint64_t page_id);

    /**
     * @brief Fixes a page, so no other thread modify it
     * @param page_id The page id of the page that should be fixed
     */
    void fix_page(uint64_t page_id);

    /**
     * @brief Unfixes a page and enables other threads to modify it again
     * @param page_id The page id of the page that should be fixed
     * @param dirty Specifies if the page corresponding to page_id has been modified
     */
    void unfix_page(uint64_t page_id, bool dirty);

    /**
     * @brief Marks a page dirty
     * @param page_id The page id of the page that should be fixed
     */
    void mark_dirty(uint64_t page_id);

    /**
     * @brief Function that needs to be called before exiting the program, saved all pages to the disc, important to be called before the storage manager is destroyed
     */
    void destroy();

    /**
     * @brief Returns the current size of the buffer
     * @return the size of the buffer
     */
    uint64_t get_current_buffer_size();
};