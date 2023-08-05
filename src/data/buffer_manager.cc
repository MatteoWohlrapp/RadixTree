
#include "buffer_manager.h"
#include "../configuration.h"
#include <iostream>
#include <stdlib.h>
#include <random>

BufferManager::BufferManager(StorageManager *storage_manager_arg, uint64_t buffer_size_arg, int page_size_arg) : storage_manager(storage_manager_arg), buffer_size(buffer_size_arg), page_size(page_size_arg)
{
    logger = spdlog::get("logger");
    dist = std::uniform_int_distribution<int>(0, buffer_size);
}

void BufferManager::destroy()
{
    for (auto &pair : page_id_map)
    {
        if (pair.second->dirty)
        {
            storage_manager->save_page(&pair.second->header);
        }
        free(pair.second);
    }
}

BHeader *BufferManager::request_page(uint64_t page_id)
{
    std::map<uint64_t, BFrame *>::iterator it = page_id_map.find(page_id);
    if (it == page_id_map.end())
    {
        // means the page is not in the buffer and we need to fetch it from memory
        fetch_page_from_disk(page_id);
        it = page_id_map.find(page_id);
    }
    // fix page
    it->second->fix_count++;
    it->second->marked = true;
    return &it->second->header;
}

BHeader *BufferManager::create_new_page()
{
    BFrame *frame_address;
    // check if buffer is full and then evict pages
    if (current_buffer_size >= buffer_size)
    {
        frame_address = evict_page();
    }
    else
    {
        // insert element in the buffer pool and save the index for the page id in the map
        // Frame size is page_size + 4 for the fix_count, the marker and the dirty flag
        frame_address = (BFrame *)malloc(page_size + 8);
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

void BufferManager::delete_page(uint64_t page_id)
{
    // storage_manager->delete_page(page_id);
    std::map<uint64_t, BFrame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        assert(it->second->fix_count == 0 && "Fix count is not zero when deleting");
        BFrame *temp = it->second;
        temp->header.page_id = 0;
        temp->marked = false;
        temp->dirty = false;
        temp->fix_count = 0;
    }
}

void BufferManager::fix_page(uint64_t page_id)
{
    std::map<uint64_t, BFrame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        assert(it->second->fix_count == 0 && "Trying to fix page that is not unfixed");
        it->second->marked = true;
        it->second->fix_count++;
    }
}

void BufferManager::unfix_page(uint64_t page_id, bool dirty)
{
    std::map<uint64_t, BFrame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        assert(it->second->fix_count == 1 && "Trying to unfix page that is not fixed with 1");
        it->second->fix_count--;
        it->second->dirty = it->second->dirty || dirty;
    }
}

BFrame *BufferManager::evict_page()
{
    //  Randomly access a page from the map and evict if unmarked - if marked, set to unmark and try next page
    while (true)
    {
        int random_index = dist(rd) % current_buffer_size;
        std::map<uint64_t, BFrame *>::iterator it = page_id_map.begin();
        std::advance(it, random_index);

        if (it->second->fix_count == 0)
        {
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
                BFrame *frame = it->second;
                page_id_map.erase(it->first);
                return frame;
            }
        }
    }
}

void BufferManager::fetch_page_from_disk(uint64_t page_id)
{
    BFrame *frame_address;
    if (current_buffer_size >= buffer_size)
    {
        frame_address = evict_page();
    }
    else
    {
        frame_address = (BFrame *)malloc(page_size + 8);
        current_buffer_size++;
    }
    frame_address->fix_count = 0;
    frame_address->dirty = false;
    storage_manager->load_page(&frame_address->header, page_id);
    assert((page_id == frame_address->header.page_id) && "Page_id requested and page_id from disk are not equal.");
    page_id_map.emplace(page_id, frame_address);
}

void BufferManager::mark_dirty(uint64_t page_id)
{
    std::map<uint64_t, BFrame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        it->second->dirty = true;
    }
}

uint64_t BufferManager::get_current_buffer_size()
{
    return current_buffer_size;
}
