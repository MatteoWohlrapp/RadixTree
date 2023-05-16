#pragma once

#include "BufferManager.h"
#include "StorageManager.h"

class DBManager
{
private:
    int node_size;
    std::filesystem::path base_path = "../db"; 


public:
    std::shared_ptr<StorageManager> storage_manager;
    std::shared_ptr<BufferManager> buffer_manager;

    DBManager(int node_size_arg) : node_size(node_size_arg)
    {
        storage_manager = std::shared_ptr<StorageManager>(new StorageManager(base_path));
        buffer_manager = std::shared_ptr<BufferManager>(new BufferManager(storage_manager, node_size, storage_manager->highest_page_id()));
    }

    ~DBManager(){
        storage_manager->~StorageManager(); 
    }
};