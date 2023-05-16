#pragma once

#include "../model/Frame.h"
#include "StorageManager.h"

#include <stdint.h>
#include <vector>
#include <map>
#include <random>


class BufferManager
{

private:
    std::shared_ptr<StorageManager> storage_manager; 

    // data structure for page id mapping
    std::map<u_int32_t, Frame *> page_id_map;

    // size for one page in memory
    uint32_t page_size;
    
    // current page_id
    uint32_t page_id_count;

    // handle the size of the buffer
    uint32_t max_buffer_size = 100;
    uint32_t current_buffer_size = 0;

    // random number generator used for eviction of pages
    std::random_device rd;
    std::uniform_int_distribution<int> dist; 

    void fetch_page_from_disk(uint32_t page_id);

    Frame* evict_page();

public:
    BufferManager(std::shared_ptr<StorageManager> storage_manager_arg, int page_size_arg = 4096, int page_id = 1);

    Header *request_page(uint32_t page_id);

    Header *create_new_page();

    void delete_page(uint32_t page_id);

    void fix_page(uint32_t page_id);

    void unfix_page(uint32_t page_id);

    void mark_diry(uint32_t page_id); 
};