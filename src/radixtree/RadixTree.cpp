
#include "RadixTree.h"

RadixTree::RadixTree()
{
    logger = spdlog::get("logger");
    // init root node
}

void RadixTree::insert(int64_t key, uint64_t page_id, BHeader *header){

}


void RadixTree::delete_reference(int64_t key){

}


int64_t RadixTree::get_value(int64_t key){
    return INT64_MIN; 
}