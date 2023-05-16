#pragma once

#include "BufferManager.h"
#include "StorageManager.h"

class DBManager
{
private:
    const int page_size;
    std::filesystem::path base_path = "./db"; 


public:
    std::shared_ptr<StorageManager> storage_manager;
    std::shared_ptr<BufferManager> buffer_manager;

    DBManager(const int page_size_arg) : page_size(page_size_arg)
    {
        storage_manager = std::shared_ptr<StorageManager>(new StorageManager(base_path, page_size));
        buffer_manager = std::shared_ptr<BufferManager>(new BufferManager(storage_manager, page_size, storage_manager->highest_page_id()));
    }

    ~DBManager(){
        storage_manager->~StorageManager(); 
    }
};