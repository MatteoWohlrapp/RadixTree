
#include "DataManager.h"
#include "../bplustree/BPlusTree.h"

DataManager::DataManager(bool cache_arg) : cache(cache_arg)
{
    logger = spdlog::get("logger");
    storage_manager = new StorageManager(base_path, Configuration::page_size);
    buffer_manager = new BufferManager(storage_manager, Configuration::max_buffer_size, Configuration::page_size);
    if (cache_arg)
    {
        logger->debug("Cache enabled");
        radixtree = new RadixTree();
    }
    bplustree = new BPlusTree<Configuration::page_size>(buffer_manager, radixtree);
}

void DataManager::destroy()
{
    buffer_manager->destroy();
    storage_manager->destroy();
    if (radixtree)
    {
        radixtree->destroy();
        delete radixtree;
    }
    delete storage_manager; 
    delete buffer_manager; 
    delete bplustree;
}

void DataManager::insert(int64_t key, int64_t value)
{
    // will be automatically added to cache if radixtree object is passed
    bplustree->insert(key, value);
}

void DataManager::delete_pair(int64_t key)
{
    // automatically deleted in bplustree
    bplustree->delete_pair(key);
}

int64_t DataManager::get_value(int64_t key)
{
    if (cache)
    {
        int64_t value = radixtree->get_value(key);
        if (value == INT64_MIN)
            return value;
    }
    return bplustree->get_value(key);
}