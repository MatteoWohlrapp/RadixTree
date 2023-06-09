
#include "BufferManager.h"
#include "../Configuration.h"
#include <iostream>
#include <stdlib.h>
#include <random>

BufferManager::BufferManager(std::shared_ptr<StorageManager> storage_manager_arg, int buffer_size_arg) : storage_manager(storage_manager_arg), buffer_size(buffer_size_arg)
{
    logger = spdlog::get("logger");
    dist = std::uniform_int_distribution<int>(0, buffer_size);
}

// TODO
void BufferManager::destroy()
{
    logger->trace("Destroying");
    for (auto &pair : page_id_map)
    {
        logger->trace("Map: {}, fixed {}, marked {}", pair.first, pair.second->fix_count, pair.second->marked);
    }
}

Header *BufferManager::request_page(uint64_t page_id)
{
    std::map<uint64_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it == page_id_map.end())
    {
        logger->debug("Page {} not in memory", page_id);
        // means the page is not in the buffer and we need to fetch it from memory
        fetch_page_from_disk(page_id);
        it = page_id_map.find(page_id);
    }
    // fix page
    logger->debug("Page id in map: {}", it->second->header.page_id);
    it->second->fix_count++;
    it->second->marked = true;
    return &it->second->header;
}

Header *BufferManager::create_new_page()
{
    Frame *frame_address;
    // check if buffer is full and then evict pages
    if (current_buffer_size >= buffer_size)
    {
        frame_address = evict_page();
    }
    else
    {
        // insert element in the buffer pool and save the index for the page id in the map
        // Frame size is page_size + 4 for the fix_count, the marker and the dirty flag
        frame_address = (Frame *)malloc(Configuration::page_size + 4);
        current_buffer_size++;
    }
    // fix page
    frame_address->fix_count = 1;
    frame_address->marked = true;
    frame_address->dirty = true;
    uint64_t page_id = storage_manager->get_unused_page_id();
    frame_address->header.page_id = page_id;
    frame_address->header.inner = false;
    page_id_map.emplace(page_id, frame_address);
    return &frame_address->header;
}

// TODO
void BufferManager::delete_page(uint64_t page_id)
{
    std::map<uint64_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        free(it->second);
    }
}

void BufferManager::fix_page(uint64_t page_id)
{
    std::map<uint64_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        it->second->fix_count++;
    }
}

void BufferManager::unfix_page(uint64_t page_id, bool dirty)
{
    std::map<uint64_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        it->second->fix_count--;
        it->second->dirty = it->second->dirty || dirty;
    }
}

Frame *BufferManager::evict_page()
{
    //  Randomly access a page from the map and evict if unmarked - if marked, set to unmark and try next page
    while (true)
    {
        int random_index = dist(rd) % current_buffer_size;
        std::map<uint64_t, Frame *>::iterator it = page_id_map.begin();
        std::advance(it, random_index);

        if (it->second->fix_count == 0)
        {
            if (it->second->marked)
            {
                it->second->marked = false;
            }
            else
            {
                logger->trace("Evicting page: {}, dirty: {}", it->first, it->second->dirty);
                if (it->second->dirty)
                {
                    storage_manager->save_page(&it->second->header);
                }
                page_id_map.erase(it->second->header.page_id);
                return it->second;
            }
        }
    }
}

void BufferManager::fetch_page_from_disk(uint64_t page_id)
{
    logger->debug("Fetching page from disk");
    Frame *frame_address;
    if (current_buffer_size >= buffer_size)
    {
        logger->debug("Buffer full, evicting page");
        frame_address = evict_page();
    }
    else
    {
        frame_address = (Frame *)malloc(Configuration::page_size + 4);
        current_buffer_size++;
    }
    frame_address->fix_count = 0;
    frame_address->dirty = false;
    storage_manager->load_page(&frame_address->header, page_id);
    assert((page_id == frame_address->header.page_id) && "Page_id requested and page_id from disk are not equal.");
    page_id_map.emplace(page_id, frame_address);
}
