
#include "StorageManager.h"
#include "../Configuration.h"

#include <cstring>
#include <cerrno>

void print_file_content(std::fstream &data_fs)
{

    // Read and print the content
    data_fs.seekg(0, std::ios::end);
    std::streamoff endPosition = data_fs.tellg();
    std::streamoff fileSize = endPosition;
    std::cout << "FileSize: " << std::dec << fileSize << std::endl;
    // Reset the file pointer to the beginning of the file
    data_fs.seekg(0, std::ios::beg);

    char ch;
    while (data_fs.get(ch))
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch & 0xFF) << " ";
    }
    data_fs.clear();

    std::cout << std::endl;
}

StorageManager::StorageManager(std::filesystem::path base_path_arg, const int page_size_arg)
    : base_path(base_path_arg), page_size(page_size_arg)
{
    logger = spdlog::get("logger");
    // check if folder and files exist
    if (!std::filesystem::exists(base_path))
    {
        std::filesystem::create_directories(base_path);
    }

    offset_fs.open(base_path / offsets, std::ios::binary | std::ios::in | std::ios::out);
    data_fs.open(base_path / data, std::ios::binary | std::ios::in | std::ios::out);
    // we cant open in append, so when they dont exist we need to open in out first to create them

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

    if (!offset_fs.is_open())
    {

        // if the file does not exist, create it
        offset_fs.clear();
        offset_fs.open(base_path / offsets, std::ios::binary | std::ios::out);
        offset_fs.close();
        // reopen with in/out
        offset_fs.open(base_path / offsets, std::ios::binary | std::ios::in | std::ios::out);
        if (!offset_fs.is_open())
        {
            logger->error("File opening failed: ", std::strerror(errno));
            exit(1);
        }
    }

    if (!(std::filesystem::file_size(base_path / offsets) == 0))
    {
        // read tuples with page_id and offset if available
        u_int32_t page_id = 0, offset = 0;
        while (offset_fs.read(reinterpret_cast<char *>(&page_id), sizeof(page_id)) && offset_fs.read(reinterpret_cast<char *>(&offset), sizeof(offset)))
        {
            page_id_offset_map.emplace(page_id, offset);
        }

        // check if previous reads failed
        /* if (!offset_fs)
        {
            std::cerr << "File read failed: " << std::strerror(errno) << std::endl;
            exit(1);
        }*/
    }

    if (!(std::filesystem::file_size(base_path / data) == 0))
    {
        data_fs.seekg(0, data_fs.end);
        current_offset = data_fs.tellg();
    }
}

void StorageManager::destroy()
{
    // we need to save the offset file here
    offset_fs.close();
    offset_fs.open(base_path / offsets, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!offset_fs.is_open())
    {
        logger->error("File opening failed: ", std::strerror(errno));
        exit(1);
    }

    std::map<u_int32_t, u_int32_t>::iterator it;

    for (it = page_id_offset_map.begin(); it != page_id_offset_map.end(); it++)
    {
        offset_fs.write(reinterpret_cast<const char *>(&it->first), sizeof(it->first));
        offset_fs.write(reinterpret_cast<const char *>(&it->second), sizeof(it->second));
    }

    offset_fs.close();
    data_fs.close();
}

void StorageManager::load_page(Header *header, u_int32_t page_id)
{
    std::map<uint32_t, uint32_t>::iterator it = page_id_offset_map.find(page_id);

    if (it != page_id_offset_map.end())
    {
        uint32_t offset = it->second;
        data_fs.seekg(offset, std::ios::beg);
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
    else
    {
        logger->error("Requested page not in memory");
        exit(1);
    }
}

void StorageManager::save_page(Header *header)
{
    uint32_t page_id = header->page_id;
    std::map<uint32_t, uint32_t>::iterator it = page_id_offset_map.find(page_id);

    if (it != page_id_offset_map.end())
    {
        uint32_t offset = it->second;
        data_fs.seekp(offset, std::ios::beg);
        if (!data_fs)
        {
            logger->error("Seekp failed: ", std::strerror(errno));
            exit(1);
        }
        data_fs.write(reinterpret_cast<char *>(header), page_size);
        if (!data_fs)
        {
            logger->error("File write failed: ", std::strerror(errno));
            exit(1);
        }
        data_fs.flush();
    }
    else
    {
        page_id_offset_map.emplace(page_id, current_offset);
        data_fs.seekp(current_offset, std::ios::beg);
        if (!data_fs)
        {
            logger->error("Seekp failed: ", std::strerror(errno));
            exit(1);
        }
        data_fs.write(reinterpret_cast<char *>(header), page_size);
        if (!data_fs)
        {
            logger->error("File write failed: ", std::strerror(errno));
            exit(1);
        }
        data_fs.flush();
        current_offset += page_size;
    }
}

int StorageManager::highest_page_id()
{
    int highest_page_id = 0;

    for (auto &pair : page_id_offset_map)
    {
        if (pair.first > highest_page_id)
            highest_page_id = pair.first;
    }

    return highest_page_id;
}
