
#include <array>
#include <math.h>
#include <iostream>
#include <cassert>
#include "../buffer_manager/BufferManager.h"

#ifndef BPLUS
#define BPLUS

// ((PAGE_SIZE-16)/2)/4 and (PAGE_SIZE-20)/4 must both be integers
// 48 works for a degree of 4 in the inner nodes, 7 in the outer nodes
const int NODE_SIZE = 52; 

template <int PAGE_SIZE>
struct InnerNode {
    // 8 bytes
    Header header; 
    // 4 bytes
    int current_index; 
    // 4 bytes
    int max_size;
    // 4 bytes padding to make it the same size as outer node
    char padding[4];
    // hardcoded keys as int for now, max size will be made one shorter, so we can 
    int keys[((PAGE_SIZE-20)/2)/4]; 
    uint32_t child_ids[((PAGE_SIZE-20)/2)/4]; 

    InnerNode(Header* header_arg): header(*new (header_arg) Header(header_arg)){
        current_index = 0; 
        max_size = ((PAGE_SIZE-20)/2)/4 - 1; 
        assert(max_size > 2 && "Node size is too small"); 
        header.inner = true; 
    }

    // gives the next page
    uint32_t next_page(int key){
        int index = 0; 
         while (index < current_index) {
            // If key is less than or equal to the current key, return child_id
            if (key <= keys[index]) {
                return child_ids[index];
            }
            index++;
        }
        return child_ids[index]; 
    }

    // the child id will be inserted on the right side of the key, as this will always correspond to the created node after splitting which is the ones with higher values
    void insert(uint32_t child_id, int key){
        assert(!is_full() && "Inserting into inner node when its full."); 
        // find index where to insert
        int index = 0; 

        while(index < current_index){
            if(keys[index] < key){
                index++; 
            } else {
                break; 
            }
        }
        // shift all keys bigger one space to the left
        for(int i = current_index; i > index; i--){
            keys[i] = keys[i-1]; 
            child_ids[i+1] = child_ids[i]; 
        }
        
        
        // insert new values
        keys[index] = key; 
        child_ids[index+1] = child_id;
        current_index++; 
    }

    bool is_full(){
        return current_index >= max_size; 
    }
}; 

template <int PAGE_SIZE>
struct OuterNode{
    // 8 bytes
    Header header; 
    // 4 bytes
    int current_index; 
    // 4 bytes 
    int max_size;
    // 4 bytes
    uint32_t next_lef_id; 
    
    int keys[((PAGE_SIZE-20)/4)/2]; 
    int values[((PAGE_SIZE-20)/4)/2]; 


    OuterNode(Header* header_arg): header(*new (header_arg) Header(header_arg)){
        current_index = 0; 
        max_size = ((PAGE_SIZE-20)/4)/2; 
        assert(max_size > 2 && "Node size is too small"); 
        next_lef_id = 0; 
        header.inner = false; 
    }

    // we only insert if there is still space left
    void insert(int key, int value){
        assert(!is_full() && "Inserting into outer node when its full."); 
        // find index where to insert
        int index = 0;
        while(index < current_index){
            if(keys[index] < key){
                index++; 
            } else {
                break; 
            }
        }

        // shift all keys bigger one space to the left
        for(int i = current_index; i > index; i--){
            keys[i] = keys[i-1]; 
            values[i] = values[i-1]; 
        }
        
        // insert new values
        keys[index] = key; 
        values[index] = value;
        current_index++; 
    } 

    int get_value(int key){
        int value = -1; 
        int index = 0; 
        while(index < current_index){
            if(values[index] == key){
                value = values[index]; 
                break; 
            }
            index++; 
        }
        return value; 
    }

    bool is_full(){
        return current_index >= max_size; 
    }
};

class BPlus{
private: 
    std::shared_ptr<BufferManager> buffer_manager; 

    void recursive_insert(uint32_t page_id, int key, int value);

    int recursive_get_value(uint32_t page_id, int key); 

    uint32_t split_outer_node(uint32_t page_id, int index_to_split); 

    uint32_t split_inner_node(uint32_t page_id, int index_to_split); 

    int get_split_index(int max_size); 

public: 
    // public for debugger
    Header* root; 

    BPlus(std::shared_ptr<BufferManager> buffer_manager_arg); 

    void insert(int key, int value); 

    int get_value(int key); 
}; 

#endif