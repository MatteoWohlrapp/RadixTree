
#include "data_manager.h"
#include "../bplus_tree/bplus_tree.h"

DataManager::DataManager(int buffer_size_arg, bool cache_arg, int radix_tree_size_arg)
{
    logger = spdlog::get("logger");
    storage_manager = new StorageManager(base_path, Configuration::page_size);
    buffer_manager = new BufferManager(storage_manager, buffer_size_arg, Configuration::page_size);
    if (cache_arg)
    {
        logger->debug("Cache enabled");
        radix_tree = new RadixTree<Configuration::page_size>(radix_tree_size_arg);
    }
    bplus_tree = new BPlusTree<Configuration::page_size>(buffer_manager, radix_tree);
}

void DataManager::destroy()
{
    buffer_manager->destroy();
    storage_manager->destroy();
    if (radix_tree)
    {
        logger->info("Destroying radix tree");
        logger->flush();
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

void DataManager::delete_value(int64_t key)
{
    // automatically deleted in bplustree
    bplus_tree->delete_value(key);
}

int64_t DataManager::get_value(int64_t key)
{
    if (radix_tree)
    {
        int64_t value = radix_tree->get_value(key);
        if (value != INT64_MIN)
            return value;
    }
    return bplus_tree->get_value(key);
}

int64_t DataManager::scan(int64_t key, int range)
{
    return bplus_tree->scan(key, range);
}

void DataManager::update(int64_t key, int64_t value)
{
    bplus_tree->update(key, value);
}

bool DataManager::validate(int num_elements)
{
    std::cout << "Validating b+ tree: " << bplus_tree->validate(num_elements) << std::endl;
    if (radix_tree)
    {   
        bool validated = radix_tree->valdidate(); 
        std::cout << "Validating radix tree: " << validated << std::endl;
    }
    return true;
}
