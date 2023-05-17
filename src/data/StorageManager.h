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

/**
 * @brief Is responsible for the writing and reading to disk
 */
class StorageManager
{
private:
    std::shared_ptr<spdlog::logger> logger;

    std::filesystem::path base_path;

    /// path to the offset file
    std::filesystem::path offsets = "offsets.bin";
    /// path to the data file
    std::filesystem::path data = "data.bin";

    /// file handle for the offset file
    std::fstream offset_fs;
    /// file handle for the data file
    std::fstream data_fs;

    const int page_size;

public:
    /// data structure that maps page_id and offset in file
    std::map<u_int32_t, u_int32_t> page_id_offset_map;

    /// how many bytes have been written to the data file
    uint32_t current_offset = 0;

    /**
     * @brief Constructor for the storage manager
     * @param base_path_arg The base path of the folder where the data and offset file will be placed in
     * @param page_size_arg The page size saved into memory
     */
    StorageManager(std::filesystem::path base_path_arg, const int page_size_arg);

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
     * @brief Gives the highest page id that is currently stored in memory
     * @return highest page id currently stored
     */
    int highest_page_id();

    /**
     * @brief Used to save the offset to disc, needs to be called before exiting the program
     */
    void destroy();
};