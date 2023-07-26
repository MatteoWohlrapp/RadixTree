
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

    // find correct first free space in file
    find_next_free_space();

    if (!(std::filesystem::file_size(base_path / data) == 0))
    {
        current_page_count = File::get_file_size(&data_fs) / page_size;
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

    uint64_t last_zero = 1;
    for (uint64_t i = 1; i < free_space_map.size(); i++)
    {
        if (free_space_map[i] == 0)
            last_zero = i / 8;
    }

    bitmap_fs.close();
    bitmap_fs.open(base_path / bitmap, std::ios::binary | std::ios::out | std::ios::trunc);

    for (uint64_t i = 0; i < (last_zero + 1) * 8; i += 8)
    {
        char byte = 0;
        for (uint64_t j = 0; j < 8 && j + i < free_space_map.size(); j++)
        {
            if (free_space_map[j + i])
            {
                byte |= (1 << j);
            }
        }
        bitmap_fs.write(&byte, sizeof(byte));
    }

    if (current_page_count > (last_zero + 1) * 8)
    {
        std::ofstream temp_fs(base_path / "temp.bin", std::ios::binary | std::ios::out);
        if (!temp_fs.is_open())
        {
            logger->error("File opening failed for temp file: ", std::strerror(errno));
            exit(1);
        }

        data_fs.seekg(0, std::ios::beg);

        // copy content into new file
        uint64_t num_pages_to_keep = (last_zero + 1) * 8;
        char buffer[page_size];
        for (uint64_t i = 0; i < num_pages_to_keep; ++i)
        {
            data_fs.read(buffer, page_size);
            temp_fs.write(buffer, page_size);
        }

        temp_fs.close();

        // Remove old one and rename new one
        std::filesystem::remove(base_path / data);
        std::filesystem::rename(base_path / "temp.bin", base_path / data);
    }
    data_fs.close();
    bitmap_fs.close();
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

int StorageManager::get_unused_page_id()
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
        int previous_size =  free_space_map.size();
        free_space_map.resize(free_space_map.size() + bitmap_increment, true);
        next_free_space = free_space_map.find_next(previous_size - 1); 
    }
}