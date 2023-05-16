
#include "BufferManager.h"
#include <iostream>
#include <stdlib.h>
#include <random>

BufferManager::BufferManager(std::shared_ptr<StorageManager> storage_manager_arg, const int page_size_arg, int highest_page_id) : storage_manager(storage_manager_arg), page_size(page_size_arg), page_id_count(highest_page_id + 1)
{
    logger = spdlog::get("logger");
    page_id_count = 1;
    dist = std::uniform_int_distribution<int>(0, max_buffer_size);
}

Header *BufferManager::request_page(u_int32_t page_id)
{
    std::map<uint32_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it == page_id_map.end())
    {
        // means the page is not in the buffer and we need to fetch it from memory
        fetch_page_from_disk(page_id);
        it = page_id_map.find(page_id);
    }
    fix_page(page_id);
    it->second->marked = true;
    return &it->second->header;
}

Header *BufferManager::create_new_page()
{
    // check if buffer is full and then evict pages
    if (current_buffer_size >= max_buffer_size)
    {
        evict_page();
    }
    else
    {
        current_buffer_size++;
    }
    // insert element in the buffer pool and save the index for the page id in the map
    // Frame size is page_size + 4 for the fix_count, the marker and the dirty flag
    Frame *frame_address = (Frame *)malloc(page_size + 4);
    frame_address->fix_count = 0;
    frame_address->marked = true;
    frame_address->header.page_id = page_id_count;
    frame_address->header.inner = false;
    page_id_map.emplace(page_id_count, frame_address);
    // page is fixed as it will likely be used
    fix_page(page_id_count);
    page_id_count++;
    return &frame_address->header;
}

// TODO, what happens when deleting from memory
void BufferManager::delete_page(u_int32_t page_id)
{
    std::map<uint32_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        free(it->second);
    }
}

void BufferManager::fix_page(uint32_t page_id)
{
    std::map<uint32_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        it->second->fix_count++;
    }
}

void BufferManager::unfix_page(uint32_t page_id)
{
    std::map<uint32_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        it->second->fix_count--;
    }
}

void BufferManager::mark_diry(uint32_t page_id)
{
    std::map<uint32_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        it->second->dirty = true;
    }
}

Frame *BufferManager::evict_page()
{
    //  Randomly access a page from the map and evict if unmarked - if marked, set to unmark and try next page
    logger->info("Evicting page");

    while (true)
    {
        int random_index = dist(rd) % current_buffer_size;
        std::map<uint32_t, Frame *>::iterator it = page_id_map.begin();
        std::advance(it, random_index);

        if (it->second->marked)
        {
            it->second->marked = false;
        }
        else
        {
            if (it->second->dirty)
            {
                storage_manager->save_page(&it->second->header);
            }
            page_id_map.erase(it->second->header.page_id);
            return it->second;
        }
    }
}

void BufferManager::fetch_page_from_disk(uint32_t page_id)
{
    logger->info("Fetching page from disk");
    if (current_buffer_size > max_buffer_size)
    {
        logger->info("Buffer full, evicting page");
        Frame *frame_address = evict_page();
        storage_manager->load_page(&frame_address->header, page_id);
        page_id_map.emplace(page_id, frame_address);
    }
    else
    {
        Frame *frame_address = (Frame *)malloc(page_size + 4);
        frame_address->fix_count = 0;
        frame_address->dirty = false;
        storage_manager->load_page(&frame_address->header, page_id);
        page_id_map.emplace(page_id_count, frame_address);
        current_buffer_size++;
    }
}
