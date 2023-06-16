/**
 * @file    BPlus.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

#include "../data/BufferManager.h"
#include "Nodes.h"
#include <array>
#include <math.h>
#include <iostream>
#include <cassert>
#include "spdlog/spdlog.h"

/// forward declaration
class BPlusTest;

/**
 * @brief Implements the B+ Tree of the Database
 */
template <int NODE_SIZE>
class BPlus
{
private:
    std::shared_ptr<spdlog::logger> logger;

    std::shared_ptr<BufferManager> buffer_manager;

    /**
     * @brief Inserts recursively into the tree
     * @param header The header of the current node
     * @param key The key to insert
     * @param value The value to insert
     */
    void recursive_insert(Header *header, int64_t key, int64_t value)
    {
        logger->debug("Recursive insert into page: {}", header->page_id);
        if (!header->inner)
        {
            // outer node
            OuterNode<NODE_SIZE> *node = (OuterNode<NODE_SIZE> *)header;
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
                Header *new_root_node_address = buffer_manager->create_new_page();
                InnerNode<NODE_SIZE> *new_root_node = new (new_root_node_address) InnerNode<NODE_SIZE>();

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
                // finished inserting, so page can be unfixed
                buffer_manager->unfix_page(header->page_id, true);
            }
        }
        else
        {
            // Inner node
            InnerNode<NODE_SIZE> *node = (InnerNode<NODE_SIZE> *)header;

            if (node->header.page_id == root_id && node->is_full())
            {
                // current inner node is root
                int split_index = get_split_index(node->max_size);
                // Because the node size has a lower limit, this does not cause issues
                int64_t split_key = node->keys[split_index - 1];
                uint64_t new_inner_id = split_inner_node(header, split_index);
                logger->debug("Splitting inner root node, new inner id: {}", new_inner_id);

                // create new inner node for root
                Header *new_root_node_address = buffer_manager->create_new_page();
                InnerNode<NODE_SIZE> *new_root_node = new (new_root_node_address) InnerNode<NODE_SIZE>();

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
                Header *child_header = buffer_manager->request_page(next_page);
                logger->debug("Finding child to insert: {}", next_page);

                if (child_header->inner)
                {
                    InnerNode<NODE_SIZE> *child = (InnerNode<NODE_SIZE> *)child_header;
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
                    OuterNode<NODE_SIZE> *child = (OuterNode<NODE_SIZE> *)child_header;

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
    void recursive_delete(Header *header, int64_t key)
    {
        if (!header->inner)
        {
            // outer node
            OuterNode<NODE_SIZE> *node = (OuterNode<NODE_SIZE> *)header;
            logger->flush();
            node->delete_pair(key);
            buffer_manager->unfix_page(header->page_id, true);
        }
        else
        {
            // Inner node
            InnerNode<NODE_SIZE> *node = (InnerNode<NODE_SIZE> *)header;

            if (header->page_id == root_id && node->current_index == 0)
            {
                // only happens for inner node after delete
                // no key left, just child ids still has an entry at position 0
                root_id = node->child_ids[0];
                buffer_manager->unfix_page(header->page_id, false);
                buffer_manager->delete_page(header->page_id);
                delete_pair(key);
            }
            else
            {
                // find next page to delete from
                uint64_t next_page = node->next_page(key);
                Header *child_header = buffer_manager->request_page(next_page);

                if (child_header->inner)
                {
                    InnerNode<NODE_SIZE> *child = (InnerNode<NODE_SIZE> *)child_header;

                    if (!child->can_delete())
                    {
                        logger->info("Cant delete inner child");
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
                            node->exchange(key, find_biggest_smallest(child_header));
                            child_header = buffer_manager->request_page(next_page);
                            dirty = true;
                        }

                        buffer_manager->unfix_page(header->page_id, dirty);
                        recursive_delete(child_header, key);
                    }
                }
                else
                {
                    OuterNode<NODE_SIZE> *child = (OuterNode<NODE_SIZE> *)child_header;

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
                            node->exchange(key, find_biggest_smallest(child_header));
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
    int64_t recursive_get_value(Header *header, int64_t key)
    {
        if (!header->inner)
        {
            OuterNode<NODE_SIZE> *node = (OuterNode<NODE_SIZE> *)header;
            int64_t value = node->get_value(key);
            buffer_manager->unfix_page(header->page_id, false);
            return value;
        }
        else
        {
            InnerNode<NODE_SIZE> *node = (InnerNode<NODE_SIZE> *)header;
            Header *child_header = buffer_manager->request_page(node->next_page(key));
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
    uint64_t split_outer_node(Header *header, int index_to_split)
    {
        assert(!header->inner && "Splitting node which is not an outer node");

        OuterNode<NODE_SIZE> *node = (OuterNode<NODE_SIZE> *)header;
        assert(index_to_split < node->max_size && "Splitting at an index which is bigger than the size");

        // Create new outer node
        Header *new_header = buffer_manager->create_new_page();
        OuterNode<NODE_SIZE> *new_outer_node = new (new_header) OuterNode<NODE_SIZE>();

        // It is important that index_to_split is already increases by 2
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
    uint64_t split_inner_node(Header *header, int index_to_split)
    {
        assert(header->inner && "Splitting node which is not an inner node");

        InnerNode<NODE_SIZE> *node = (InnerNode<NODE_SIZE> *)header;
        assert(index_to_split < node->max_size && "Splitting at an index which is bigger than the size");

        // Create new inner node
        Header *new_header = buffer_manager->create_new_page();
        InnerNode<NODE_SIZE> *new_inner_node = new (new_header) InnerNode<NODE_SIZE>();

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

    bool substitute(Header *header, Header *child_header)
    {
        InnerNode<NODE_SIZE> *node = (InnerNode<NODE_SIZE> *)header;

        if (child_header->inner)
        {
            InnerNode<NODE_SIZE> *child = (InnerNode<NODE_SIZE> *)child_header;
            InnerNode<NODE_SIZE> *substitute;
            int index = 0;

            while (index <= node->current_index)
            {
                if (node->child_ids[index] == child_header->page_id)
                {
                    if (index > 0)
                    {
                        substitute = (InnerNode<NODE_SIZE> *)buffer_manager->request_page(node->child_ids[index - 1]);
                        if (substitute->can_delete())
                        {
                            child->insert_first(node->keys[index - 1], substitute->child_ids[substitute->current_index]);
                            node->keys[index - 1] = substitute->keys[substitute->current_index - 1];

                            substitute->delete_pair(substitute->keys[substitute->current_index - 1]);
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
                        substitute = (InnerNode<NODE_SIZE> *)buffer_manager->request_page(node->child_ids[index + 1]);
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
            OuterNode<NODE_SIZE> *child = (OuterNode<NODE_SIZE> *)child_header;
            OuterNode<NODE_SIZE> *substitute;
            int index = 0;

            while (index <= node->current_index)
            {
                if (node->child_ids[index] == child_header->page_id)
                {
                    if (index > 0)
                    {
                        substitute = (OuterNode<NODE_SIZE> *)buffer_manager->request_page(node->child_ids[index - 1]);
                        if (substitute->can_delete())
                        {
                            // save biggest key and value from left in right node
                            child->insert(substitute->keys[substitute->current_index - 1], substitute->values[substitute->current_index - 1]);
                            substitute->delete_pair(substitute->keys[substitute->current_index - 1]);
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
                        substitute = (OuterNode<NODE_SIZE> *)buffer_manager->request_page(node->child_ids[index + 1]);
                        if (substitute->can_delete())
                        {
                            // save biggest key and value from right to left
                            child->insert(substitute->keys[0], substitute->values[0]);
                            node->keys[index] = substitute->keys[0];
                            substitute->delete_pair(substitute->keys[0]);

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

    void merge(Header *header, Header *child_header)
    {
        InnerNode<NODE_SIZE> *node = (InnerNode<NODE_SIZE> *)header;

        if (child_header->inner)
        {
            InnerNode<NODE_SIZE> *child = (InnerNode<NODE_SIZE> *)child_header;
            InnerNode<NODE_SIZE> *merge;
            int index = 0;

            while (index <= node->current_index)
            {
                if (node->child_ids[index] == child_header->page_id)
                {
                    if (index > 0)
                    {
                        merge = (InnerNode<NODE_SIZE> *)buffer_manager->request_page(node->child_ids[index - 1]);
                        if (!merge->can_delete())
                        {
                            // add all from right node to left node
                            merge->insert(node->keys[index - 1], child->child_ids[0]);

                            for (int i = 0; i < child->current_index; i++)
                            {
                                merge->insert(child->keys[i], child->child_ids[i + 1]);
                            }
                            node->delete_pair(node->keys[index - 1]);
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
                        merge = (InnerNode<NODE_SIZE> *)buffer_manager->request_page(node->child_ids[index + 1]);
                        if (!merge->can_delete())
                        {
                            child->insert(node->keys[index], merge->child_ids[0]);
                            for (int i = 0; i < merge->current_index; i++)
                            {
                                child->insert(merge->keys[i], merge->child_ids[i + 1]);
                            }

                            node->delete_pair(node->keys[index]);

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
            OuterNode<NODE_SIZE> *child = (OuterNode<NODE_SIZE> *)child_header;
            OuterNode<NODE_SIZE> *merge;
            int index = 0;

            while (index <= node->current_index)
            {
                if (node->child_ids[index] == child_header->page_id)
                {
                    if (index > 0)
                    {
                        merge = (OuterNode<NODE_SIZE> *)buffer_manager->request_page(node->child_ids[index - 1]);
                        if (!merge->can_delete())
                        {
                            // add all from right node to left node
                            for (int i = 0; i < child->current_index; i++)
                            {
                                merge->insert(child->keys[i], child->values[i]);
                            }
                            merge->next_lef_id = child->next_lef_id;
                            node->delete_pair(node->keys[index - 1]);
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
                        merge = (OuterNode<NODE_SIZE> *)buffer_manager->request_page(node->child_ids[index + 1]);
                        if (!merge->can_delete())
                        {
                            // add all from right node to left node
                            for (int i = 0; i < merge->current_index; i++)
                            {
                                child->insert(merge->keys[i], merge->values[i]);
                            }
                            child->next_lef_id = merge->next_lef_id;
                            node->delete_pair(node->keys[index]);
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

    int64_t find_biggest_smallest(Header *header)
    {
        if (!header->inner)
        {
            OuterNode<NODE_SIZE> *node = (OuterNode<NODE_SIZE> *)header;
            // last one is still the key that will be deleted
            int64_t key = node->keys[node->current_index - 2];
            buffer_manager->unfix_page(header->page_id, false);
            return key;
        }
        else
        {
            InnerNode<NODE_SIZE> *node = (InnerNode<NODE_SIZE> *)header;
            Header *child_header = buffer_manager->request_page(node->child_ids[node->current_index]);
            buffer_manager->unfix_page(header->page_id, false);
            return find_biggest_smallest(child_header);
        }
    }

public:
    friend class BPlusTest;
    /// root of tree
    uint64_t root_id;

    /**
     * @brief Constructor for the B+ tree
     * @param buffer_manager_arg The buffer manager
     */
    BPlus(std::shared_ptr<BufferManager> buffer_manager_arg) : buffer_manager(buffer_manager_arg)
    {
        logger = spdlog::get("logger");
        logger->debug("Called in BPlus tree");
        Header *root = buffer_manager->create_new_page();
        buffer_manager->unfix_page(root->page_id, true);
        new (root) OuterNode<NODE_SIZE>();
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
        buffer_manager->destroy();
    }

    /**
     * @brief Delete an element from the tree
     * @param key The key that will be deleted
     */
    void delete_pair(int64_t key)
    {
        recursive_delete(buffer_manager->request_page(root_id), key);
        buffer_manager->destroy();
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
};