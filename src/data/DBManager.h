/**
 * @file    DBManager.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "BufferManager.h"
#include "StorageManager.h"
#include "../Configuration.h"

/**
 * @brief Makes sure that the DB is initialized correctly and hold all variables necessary to run the system
 */
class DBManager
{
private:
    /// path were files related to the DB should be saved
    std::filesystem::path base_path = "./db";

public:
    std::shared_ptr<StorageManager> storage_manager;
    std::shared_ptr<BufferManager> buffer_manager;

    /**
     * @brief Constructor for the DBManager
     */
    DBManager()
    {
        storage_manager = std::shared_ptr<StorageManager>(new StorageManager(base_path, Configuration::page_size));
        buffer_manager = std::shared_ptr<BufferManager>(new BufferManager(storage_manager, storage_manager->highest_page_id()));
    }
};