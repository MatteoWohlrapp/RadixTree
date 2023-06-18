
#include "DataManager.h"
#include "../btree/BPlus.h"

DataManager::DataManager(bool cache_arg) : cache(cache_arg)
{
    logger = spdlog::get("logger");
    storage_manager = std::make_shared<StorageManager>(base_path, Configuration::page_size);
    buffer_manager = std::make_shared<BufferManager>(storage_manager, Configuration::max_buffer_size, Configuration::page_size);
    if (cache_arg)
    {
        logger->info("Cache enabled");
        rtree = new RadixTree();
    }
    btree = new BPlus<Configuration::page_size>(buffer_manager, rtree);
}

void DataManager::destroy()
{
    buffer_manager->destroy();
    storage_manager->destroy();
    delete rtree; 
    delete btree; 
}

void DataManager::insert(int64_t key, int64_t value)
{
    // will be automatically added to cache if rtree object is passed
    btree->insert(key, value);
}

void DataManager::delete_pair(int64_t key)
{
    if (cache)
        rtree->delete_reference(key);
    btree->delete_pair(key);
}

int64_t DataManager::get_value(int64_t key)
{
    if (cache)
    {
        int64_t value = rtree->get_value(key);
        if (value != INT64_MIN)
            return value;
    }
    return btree->get_value(key);
}