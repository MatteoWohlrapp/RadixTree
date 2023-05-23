
#include "StorageManager.h"

#include <cstring>
#include <cerrno>
#include <cassert>
#include <cmath>
#include "../utils/File.h"

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

    bitmap_fs.open(base_path / bitmap, std::ios::binary | std::ios::in | std::ios::out);
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
    if (!bitmap_fs.is_open())
    {
        // if the file does not exist, create it
        bitmap_fs.clear();
        bitmap_fs.open(base_path / bitmap, std::ios::binary | std::ios::out);
        bitmap_fs.close();
        // reopen with in/out
        bitmap_fs.open(base_path / bitmap, std::ios::binary | std::ios::in | std::ios::out);
        if (!bitmap_fs.is_open())
        {
            logger->error("File opening failed: ", std::strerror(errno));
            exit(1);
        }
    }

    if (!(std::filesystem::file_size(base_path / bitmap) == 0))
    {
        // read bitmap if available
        char byte = 0;
        while (bitmap_fs.read(reinterpret_cast<char *>(&byte), sizeof(byte)))
        {

            for (int i = 0; i < 8; i++)
            {
                char temp = byte;
                free_space_map.push_back((temp >> i) & 1);
            }
        }

        // check if previous reads failed
        if (!bitmap_fs && !bitmap_fs.eof())
        {
            std::cerr << "File read failed: " << std::strerror(errno) << std::endl;
            exit(1);
        }
        else
        {
            bitmap_fs.clear();
        }
    }
    else
    {
        // page_size needs to be dividable by 8
        free_space_map.resize(bitmap_increment, true);
        free_space_map.reset(0);
    }

    if (!(std::filesystem::file_size(base_path / data) == 0))
    {
        current_page_count = File::get_file_size(data_fs) / page_size;
    }
}

void StorageManager::destroy()
{
    // we need to save the bitmap file here
    bitmap_fs.close();
    bitmap_fs.open(base_path / bitmap, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!bitmap_fs.is_open())
    {
        logger->error("File opening failed: ", std::strerror(errno));
        exit(1);
    }

    for (int i = 0; i < free_space_map.size(); i += 8)
    {
        char byte = 0;
        for (int j = 0; j < 8 && j + i < free_space_map.size(); j++)
        {
            if (free_space_map[j + i])
            {
                byte |= (1 << j);
            }
        }
        bitmap_fs.write(&byte, sizeof(byte));
    }

    bitmap_fs.close();
    data_fs.close();
}

void StorageManager::load_page(Header *header, u_int32_t page_id)
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

void StorageManager::save_page(Header *header)
{
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

        while (free_space_map.size() < header->page_id)
        {
            free_space_map.resize(free_space_map.size() + page_size, true);
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
    data_fs.flush();
}

int StorageManager::get_unused_page_id()
{
    int next = free_space_map.find_next(0);

    if (next == free_space_map.npos)
    {
        free_space_map.resize(free_space_map.size() + bitmap_increment, true);
        next = free_space_map.find_next(0);
        free_space_map.reset(next);
    }
    return next;
}
