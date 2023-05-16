#pragma once

#include "../model/Header.h"

#include <map>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "spdlog/spdlog.h"

class StorageManager
{
private:
    std::shared_ptr<spdlog::logger> logger;

    std::filesystem::path base_path;

    std::filesystem::path offsets = "offsets.bin";
    std::filesystem::path data = "data.bin";

    std::fstream offset_fs;

    std::fstream data_fs;

    const int page_size;

public:
    // data structure that maps page_id and offset in file
    std::map<u_int32_t, u_int32_t> page_id_offset_map;

    uint32_t current_offset = 0;

    StorageManager(std::filesystem::path base_path_arg, const int page_size_arg);

    void save_page(Header *header);

    void load_page(Header *header, u_int32_t page_id);

    int highest_page_id();

    void destroy();
};