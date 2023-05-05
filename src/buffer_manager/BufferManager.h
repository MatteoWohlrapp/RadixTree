#include <stdint.h>
#include <vector>
#include <map>

#include "../data/Frame.h"
#ifndef BUFFER_MANAGER
#define BUFFER_MANAGER

class BufferManager{

private: 
    //data structure for page id mapping
    std::map<u_int32_t, Frame*> page_id_map; 

    // current page_id
    uint32_t page_id_count; 

    // handle the size of the buffer
    uint32_t max_buffer_size = 100; 
    uint32_t current_buffer_size = 0; 

    // size for one page in memory
    uint32_t page_size; 

    void fetch_page_from_disk(uint32_t page_id); 

    void evict_page(); 

public: 
    BufferManager(int page_size_arg = 4096); 

    Header* request_page(uint32_t page_id); 

    Header* create_new_page(); 

    void delete_page(uint32_t page_id); 

    void fix_page(uint32_t page_id); 

    void unfix_page(uint32_t page_id);
};

#endif