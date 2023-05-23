/**
 * @file    BufferManager.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "../model/Frame.h"
#include "StorageManager.h"
#include <stdint.h>
#include <vector>
#include <map>
#include <random>
#include "spdlog/spdlog.h"

/**
 * @brief Handles the pages currently stored in memory
 */
class BufferManager
{

private:
    std::shared_ptr<spdlog::logger> logger;
    /// write and read to disc
    std::shared_ptr<StorageManager> storage_manager;
    /// data structure for page id mapping
    std::map<u_int32_t, Frame *> page_id_map;
    /// information about how full the buffer is right now
    uint32_t current_buffer_size = 0;

    /// random number generator used for eviction of pages
    std::random_device rd;
    /// distributen associated with random number generator
    std::uniform_int_distribution<int> dist;

    /**
     * @brief Get a specific page from disc
     * @param page_id The page id of the page that should be retreived
     */
    void fetch_page_from_disk(uint32_t page_id);

    /**
     * @brief Removes pages from memory to free space for further pages
     * @returns A free frame that can be used to write again
     */
    Frame *evict_page();

public:
    /**
     * @brief Constructor for the Buffer Manager
     * @param storage_manager_arg A reference to the storage manager
     * @param page_id The lowest page it that can be used from here
     */
    BufferManager(std::shared_ptr<StorageManager> storage_manager_arg);

    /**
     * @brief Request a page
     * @param page_id The identifier of the page to retrieve
     * @return A pointer to the page
     */
    Header *request_page(uint32_t page_id);

    /**
     * @brief Creates a new page
     * @return A pointer to the page
     */
    Header *create_new_page();

    /**
     * @brief Deletes a page from the buffer
     * @param page_id The page id from the page that should be deleted
     */
    void delete_page(uint32_t page_id);

    /**
     * @brief Fixes a page, so no other thread modify it
     * @param page_id The page id of the page that should be fixed
     */
    void fix_page(uint32_t page_id);

    /**
     * @brief Unfixes a page and enables other threads to modify it again
     * @param page_id The page id of the page that should be fixed
     */
    void unfix_page(uint32_t page_id);

    /**
     * @brief Marks a page dirty so it will be written to memory
     * @param page_id The page id of the page that should be flagged dirty
     */
    void mark_dirty(uint32_t page_id);

    /**
     * @brief Function that needs to be called before exiting the program, saved all pages to the disc, important to be called before the storage manager is destroyed
     */
    void destroy();
};