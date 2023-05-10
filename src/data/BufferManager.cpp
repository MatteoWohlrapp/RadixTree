
#include "BufferManager.h"
#include <iostream>
#include <stdlib.h>

BufferManager::BufferManager(int page_size_arg) : page_size(page_size_arg)
{
    page_id_count = 1;
}

Header *BufferManager::request_page(u_int32_t page_id)
{
    std::map<u_int32_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it == page_id_map.end())
    {
        // means the page is not in the buffer and we need to fetch it from memory
        fetch_page_from_disk(page_id);
        it = page_id_map.find(page_id);
    }
    return &it->second->header;
}

Header *BufferManager::create_new_page()
{
    // check if buffer is full and then evict pages
    if (current_buffer_size >= max_buffer_size)
    {
        evict_page();
    }
    // insert element in the buffer pool and save the index for the page id in the map
    // Frame size is page_size + 2 for the fix_count and the dirty flag
    Frame *frame_address = (Frame *)malloc(page_size + 2);
    frame_address->fix_count = 0;
    frame_address->dirty = true;
    frame_address->header.page_id = page_id_count;
    frame_address->header.inner = false;
    page_id_map.emplace(page_id_count, frame_address);
    // page is fixed as it will likely be used
    fix_page(page_id_count);
    page_id_count++;
    return &frame_address->header;
}

void BufferManager::delete_page(u_int32_t page_id)
{
    std::map<u_int32_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        free(it->second);
    }
}

// TODO wait when fixing
void BufferManager::fix_page(uint32_t page_id)
{
    std::map<u_int32_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        it->second->fix_count++;
    }
}

void BufferManager::unfix_page(uint32_t page_id)
{
    std::map<u_int32_t, Frame *>::iterator it = page_id_map.find(page_id);
    if (it != page_id_map.end())
    {
        it->second->fix_count--;
    }
}

void BufferManager::evict_page()
{
    // TODO
    //  Randomly access a page from the map and evict if unmarked - if marked, set to unmark and try next page
    std::cout << "Evicting page" << std::endl;
    exit(1);
}

void BufferManager::fetch_page_from_disk(uint32_t page_id)
{
    // TODO
    if (current_buffer_size > max_buffer_size)
    {
        evict_page();
    }
    std::cout << "Fetching page from disk" << std::endl;
    exit(1);
}
