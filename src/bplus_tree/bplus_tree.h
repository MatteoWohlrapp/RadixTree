/**
 * @file    bplus_tree.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "../data/buffer_manager.h"
#include "../radix_tree/radix_tree.h"
#include "b_nodes.h"
#include <array>
#include <math.h>
#include <iostream>
#include <cassert>
#include "spdlog/spdlog.h"
#include "../utils/tree_operations.h"

/// forward declaration
class BPlusTreeTest;
class Debuger;

/**
 * @brief Implements the B+ Tree of the Database
 */
template <int PAGE_SIZE>
class BPlusTree
{
private:
    std::shared_ptr<spdlog::logger> logger;

    BufferManager *buffer_manager;

    RadixTree<PAGE_SIZE> *cache;

    /// root of tree
    uint64_t root_id = 0;

    /**
     * @brief Inserts recursively into the tree
     * @param header The header of the current node
     * @param key The key to insert
     * @param value The value to insert
     */
    void recursive_insert(BHeader *header, int64_t key, int64_t value)
    {
        logger->debug("Recursive insert into page: {}", header->page_id);
        if (!header->inner)
        {
            // outer node
            BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;
            // case where the tree only has one outer node
            if (root_id == node->header.page_id && node->is_full())
            {
                // only when outer leaf is full, otherwise we will split node before
                // split outer node and save key
                int split_index = get_split_index(node->max_size);
                // Because the node size has a lower limit, this does not cause issues
                int64_t split_key = node->keys[split_index - 1];
                uint64_t new_outer_id = split_outer_node(header, split_index);
                logger->debug("Splitting outer root node, new outer id: {}", new_outer_id);

                // create new inner node for root
                BHeader *new_root_node_address = buffer_manager->create_new_page();
                BInnerNode<PAGE_SIZE> *new_root_node = new (new_root_node_address) BInnerNode<PAGE_SIZE>();

                new_root_node->child_ids[0] = node->header.page_id;
                new_root_node->insert(split_key, new_outer_id);

                root_id = new_root_node->header.page_id;

                // root node already fixed when requesting, so now only need to unfix current
                buffer_manager->unfix_page(header->page_id, true);
                // insert into new root
                recursive_insert(new_root_node_address, key, value);
            }
            else
            {
                logger->debug("Inserting into outer node");
                node->insert(key, value);
                if (cache)
                {
                    cache->insert(key, header->page_id, header);
                }
                // finished inserting, so page can be unfixed
                buffer_manager->unfix_page(header->page_id, true);
            }
        }
        else
        {
            // Inner node
            BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;

            if (node->header.page_id == root_id && node->is_full())
            {
                // current inner node is root
                int split_index = get_split_index(node->max_size);
                // Because the node size has a lower limit, this does not cause issues
                int64_t split_key = node->keys[split_index - 1];
                uint64_t new_inner_id = split_inner_node(header, split_index);
                logger->debug("Splitting inner root node, new inner id: {}", new_inner_id);

                // create new inner node for root
                BHeader *new_root_node_address = buffer_manager->create_new_page();
                BInnerNode<PAGE_SIZE> *new_root_node = new (new_root_node_address) BInnerNode<PAGE_SIZE>();

                new_root_node->child_ids[0] = node->header.page_id;
                new_root_node->insert(split_key, new_inner_id);

                root_id = new_root_node->header.page_id;

                // root node already fixed when requesting, so now only need to unfix current
                buffer_manager->unfix_page(header->page_id, true);
                // insert into new root

                recursive_insert(new_root_node_address, key, value);
            }
            else
            {

                // find next page to insert
                uint64_t next_page = node->next_page(key);
                BHeader *child_header = buffer_manager->request_page(next_page);
                logger->debug("Finding child to insert: {}", next_page);

                if (child_header->inner)
                {
                    BInnerNode<PAGE_SIZE> *child = (BInnerNode<PAGE_SIZE> *)child_header;
                    if (child->is_full())
                    {
                        logger->debug("Child is inner and full");
                        // split the inner node
                        int split_index = get_split_index(child->max_size);
                        // Because the node size has a lower limit, this does not cause issues
                        int64_t split_key = child->keys[split_index - 1];
                        uint64_t new_inner_id = split_inner_node(child_header, split_index);
                        logger->debug("New inner child id: {}", new_inner_id);

                        node->insert(split_key, new_inner_id);

                        // unfix previous child which could have been changed due to splitting
                        buffer_manager->unfix_page(next_page, true);
                        // find correct new child and fix it, then unfix current child
                        next_page = node->next_page(key);
                        child_header = buffer_manager->request_page(next_page);
                        // unfix the current node after fixing the correct child
                        buffer_manager->unfix_page(header->page_id, true);

                        recursive_insert(child_header, key, value);
                    }
                    else
                    {
                        logger->debug("Child is inner and not full");
                        // child correctly fixed before, just need to unfix current node
                        buffer_manager->unfix_page(header->page_id, false);
                        recursive_insert(child_header, key, value);
                    }
                }
                else
                {
                    BOuterNode<PAGE_SIZE> *child = (BOuterNode<PAGE_SIZE> *)child_header;

                    if (child->is_full())
                    {
                        logger->debug("Child is outer and full");
                        // split the outer node
                        int split_index = get_split_index(child->max_size);
                        // Because the node size has a lower limit, this does not cause issues
                        int64_t split_key = child->keys[split_index - 1];
                        uint64_t new_outer_id = split_outer_node(child_header, split_index);
                        logger->debug("New outer child id: {}", new_outer_id);

                        // Insert new child and then call function again
                        node->insert(split_key, new_outer_id);

                        // unfix previous child which could have been changed due to splitting
                        buffer_manager->unfix_page(next_page, true);
                        // find correct new child and fix it, then unfix current child
                        next_page = node->next_page(key);
                        child_header = buffer_manager->request_page(next_page);
                        // unfix the current node after fixing the correct child
                        buffer_manager->unfix_page(header->page_id, true);

                        recursive_insert(child_header, key, value);
                    }
                    else
                    {
                        logger->debug("Child is outer and not full");
                        // child correctly fixed before, just need to unfix current node
                        buffer_manager->unfix_page(header->page_id, false);
                        recursive_insert(child_header, key, value);
                    }
                }
            }
        }
    }

    /**
     * @brief Delete recursively from the tree
     * @param header The header of the current node
     * @param key The key to delete
     */
    void recursive_delete(BHeader *header, int64_t key)
    {
        logger->debug("Deleting key {}", key);
        if (!header->inner)
        {
            // outer node
            BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;
            node->delete_value(key);
            if (cache)
                cache->delete_reference(key);
            buffer_manager->unfix_page(header->page_id, true);
        }
        else
        {
            // Inner node
            BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;

            if (header->page_id == root_id && node->current_index == 0)
            {
                // only happens for inner node after delete
                // no key left, just child ids still has an entry at position 0
                root_id = node->child_ids[0];
                buffer_manager->unfix_page(header->page_id, false);
                buffer_manager->delete_page(header->page_id);
                delete_value(key);
            }
            else
            {
                // find next page to delete from
                uint64_t next_page = node->next_page(key);
                BHeader *child_header = buffer_manager->request_page(next_page);

                if (child_header->inner)
                {
                    BInnerNode<PAGE_SIZE> *child = (BInnerNode<PAGE_SIZE> *)child_header;

                    if (!child->can_delete())
                    {
                        logger->debug("Cant delete inner child");
                        /**
                         * 1. Substitute with left or right if you can
                         * 2. If not, merge with left or right
                         * -> no need to replace key in current node because with both substitution and merge, key will either change or be deleted
                         */
                        if (substitute(header, child_header))
                        {
                            buffer_manager->unfix_page(child_header->page_id, true);
                            buffer_manager->mark_dirty(header->page_id);
                            recursive_delete(header, key);
                        }
                        else
                        {
                            // merge child with adjacent node
                            merge(header, child_header);

                            // child will be unfixed after the merge, current node can stay fixed
                            buffer_manager->mark_dirty(header->page_id);
                            recursive_delete(header, key);
                        }
                    }
                    else
                    {
                        // function checks if key is contained and if it is, exchanges it with biggest smaller key
                        bool dirty = false;
                        if (node->contains(key))
                        {
                            node->exchange(key, find_biggest(child_header));
                            child_header = buffer_manager->request_page(next_page);
                            dirty = true;
                        }

                        buffer_manager->unfix_page(header->page_id, dirty);
                        recursive_delete(child_header, key);
                    }
                }
                else
                {
                    BOuterNode<PAGE_SIZE> *child = (BOuterNode<PAGE_SIZE> *)child_header;

                    if (!child->can_delete())
                    {
                        /**
                         * 1. Substitute with left or right if you can
                         * 2. If not, merge with left or right
                         * -> no need to replace key in current node because with both substitution and merge, key will either change or be deleted
                         */
                        if (substitute(header, child_header))
                        {
                            buffer_manager->unfix_page(child_header->page_id, true);
                            buffer_manager->mark_dirty(header->page_id);
                            recursive_delete(header, key);
                        }
                        else
                        {
                            // merge child with adjacent node
                            merge(header, child_header);

                            // child will be unfixed after the merge, current node can stay fixed
                            buffer_manager->mark_dirty(header->page_id);
                            recursive_delete(header, key);
                        }
                    }
                    else
                    {
                        // function checks if key is contained and if it is, exchanges it with biggest smaller key
                        bool dirty = false;
                        if (node->contains(key))
                        {
                            node->exchange(key, find_biggest(child_header));
                            child_header = buffer_manager->request_page(next_page);
                            dirty = true;
                        }

                        buffer_manager->unfix_page(header->page_id, dirty);
                        recursive_delete(child_header, key);
                    }
                }
            }
        }
    }

    /**
     * @brief Get the value to a key recursively
     * @param header The header of the current node
     * @param key The key corresponding to the value
     */
    int64_t recursive_get_value(BHeader *header, int64_t key)
    {
        if (!header->inner)
        {
            BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;
            int64_t value = node->get_value(key);
            if (cache)
            {
                if (value != INT64_MIN)
                    cache->insert(key, header->page_id, header);
            }
            buffer_manager->unfix_page(header->page_id, false);
            return value;
        }
        else
        {
            BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;
            BHeader *child_header = buffer_manager->request_page(node->next_page(key));
            buffer_manager->unfix_page(header->page_id, false);
            return recursive_get_value(child_header, key);
        }
    }

    /**
     * @brief Splits the outer node and copies values
     * @param header The header of the current node
     * @param index_to_split The index where the node needs to be split
     * @return The page_id of the new node containing the higher elements
     */
    uint64_t split_outer_node(BHeader *header, int index_to_split)
    {
        assert(!header->inner && "Splitting node which is not an outer node");

        BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;
        assert(index_to_split < node->max_size && "Splitting at an index which is bigger than the size");

        // Create new outer node
        BHeader *new_header = buffer_manager->create_new_page();
        BOuterNode<PAGE_SIZE> *new_outer_node = new (new_header) BOuterNode<PAGE_SIZE>();

        // It is important that index_to_split is already increases by 2
        if (cache)
        {
            cache->update_range(node->keys[index_to_split], node->keys[node->current_index - 1], new_header->page_id, new_header);
        }

        for (int i = index_to_split; i < node->max_size; i++)
        {
            new_outer_node->insert(node->keys[i], node->values[i]);
            node->current_index--;
        }

        // set correct chaining
        u_int64_t next_temp = node->next_lef_id;
        node->next_lef_id = new_outer_node->header.page_id;
        new_outer_node->next_lef_id = next_temp;

        buffer_manager->unfix_page(new_header->page_id, true);

        return new_outer_node->header.page_id;
    }

    /**
     * @brief Splits the inner node and copies values
     * @param header The header of the current node
     * @param index_to_split The index where the node needs to be split
     * @return The page_id of the new node containing the higher elements
     */
    uint64_t split_inner_node(BHeader *header, int index_to_split)
    {
        assert(header->inner && "Splitting node which is not an inner node");

        BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;
        assert(index_to_split < node->max_size && "Splitting at an index which is bigger than the size");

        // Create new inner node
        BHeader *new_header = buffer_manager->create_new_page();
        BInnerNode<PAGE_SIZE> *new_inner_node = new (new_header) BInnerNode<PAGE_SIZE>();

        new_inner_node->child_ids[0] = node->child_ids[index_to_split];
        for (int i = index_to_split; i < node->max_size; i++)
        {
            new_inner_node->insert(node->keys[i], node->child_ids[i + 1]);
            node->current_index--;
        }
        node->current_index--;

        // unfixing the new page as we finished writing
        buffer_manager->unfix_page(new_header->page_id, true);

        return new_inner_node->header.page_id;
    }

    /**
     * @brief Returns the index where to split depending on the size
     * @param max_size The maximum capacity of the node
     * @return The index where to split the node
     */
    int get_split_index(int max_size)
    {
        if (max_size % 2 == 0)
            return max_size / 2;
        else
            return max_size / 2 + 1;
    }

    /**
     * @brief Substitutes one element from the left or right side of the node on the same level
     * @param header The parent node of which an element will be deleted
     * @param child_header The node which needs substitution to be able to delete
     * @return wether substitution was possible or not
     */
    bool substitute(BHeader *header, BHeader *child_header)
    {
        BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;

        if (child_header->inner)
        {
            BInnerNode<PAGE_SIZE> *child = (BInnerNode<PAGE_SIZE> *)child_header;
            BInnerNode<PAGE_SIZE> *substitute;
            int index = 0;

            while (index <= node->current_index)
            {
                if (node->child_ids[index] == child_header->page_id)
                {
                    if (index > 0)
                    {
                        substitute = (BInnerNode<PAGE_SIZE> *)buffer_manager->request_page(node->child_ids[index - 1]);
                        if (substitute->can_delete())
                        {
                            child->insert_first(node->keys[index - 1], substitute->child_ids[substitute->current_index]);
                            node->keys[index - 1] = substitute->keys[substitute->current_index - 1];

                            substitute->delete_value(substitute->keys[substitute->current_index - 1]);
                            buffer_manager->unfix_page(node->child_ids[index - 1], true);
                            return true;
                        }
                        else
                        {
                            buffer_manager->unfix_page(node->child_ids[index - 1], false);
                        }
                    }

                    if (index < node->current_index)
                    {
                        substitute = (BInnerNode<PAGE_SIZE> *)buffer_manager->request_page(node->child_ids[index + 1]);
                        if (substitute->can_delete())
                        {
                            child->insert(node->keys[index], substitute->child_ids[0]);
                            node->keys[index] = substitute->keys[0];
                            substitute->delete_first_pair();
                            buffer_manager->unfix_page(node->child_ids[index + 1], true);
                            return true;
                        }
                        else
                        {
                            buffer_manager->unfix_page(node->child_ids[index + 1], false);
                        }
                    }
                    return false;
                }
                index++;
            }
            return false;
        }
        else
        {
            BOuterNode<PAGE_SIZE> *child = (BOuterNode<PAGE_SIZE> *)child_header;
            BOuterNode<PAGE_SIZE> *substitute;
            int index = 0;

            while (index <= node->current_index)
            {
                if (node->child_ids[index] == child_header->page_id)
                {
                    if (index > 0)
                    {
                        substitute = (BOuterNode<PAGE_SIZE> *)buffer_manager->request_page(node->child_ids[index - 1]);
                        if (substitute->can_delete())
                        {
                            // save biggest key and value from left in right node
                            child->insert(substitute->keys[substitute->current_index - 1], substitute->values[substitute->current_index - 1]);
                            if (cache)
                                cache->insert(substitute->keys[substitute->current_index - 1], child_header->page_id, child_header);
                            substitute->delete_value(substitute->keys[substitute->current_index - 1]);
                            node->keys[index - 1] = substitute->keys[substitute->current_index - 1];

                            // unfix
                            buffer_manager->unfix_page(node->child_ids[index - 1], true);

                            // substitution worked
                            return true;
                        }
                        else
                        {
                            buffer_manager->unfix_page(node->child_ids[index - 1], false);
                        }
                    }
                    if (index < node->current_index)
                    {
                        substitute = (BOuterNode<PAGE_SIZE> *)buffer_manager->request_page(node->child_ids[index + 1]);
                        if (substitute->can_delete())
                        {
                            // save biggest key and value from right to left
                            child->insert(substitute->keys[0], substitute->values[0]);
                            if (cache)
                                cache->insert(substitute->keys[0], child_header->page_id, child_header);
                            node->keys[index] = substitute->keys[0];
                            substitute->delete_value(substitute->keys[0]);

                            // unfix
                            buffer_manager->unfix_page(node->child_ids[index + 1], true);

                            // substitution worked
                            return true;
                        }
                        else
                        {
                            buffer_manager->unfix_page(node->child_ids[index + 1], false);
                        }
                    }
                    return false;
                }
                index++;
            }
            return false;
        }
    }

    /**
     * @brief Merges node with left or right node
     * @param header The parent node of the node that will be merged
     * @param child_header The node that will be merged
     */
    void merge(BHeader *header, BHeader *child_header)
    {
        BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;

        if (child_header->inner)
        {
            BInnerNode<PAGE_SIZE> *child = (BInnerNode<PAGE_SIZE> *)child_header;
            BInnerNode<PAGE_SIZE> *merge;
            int index = 0;

            while (index <= node->current_index)
            {
                if (node->child_ids[index] == child_header->page_id)
                {
                    if (index > 0)
                    {
                        merge = (BInnerNode<PAGE_SIZE> *)buffer_manager->request_page(node->child_ids[index - 1]);
                        if (!merge->can_delete())
                        {
                            // add all from right node to left node
                            merge->insert(node->keys[index - 1], child->child_ids[0]);

                            for (int i = 0; i < child->current_index; i++)
                            {
                                merge->insert(child->keys[i], child->child_ids[i + 1]);
                            }
                            node->delete_value(node->keys[index - 1]);
                            buffer_manager->unfix_page(child->header.page_id, false);
                            buffer_manager->delete_page(child->header.page_id);

                            // unfix
                            buffer_manager->unfix_page(merge->header.page_id, true);

                            return;
                        }
                        else
                        {
                            buffer_manager->unfix_page(merge->header.page_id, false);
                        }
                    }
                    if (index < node->current_index)
                    {
                        merge = (BInnerNode<PAGE_SIZE> *)buffer_manager->request_page(node->child_ids[index + 1]);
                        if (!merge->can_delete())
                        {
                            child->insert(node->keys[index], merge->child_ids[0]);
                            for (int i = 0; i < merge->current_index; i++)
                            {
                                child->insert(merge->keys[i], merge->child_ids[i + 1]);
                            }

                            node->delete_value(node->keys[index]);

                            buffer_manager->unfix_page(merge->header.page_id, false);
                            buffer_manager->delete_page(merge->header.page_id);

                            // unfix
                            buffer_manager->unfix_page(child->header.page_id, true);

                            return;
                        }
                        else
                        {
                            buffer_manager->unfix_page(merge->header.page_id, false);
                        }
                    }
                }
                index++;
            }
        }
        else
        {
            BOuterNode<PAGE_SIZE> *child = (BOuterNode<PAGE_SIZE> *)child_header;
            BOuterNode<PAGE_SIZE> *merge;
            BHeader *merge_header;
            int index = 0;

            while (index <= node->current_index)
            {
                if (node->child_ids[index] == child_header->page_id)
                {
                    if (index > 0)
                    {
                        merge_header = buffer_manager->request_page(node->child_ids[index - 1]);
                        merge = (BOuterNode<PAGE_SIZE> *)merge_header;
                        if (!merge->can_delete())
                        {
                            // add all from right node to left node
                            if (cache)
                            {
                                cache->update_range(child->keys[0], child->keys[child->current_index - 1], merge_header->page_id, merge_header);
                            }

                            logger->debug("Deleting page {}", (void *)&child->header);
                            for (int i = 0; i < child->current_index; i++)
                            {
                                merge->insert(child->keys[i], child->values[i]);
                            }

                            merge->next_lef_id = child->next_lef_id;
                            node->delete_value(node->keys[index - 1]);
                            buffer_manager->unfix_page(child->header.page_id, false);
                            buffer_manager->delete_page(child->header.page_id);

                            // unfix
                            buffer_manager->unfix_page(merge->header.page_id, true);

                            return;
                        }
                        else
                        {
                            buffer_manager->unfix_page(merge->header.page_id, false);
                        }
                    }
                    if (index < node->current_index)
                    {
                        merge_header = buffer_manager->request_page(node->child_ids[index + 1]);
                        merge = (BOuterNode<PAGE_SIZE> *)merge_header;
                        if (!merge->can_delete())
                        {
                            // add all from right node to left node
                            if (cache)
                            {
                                cache->update_range(merge->keys[0], merge->keys[merge->current_index - 1], child_header->page_id, child_header);
                            }

                            for (int i = 0; i < merge->current_index; i++)
                            {
                                child->insert(merge->keys[i], merge->values[i]);
                            }

                            child->next_lef_id = merge->next_lef_id;
                            node->delete_value(node->keys[index]);
                            buffer_manager->unfix_page(merge->header.page_id, false);
                            buffer_manager->delete_page(merge->header.page_id);

                            // unfix
                            buffer_manager->unfix_page(child->header.page_id, true);

                            return;
                        }
                        else
                        {
                            buffer_manager->unfix_page(merge->header.page_id, false);
                        }
                    }
                }
                index++;
            }
        }
    }

    /**
     * @brief Finds the next bigger element
     * @param header The parent node of which an element will be deleted
     * @return the biggest element in the tree smaller than the actual rightmost element in the outer node
     */
    int64_t find_biggest(BHeader *header)
    {
        if (!header->inner)
        {
            BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;
            // last one is still the key that will be deleted
            int64_t key = node->keys[node->current_index - 2];
            buffer_manager->unfix_page(header->page_id, false);
            return key;
        }
        else
        {
            BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;
            BHeader *child_header = buffer_manager->request_page(node->child_ids[node->current_index]);
            buffer_manager->unfix_page(header->page_id, false);
            return find_biggest(child_header);
        }
    }

    /**
     * @brief Update the value for a key recursively
     * @param header The pointer to the current node
     * @param key The key corresponding to a value
     * @param value The value corresponding to the key
     */
    void update_recursive(BHeader *header, int64_t key, int64_t value)
    {
        if (!header->inner)
        {
            BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;
            node->update(key, value);
            if (cache)
            {
                cache->insert(key, header->page_id, header);
            }
            buffer_manager->unfix_page(node->header.page_id, true);
        }
        else
        {
            BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;
            BHeader *child_header = buffer_manager->request_page(node->next_page(key));
            buffer_manager->unfix_page(header->page_id, false);
            update_recursive(child_header, key, value);
        }
    }

    /**
     * @brief Start at element key and get range consecutive elements
     * @param header The pointer to the current node
     * @param key The key corresponding to a value
     * @param range The number of elements that are scanned
     * @return The sum of the elements
     */
    int64_t scan_recursive(BHeader *header, int64_t key, int range)
    {
        if (!header->inner)
        {
            return TreeOperations::scan<PAGE_SIZE>(buffer_manager, cache, header, key, range);
        }
        else
        {
            BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;
            BHeader *child_header = buffer_manager->request_page(node->next_page(key));
            buffer_manager->unfix_page(header->page_id, false);
            return scan_recursive(child_header, key, range);
        }
    }

    /**
     * @brief Validates if the tree is balanced
     * @param page_id The page_id of node
     * @return true if the tree is balanced, false if not
     */
    bool is_balanced()
    {
        BHeader *root = buffer_manager->request_page(root_id);
        buffer_manager->unfix_page(root_id, false);
        int balanced = recursive_is_balanced(root);
        return balanced != -1;
    }

    /**
     * @brief Validates if the tree is balanced recursively
     * @param header The current node
     * @return -1 if the tree is unbalanced, the depth otherwise
     */
    int recursive_is_balanced(BHeader *header)
    {
        if (!header->inner)
            return 1;

        BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;

        auto current_index = node->current_index;
        uint64_t child_ids[current_index + 1];
        for (int i = 0; i <= current_index; i++)
        {
            child_ids[i] = node->child_ids[i];
        }

        int depth = 1;
        if (current_index > 0)
        {
            BHeader *child_header = buffer_manager->request_page(child_ids[0]);
            buffer_manager->unfix_page(child_ids[0], false);
            depth = recursive_is_balanced(child_header);

            for (int i = 1; i <= current_index; i++)
            {
                child_header = buffer_manager->request_page(child_ids[i]);
                buffer_manager->unfix_page(child_ids[i], false);
                bool balanced = (depth == recursive_is_balanced(child_header));

                if (!balanced)
                    return -1;
            }
            depth += 1;
        }
        return depth;
    }

    /**
     * @brief Validates if the tree is ordered
     * @return true if the tree is ordered, false if not
     */
    bool is_ordered()
    {
        BHeader *root = buffer_manager->request_page(root_id);
        buffer_manager->unfix_page(root_id, false);
        bool ordered = recursive_is_ordered(root);
        return ordered;
    }

    /**
     * @brief Validates if the tree is ordered recursively
     * @param header The current node
     * @return true if the tree is ordered, false otherwise
     */
    bool recursive_is_ordered(BHeader *header)
    {
        if (!header->inner)
            return true;

        BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;
        auto current_index = node->current_index;
        uint64_t child_ids[current_index + 1];
        for (int i = 0; i <= current_index; i++)
        {
            child_ids[i] = node->child_ids[i];
        }
        uint64_t keys[current_index];
        for (int i = 0; i < current_index; i++)
        {
            keys[i] = node->keys[i];
        }

        for (int i = 0; i < current_index; i++)
        {
            bool ordered = smaller_or_equal(child_ids[i], keys[i]) && bigger(child_ids[i + 1], keys[i]);
            if (!ordered)
                return false;
        }

        BHeader *child_header;
        for (int i = 0; i <= current_index; i++)
        {
            child_header = buffer_manager->request_page(child_ids[i]);
            buffer_manager->unfix_page(child_ids[i], false);
            bool ordered = recursive_is_ordered(child_header);
            if (!ordered)
                return false;
        }
        return true;
    }

    /**
     * @brief Checks if all keys in a page are smaller or equal to key
     * @param page_id The page to check
     * @param key The key to compare to
     * @return true if all keys are smaller or equal, false otherwise
     */
    bool smaller_or_equal(uint64_t page_id, int64_t key)
    {
        BHeader *header = buffer_manager->request_page(page_id);
        if (!header->inner)
        {
            buffer_manager->unfix_page(page_id, false);
            BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;

            for (int i = 0; i < node->current_index; i++)
            {
                if (node->keys[i] > key)
                {
                    return false;
                }
            }
            return true;
        }

        BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;
        auto current_index = node->current_index;
        uint64_t child_ids[current_index + 1];
        for (int i = 0; i <= current_index; i++)
        {
            child_ids[i] = node->child_ids[i];
        }
        buffer_manager->unfix_page(page_id, false);

        for (int i = 0; i < node->current_index; i++)
        {
            if (node->keys[i] > key)
            {
                return false;
            }
        }

        for (int i = 0; i <= current_index; i++)
        {
            if (!smaller_or_equal(child_ids[0], key))
                return false;
        }

        return true;
    }

    /**
     * @brief Checks if all keys in a page are bigger to key
     * @param page_id The page to check
     * @param key The key to compare to
     * @return true if all keys are bigger, false otherwise
     */
    bool bigger(uint64_t page_id, int64_t key)
    {
        BHeader *header = buffer_manager->request_page(page_id);
        if (!header->inner)
        {
            buffer_manager->unfix_page(page_id, false);
            BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;

            for (int i = 0; i < node->current_index; i++)
            {
                if (node->keys[i] < key)
                {
                    return false;
                }
            }
            return true;
        }

        BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;
        auto current_index = node->current_index;
        uint64_t child_ids[current_index + 1];
        for (int i = 0; i <= current_index; i++)
        {
            child_ids[i] = node->child_ids[i];
        }
        buffer_manager->unfix_page(page_id, false);

        for (int i = 0; i < node->current_index; i++)
        {
            if (node->keys[i] < key)
            {
                return false;
            }
        }

        for (int i = 0; i <= current_index; i++)
        {
            if (!bigger(child_ids[0], key))
                return false;
        }

        return true;
    }

    /**
     * @brief Validates if the tree nodes are concatenated
     * @param num_elements Checks the number of elements in the tree
     * @return true if the tree is concatenated, false otherwise
     */
    bool is_concatenated(int num_elements)
    {
        uint64_t page_id = find_leftmost(root_id);
        BHeader *header;
        BOuterNode<PAGE_SIZE> *node;
        int count = 0;
        while (page_id != 0)
        {
            header = buffer_manager->request_page(page_id);
            node = (BOuterNode<PAGE_SIZE> *)header;
            buffer_manager->unfix_page(page_id, false);
            page_id = node->next_lef_id;
            for (int i = 0; i < node->current_index; i++)
            {
                count++;
                if (i > 0)
                {
                    if (node->keys[i - 1] > node->keys[i])
                    {
                        return false;
                    }
                }
            }
        }
        if (count != num_elements)
        {
            logger->debug("Count is off, count: {}, expected {}", count, num_elements);
            return false;
        }

        return true;
    }

    /**
     * @brief Finds the leftmost element in the tree
     * @param page_id The page id of the node
     * @returns the page_id of the leftmost node
     */
    uint64_t find_leftmost(uint64_t page_id)
    {
        BHeader *header = buffer_manager->request_page(page_id);
        if (!header->inner)
        {
            buffer_manager->unfix_page(page_id, false);
            return header->page_id;
        }

        BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)header;
        auto child_id = node->child_ids[0];
        buffer_manager->unfix_page(page_id, false);
        return find_leftmost(child_id);
    }

public:
    friend class BPlusTreeTest;
    friend class Debuger;

    /**
     * @brief Constructor for the B+ tree
     * @param buffer_manager_arg The buffer manager
     * @param cache_arg The chache
     */
    BPlusTree(BufferManager *buffer_manager_arg, RadixTree<PAGE_SIZE> *cache_arg = nullptr) : buffer_manager(buffer_manager_arg), cache(cache_arg)
    {
        logger = spdlog::get("logger");
        BHeader *root = buffer_manager->create_new_page();
        buffer_manager->unfix_page(root->page_id, true);
        new (root) BOuterNode<PAGE_SIZE>();
        root_id = root->page_id;
    };

    /**
     * @brief Insert an element into the tree
     * @param key The key that will be inserted
     * @param value The value that will be inserted
     */
    void insert(int64_t key, int64_t value)
    {
        recursive_insert(buffer_manager->request_page(root_id), key, value);
    }

    /**
     * @brief Delete an element from the tree
     * @param key The key that will be deleted
     */
    void delete_value(int64_t key)
    {
        recursive_delete(buffer_manager->request_page(root_id), key);
    }

    /**
     * @brief Get a value corresponding to the key
     * @param key The key corresponding a value
     * @return The value
     */
    int64_t get_value(int64_t key)
    {
        return recursive_get_value(buffer_manager->request_page(root_id), key);
    }

    /**
     * @brief Start at element key and get range consecutive elements
     * @param key The key corresponding to a value
     * @param range The number of elements that are scanned
     * @return The sum of the elements
     */
    int64_t scan(int64_t key, int range)
    {
        return scan_recursive(buffer_manager->request_page(root_id), key, range);
    }

    /**
     * @brief Update the value for a key
     * @param key The key corresponding to a value
     * @param value The value corresponding to the key
     */
    void update(int64_t key, int64_t value)
    {
        return update_recursive(buffer_manager->request_page(root_id), key, value);
    }

    /**
     * @brief Validates the b+ tree
     * @param num_elements The number of elements in the tree
     * @return true if the tree is valid, false otherwise
     */
    bool validate(int num_elements)
    {
        if (!is_balanced())
            return false;
        if (!is_ordered())
            return false;
        if (!is_concatenated(num_elements))
            return false;
        return true;
    }
};