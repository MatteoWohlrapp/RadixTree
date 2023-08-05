
#include "storage_manager.h"
#include "../utils/file.h"
#include <cstring>
#include <cerrno>
#include <cassert>
#include <cmath>

StorageManager::StorageManager(std::filesystem::path base_path_arg, int page_size_arg)
    : base_path(base_path_arg), page_size(page_size_arg)
{
    logger = spdlog::get("logger");
    bitmap_increment = std::ceil(page_size_arg / 8.0) * 8;
    // check if folder and files exist
    if (!std::filesystem::exists(base_path))
    {
        std::filesystem::create_directories(base_path);
    }
    else
    {
        std::filesystem::remove(base_path / data);
    }

    data_fs.open(base_path / data, std::ios::binary | std::ios::in | std::ios::out);

    if (!data_fs.is_open())
    {
        // if the file does not exist, create it
        data_fs.clear();
        data_fs.open(base_path / data, std::ios::binary | std::ios::out);
        data_fs.close();
        // reopen with in/out
        data_fs.open(base_path / data, std::ios::binary | std::ios::in | std::ios::out);
        if (!data_fs.is_open())
        {
            logger->error("File opening failed: ", std::strerror(errno));
            exit(1);
        }
    }

    // page_size needs to be dividable by 8
    free_space_map.resize(bitmap_increment, true);
    free_space_map.reset(0);

    // find correct first free space in file
    find_next_free_space();
}

void StorageManager::destroy()
{
    // delete file content and prevent them being written to the trash can
    data_fs.close();
    data_fs.open(base_path / data, std::ofstream::out | std::ofstream::trunc);
    data_fs.close();
}

void StorageManager::load_page(BHeader *header, uint64_t page_id)
{
    if (page_id > current_page_count)
    {
        logger->error("Page {} does not exist.", page_id);
        exit(1);
    }

    data_fs.seekg(page_id * page_size, std::ios::beg);
    if (!data_fs)
    {
        logger->error("Seekg failed: ", std::strerror(errno));
        exit(1);
    }
    data_fs.read(reinterpret_cast<char *>(header), page_size);

    if (!data_fs)
    {
        logger->error("File read failed: ", std::strerror(errno));
        exit(1);
    }
}

void StorageManager::save_page(BHeader *header)
{
    while (free_space_map.size() <= header->page_id)
    {
        free_space_map.resize(free_space_map.size() + bitmap_increment, true);
    }

    if (current_page_count <= header->page_id)
    {

        data_fs.seekp(0, std::ios::end);
        if (!data_fs)
        {
            logger->error("Seekp failed: ", std::strerror(errno));
            exit(1);
        }
        // we can insert other data
        while (current_page_count <= header->page_id)
        {
            data_fs.write(reinterpret_cast<char *>(header), page_size);
            current_page_count++;
        }
    }
    else
    {
        data_fs.seekp(header->page_id * page_size, std::ios::beg);
        if (!data_fs)
        {
            logger->error("Seekp failed: ", std::strerror(errno));
            exit(1);
        }
        data_fs.write(reinterpret_cast<char *>(header), page_size);
    }
    if (!data_fs)
    {
        logger->error("File write failed: ", std::strerror(errno));
        exit(1);
    }
    free_space_map.reset(header->page_id);
    find_next_free_space();
    data_fs.flush();
}

void StorageManager::delete_page(uint64_t page_id)
{
    assert(page_id != 0 && "Deleting page 0");

    if (free_space_map.size() > page_id)
    {
        free_space_map.set(page_id);
    }

    if (page_id < next_free_space)
        next_free_space = page_id;
}

uint64_t StorageManager::get_unused_page_id()
{
    int next = next_free_space;
    free_space_map.reset(next);
    find_next_free_space();
    return next;
}

void StorageManager::find_next_free_space()
{
    next_free_space = free_space_map.find_next(next_free_space - 1);
    if (next_free_space == free_space_map.npos)
    {
        int previous_size = free_space_map.size();
        free_space_map.resize(free_space_map.size() + bitmap_increment, true);
        next_free_space = free_space_map.find_next(previous_size - 1);
    }
}
