/**
 * @file    tree_operations.h
 *
 * @author  Matteo Wohlrapp
 * @date    23.07.2023
 */

#pragma once

#include "../bplus_tree/b_nodes.h"
#include "../radix_tree/radix_tree.h"
#include "../data/buffer_manager.h"

// forward declaration
template <int PAGE_SIZE>
class RadixTree;

/**
 * @brief namespace that handles tree operations
 */
namespace TreeOperations
{
    /**
     * @brief Start at element key and get range consecutive elements
     * @param buffer_manager The buffer manager
     * @param cache A reference to the cache
     * @param header The pointer to the current node
     * @param key The key corresponding to a value
     * @param range The number of elements that are scanned
     * @return The sum of the elements
     */
    template <int PAGE_SIZE>
    inline int64_t scan(BufferManager *buffer_manager, RadixTree<PAGE_SIZE> *cache, BHeader *header, int64_t key, int range)
    {
        BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;

        int index = node->binary_search(key);
        int scanned = 0;
        int64_t sum = 0;

        assert((node->keys[index] == key) && "Scan on key that does not exist.");
        if (node->keys[index] == key)
        {
            if (cache)
            {
                cache->insert(key, header->page_id, header);
            }
            while (scanned < range)
            {
                if (index == node->current_index)
                {
                    if (node->next_lef_id == 0)
                    {
                        buffer_manager->unfix_page(node->header.page_id, false);
                        if (sum == INT64_MIN)
                            return INT64_MIN + 1;
                        else
                            return sum;
                    }
                    BOuterNode<PAGE_SIZE> *temp = node;
                    node = (BOuterNode<PAGE_SIZE> *)buffer_manager->request_page(node->next_lef_id);
                    buffer_manager->unfix_page(temp->header.page_id, false);
                    index = 0;
                }

                sum ^= node->values[index];
                scanned++;
                index++;
            }
            buffer_manager->unfix_page(node->header.page_id, false);

            if (sum == INT64_MIN)
                return INT64_MIN + 1;
            else
                return sum;
        }
        return INT64_MIN;
    }
}