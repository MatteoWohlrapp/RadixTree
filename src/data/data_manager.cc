
#include "data_manager.h"
#include "../bplus_tree/bplus_tree.h"

DataManager::DataManager(bool cache_arg) : cache(cache_arg)
{
    logger = spdlog::get("logger");
    storage_manager = new StorageManager(base_path, Configuration::page_size);
    buffer_manager = new BufferManager(storage_manager, Configuration::max_buffer_size, Configuration::page_size);
    if (cache_arg)
    {
        logger->debug("Cache enabled");
        radix_tree = new RadixTree();
    }
    bplus_tree = new BPlusTree<Configuration::page_size>(buffer_manager, radix_tree);
}

void DataManager::destroy()
{
    buffer_manager->destroy();
    storage_manager->destroy();
    if (radix_tree)
    {
        radix_tree->destroy();
        delete radix_tree;
    }
    delete storage_manager; 
    delete buffer_manager; 
    delete bplus_tree;
}

void DataManager::insert(int64_t key, int64_t value)
{
    // will be automatically added to cache if radix_tree object is passed
    bplus_tree->insert(key, value);
}

void DataManager::delete_pair(int64_t key)
{
    // automatically deleted in bplustree
    bplus_tree->delete_pair(key);
}

int64_t DataManager::get_value(int64_t key)
{
    if (cache)
    {
        int64_t value = radix_tree->get_value(key);
        if (value == INT64_MIN)
            return value;
    }
    return bplus_tree->get_value(key);
}