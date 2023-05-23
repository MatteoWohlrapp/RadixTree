/**
 * @file    StorageManager.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "../model/Header.h"
#include <map>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "spdlog/spdlog.h"
#include <boost/dynamic_bitset.hpp>
#include "../Configuration.h"

/// forward declaration
class StorageManagerTest;

/**
 * @brief Is responsible for the writing and reading to disk
 */
class StorageManager
{
private:
    std::shared_ptr<spdlog::logger> logger;

    std::filesystem::path base_path;

    /// path to the bitmap file
    std::filesystem::path bitmap = "bitmap.bin";
    /// path to the data file
    std::filesystem::path data = "data.bin";

    /// file handle for the offset file
    std::fstream bitmap_fs;
    /// file handle for the data file
    std::fstream data_fs;

    int page_size;

    int bitmap_increment; 

public:
    /// data structure that shows if page_id is currently in use, 1 is free, 0 is occupied
    boost::dynamic_bitset<> free_space_map;

    int current_page_count = 0;
    /**
     * @brief Constructor for the storage manager
     * @param base_path_arg The base path of the folder where the data and offset file will be placed in
     * @param page_size_arg The page size saved into memory
     */
    StorageManager(std::filesystem::path base_path_arg, int page_size_arg);

    /**
     * @brief Saves a page to disc
     * @param header A pointer to the header (page) that contains the information that should be written to file
     */
    void save_page(Header *header);

    /**
     * @brief Loads a page from disc
     * @param header A pointer to the header (page) that should be written to
     * @param page_id The unique identifier of the page that should be loaded
     */
    void load_page(Header *header, u_int32_t page_id);

    /**
     * @brief Gives an unsued page_id, when requesting a new unused page_id, the bitmap is already set, so its important to write the page at the end
     * @return page_id that is currently not in use
     */
    int get_unused_page_id();

    /**
     * @brief Used to save the offset to disc, needs to be called before exiting the program
     */
    void destroy();

    friend class StorageManagerTest;
};