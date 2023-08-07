/**
 * @file    radix_tree.h
 *
 * @author  Matteo Wohlrapp
 * @date    17.06.2023
 */

#pragma once

#include "../model/b_header.h"
#include "../model/r_header.h"
#include "r_nodes.h"
#include "../bplus_tree/b_nodes.h"
#include "../utils/tree_operations.h"
#include "spdlog/spdlog.h"
#include <netinet/in.h>

/// friend class
class RadixTreeTest;
class Debuger;

template <int PAGE_SIZE>
class RadixTree
{
private:
    std::shared_ptr<spdlog::logger> logger;

    // Root Node
    RHeader *root = nullptr;

    uint64_t radix_tree_size;  /// maximum size of the cache in bytes
    uint64_t current_size = 0; /// current size of the cache in bytes

    BufferManager *buffer_manager;

    std::shared_mutex root_lock;

    int64_t buffer[256]; /// buffer that handles deletes if the cache is full
    int read = 0;
    int write = 0;

    /**
     * @brief transforms a signed key to an unsigned key
     * @param key The signed key
     * @return The unsigned key
     */
    uint64_t transform(int64_t key)
    {
        uint64_t transformed_key = ((uint64_t)key) + INT64_MAX + 1;
        return transformed_key;
    }

    /**
     * @brief transforms an unsigned key to a signed key
     * @param key The unsigned key
     * @return The signed key
     */
    int64_t inverse_transform(uint64_t key)
    {
        uint64_t transformed_key = ((uint64_t)key) - INT64_MAX - 1;
        return transformed_key;
    }

    /**
     * @brief Insert an element into the radix_tree recursively
     * @param rheader The current node of the radix tree
     * @param key The key that will be inserted
     * @param page_id The page id of the page where the key is located
     * @param bheader The pointer to the header where the key is located
     */
    void insert_recursive(RHeader *rheader, uint64_t key, uint64_t page_id, BHeader *bheader)
    {
        if (!root)
        {
            // insert first element
            RHeader *new_root_header = (RHeader *)malloc(size_4);
            RNode4 *new_root = new (new_root_header) RNode4(true, 8, key, 0);

            current_size += size_4;
            current_size += new_root->insert(get_key(key, 8), page_id, bheader);

            root = new_root_header;
        }
        else
        {
            // root edge cases
            if (rheader == root)
            {
                if (!can_insert(rheader))
                {
                    root = increase_node_size(rheader);
                    insert(inverse_transform(key), page_id, bheader);
                    return;
                }
                if (rheader->depth != 1)
                {
                    // compressed
                    // check similarities between existing element and new key
                    int prefix_length = longest_common_prefix(root->key, key);

                    if (prefix_length + 1 < rheader->depth)
                    {
                        // add new node

                        // create new root node
                        RHeader *new_root_header = (RHeader *)malloc(size_4);
                        new (new_root_header) RNode4(false, prefix_length + 1, key, 0);

                        node_insert(new_root_header, get_key(root->key, prefix_length + 1), rheader);
                        current_size += size_4;
                        // here, current size will be updated implicitly via return from node
                        node_insert(new_root_header, get_key(key, prefix_length + 1), key, page_id, bheader);
                        rheader->unfix_node();
                        // set new root
                        root = new_root_header;
                        return;
                    }
                }
            }
            // at this point, up to the current node, the values have the same prefix

            uint8_t partial_key = get_key(key, rheader->depth);

            if (rheader->leaf)
            {
                node_insert(rheader, partial_key, key, page_id, bheader);
                rheader->unfix_node();
            }
            else
            {
                RHeader *child_header = (RHeader *)get_next_page(rheader, partial_key);

                if (!child_header)
                {
                    // will do lazy expansion to save information
                    node_insert(rheader, partial_key, key, page_id, bheader);
                    rheader->unfix_node();
                }
                else
                {
                    if (!can_insert(child_header))
                    {
                        // if child is full we need to resize it
                        child_header = increase_node_size(child_header);
                        node_insert(rheader, partial_key, child_header);
                    }

                    int prefix_length = longest_common_prefix(child_header->key, key);

                    if (prefix_length + 1 < child_header->depth)
                    {
                        // compression
                        // create new node
                        RHeader *new_node_header = (RHeader *)malloc(size_4);
                        new (new_node_header) RNode4(false, prefix_length + 1, key, 0);

                        // size will be updated implicitly
                        node_insert(new_node_header, get_key(key, prefix_length + 1), key, page_id, bheader);
                        // size not update as just header is passed
                        node_insert(new_node_header, get_key(child_header->key, prefix_length + 1), child_header);
                        node_insert(rheader, partial_key, new_node_header);
                        current_size += size_4;
                        rheader->unfix_node();
                    }
                    else
                    {
                        child_header->fix_node();
                        rheader->unfix_node();
                        // in this case the prefix of the numbers matches and we know that we can insert, so calling recursive insert instead
                        insert_recursive(child_header, key, page_id, bheader);
                    }
                }
            }
        }
    }

    /**
     * @brief Check if you can insert into a header
     * @param header The current node of the radix tree
     * @return whether or not into the node can be inserted
     */
    bool can_insert(RHeader *header)
    {
        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            return node->can_insert();
        }
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            return node->can_insert();
        }
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            return node->can_insert();
        }
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            return node->can_insert();
        }
        }
        return false;
    }

    /**
     * @brief Check if you can delete from a header
     * @param header The current node of the radix tree
     * @return whether or not from the node can be deleted
     */
    bool can_delete(RHeader *header)
    {
        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            return node->can_delete();
        }
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            return node->can_delete();
        }
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            return node->can_delete();
        }
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            return node->can_delete();
        }
        }
        return false;
    }

    /**
     * @brief Cleanup the memory recursively
     * @param header The current node of the radix tree
     */
    void destroy_recursive(RHeader *header)
    {
        if (!header)
            return;
        if (header->leaf)
        {
            switch (header->type)
            {
            case 4:
            {
                RNode4 *node = (RNode4 *)header;
                for (int i = 0; i < header->current_size; ++i)
                {
                    free(node->children[i]);
                }
                break;
            }
            case 16:
            {
                RNode16 *node = (RNode16 *)header;
                for (int i = 0; i < header->current_size; ++i)
                {
                    free(node->children[i]);
                }
                break;
            }
            case 48:
            {
                RNode48 *node = (RNode48 *)header;
                for (int i = 0; i < 48; ++i)
                {
                    if (node->children[i])
                    {
                        free(node->children[i]);
                    }
                }
                break;
            }
            case 256:
            {
                RNode256 *node = (RNode256 *)header;
                for (int i = 0; i < 256; ++i)
                {
                    if (node->children[i])
                    {
                        free(node->children[i]);
                    }
                }
                break;
            }
            }
        }
        else
        {
            switch (header->type)
            {
            case 4:
            {
                RNode4 *node = (RNode4 *)header;
                for (int i = 0; i < header->current_size; ++i)
                {
                    destroy_recursive((RHeader *)node->children[i]);
                }
                break;
            }
            case 16:
            {
                RNode16 *node = (RNode16 *)header;
                for (int i = 0; i < header->current_size; ++i)
                {
                    destroy_recursive((RHeader *)node->children[i]);
                }
                break;
            }
            case 48:
            {
                RNode48 *node = (RNode48 *)header;
                for (int i = 0; i < 48; ++i)
                {
                    if (node->children[i])
                    {
                        destroy_recursive((RHeader *)node->children[i]);
                    }
                }
            }
            break;
            case 256:
            {
                RNode256 *node = (RNode256 *)header;
                for (int i = 0; i < 256; ++i)
                {
                    if (node->children[i])
                    {
                        destroy_recursive((RHeader *)node->children[i]);
                    }
                }
                break;
            }
            }
        }

        free(header);
    }

    /**
     * @brief Returns the 8 bit key at a certain depth
     * @param key The complete key
     * @param depth Which bits are currently relevant
     * @return the 8 bit part of the key that apply to the current node
     */
    uint8_t get_key(uint64_t key, int depth)
    {
        return (key >> ((8 - depth) * 8)) & 0xFF;
    }

    /**
     * @brief Returns the key up until a certain depth and replaces the last 8 bit with intermediate_key
     * @param key The complete key
     * @param intermediate_key The 8 bit that will represent the least significant byte
     * @param depth Which bits are currently relevant
     * @return the 8 bit part of the key that apply to the current node
     */
    uint64_t get_intermediate_key(uint64_t key, uint8_t intermediate_key, int depth)
    {
        return ((key >> ((8 - depth) * 8)) & ~0xFF) | intermediate_key;
    }

    /**
     * @brief Increase the node size of the current node
     * @param header The current node of the radix tree
     * @return The new node
     */
    RHeader *increase_node_size(RHeader *header)
    {
        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;

            RHeader *new_header = (RHeader *)malloc(size_16);
            RNode16 *new_node = new (new_header) RNode16(header->leaf, header->depth, header->key, 0);

            for (int i = 0; i < node->header.current_size; i++)
            {
                new_node->insert(node->keys[i], node->children[i]);
            }

            free(node);
            current_size -= size_4;
            current_size += size_16;
            return new_header;
        }
        case 16:
        {
            RNode16 *node = (RNode16 *)header;

            RHeader *new_header = (RHeader *)malloc(size_48);
            RNode48 *new_node = new (new_header) RNode48(header->leaf, header->depth, header->key, 0);

            for (int i = 0; i < node->header.current_size; i++)
            {
                new_node->insert(node->keys[i], node->children[i]);
            }

            free(node);
            current_size -= size_16;
            current_size += size_48;
            return new_header;
        }
        case 48:
        {
            RNode48 *node = (RNode48 *)header;

            RHeader *new_header = (RHeader *)malloc(size_256);
            RNode256 *new_node = new (new_header) RNode256(header->leaf, header->depth, header->key, 0);

            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 255)
                {
                    new_node->insert(i, node->children[node->keys[i]]);
                }
            }

            free(node);
            current_size -= size_48;
            current_size += size_256;
            return new_header;
        }
        }
        return nullptr;
    }

    /**
     * @brief Decrease the node size of the current node
     * @param header The current node of the radix tree
     * @return The new node
     */
    RHeader *decrease_node_size(RHeader *header)
    {
        switch (header->type)
        {
        case 16:
        {
            RNode16 *node = (RNode16 *)header;

            RHeader *new_header = (RHeader *)malloc(size_4);
            RNode4 *new_node = new (new_header) RNode4(header->leaf, header->depth, header->key, 0);

            for (int i = 0; i < node->header.current_size; i++)
            {
                new_node->insert(node->keys[i], node->children[i]);
            }

            free(node);
            current_size -= size_16;
            current_size += size_4;
            return new_header;
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;

            RHeader *new_header = (RHeader *)malloc(size_16);
            RNode16 *new_node = new (new_header) RNode16(header->leaf, header->depth, header->key, 0);

            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 255)
                {
                    new_node->insert(i, node->children[node->keys[i]]);
                }
            }

            free(node);
            current_size -= size_48;
            current_size += size_16;
            return new_header;
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;

            RHeader *new_header = (RHeader *)malloc(size_48);
            RNode48 *new_node = new (new_header) RNode48(header->leaf, header->depth, header->key, 0);

            for (int i = 0; i < 256; i++)
            {
                if (node->children[i])
                {
                    new_node->insert(i, node->children[i]);
                }
            }

            free(node);
            current_size -= size_256;
            current_size += size_48;
            return new_header;
        }
        break;
        }
        return nullptr;
    }

    /**
     * @brief Finds the longest possible byte prefix, longest possible is 7 - last insert is handled in node directly
     * @param key_a The first key
     * @param key_b The second key
     * @return How many bytes they have in common
     */
    int longest_common_prefix(uint64_t key_a, uint64_t key_b)
    {
        int prefix = 0;

        for (int i = 1; i < 9; i++)
        {
            if (get_key(key_a, i) == get_key(key_b, i) && prefix < 7)
            {
                prefix++;
            }
            else
            {
                break;
            }
        }

        return prefix;
    }

    /**
     * @brief Insert the child node into the parent node
     * @param parent The node in which will be inserted
     * @param key The key where insertion will take place
     * @param child The child pointer that will be inserted
     */
    void node_insert(RHeader *parent, uint8_t key, void *child)
    {
        switch (parent->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)parent;
            node->insert(key, child);
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)parent;
            node->insert(key, child);
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)parent;
            node->insert(key, child);
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)parent;
            node->insert(key, child);
        }
        break;
        }
    }

    /**
     * @brief Insert the page_id and bheader into the tree
     * @param parent The node in which will be inserted
     * @param partial_key The key where insertion will take place at the current depth
     * @param key The full key
     * @param page_id The page id of the page where the data is stored
     * @param bheader The bheader where the data is stored
     */
    void node_insert(RHeader *parent, uint8_t partial_key, uint64_t key, uint64_t page_id, BHeader *bheader)
    {
        void *new_header;
        if (parent->leaf)
        {
            switch (parent->type)
            {
            case 4:
            {
                RNode4 *node = (RNode4 *)parent;
                current_size += node->insert(partial_key, page_id, bheader);
            }
            break;
            case 16:
            {
                RNode16 *node = (RNode16 *)parent;
                current_size += node->insert(partial_key, page_id, bheader);
            }
            break;
            case 48:
            {
                RNode48 *node = (RNode48 *)parent;
                current_size += node->insert(partial_key, page_id, bheader);
            }
            break;
            case 256:
            {
                RNode256 *node = (RNode256 *)parent;
                current_size += node->insert(partial_key, page_id, bheader);
            }
            break;
            }
        }
        else
        {
            // parent is not leaf, meaning we create a node with lazy expansion and add it
            new_header = malloc(size_4);
            RNode4 *new_node = new (new_header) RNode4(true, 8, key, 0);
            current_size += size_4;
            current_size += new_node->insert(get_key(key, 8), page_id, bheader);

            switch (parent->type)
            {
            case 4:
            {
                RNode4 *node = (RNode4 *)parent;
                node->insert(partial_key, new_header);
            }
            break;
            case 16:
            {
                RNode16 *node = (RNode16 *)parent;
                node->insert(partial_key, new_header);
            }
            break;
            case 48:
            {
                RNode48 *node = (RNode48 *)parent;
                node->insert(partial_key, new_header);
            }
            break;
            case 256:
            {
                RNode256 *node = (RNode256 *)parent;
                node->insert(partial_key, new_header);
            }
            break;
            }
        }
    }

    /**
     * @brief Get the next page in the tree
     * @param header The radix tree node
     * @param key The key where the next child is
     * @return A pointer to the next page
     */
    void *get_next_page(RHeader *header, uint8_t key)
    {
        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            return node->get_next_page(key);
        }
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            return node->get_next_page(key);
        }
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            return node->get_next_page(key);
        }
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            return node->get_next_page(key);
        }
        }
        return nullptr;
    }

    /**
     * @brief Get the page a key is located on
     * @param header The radix tree node
     * @param key The key which is on the page
     * @return The page
     */
    BHeader *get_page_recursive(RHeader *header, uint64_t key)
    {
        if (header == nullptr)
            return nullptr;

        void *next = get_next_page(header, get_key(key, header->depth));
        if (next)
        {
            if (header->leaf)
            {
                RFrame *frame = (RFrame *)next;
                if (buffer_manager->can_fix(frame->page_id, frame->header))
                {
                    logger->info("Can fix worked");
                    header->unfix_node_read();
                    if (header == root)
                        root_lock.unlock_shared();
                    return frame->header;
                }
                else
                {
                    if (header == root)
                        root_lock.unlock_shared();
                    header->unfix_node_read();
                    // Delete from tree as the reference is not matching and the page_id is wrong
                    delete_reference(inverse_transform(key));
                    return nullptr;
                }
            }
            else
            {
                RHeader *child = (RHeader *)next;
                child->fix_node_read();
                header->unfix_node_read();
                if (header == root)
                    root_lock.unlock_shared();
                return get_page_recursive(child, key);
            }
        }
        else
        {
            header->unfix_node_read();
            if (header == root)
                root_lock.unlock_shared();
            return nullptr;
        }
    }

    /**
     * @brief Deletes a value from the tree
     * @param parent The parent radix tree node
     * @param child The child node of parent
     * @param key The key that will be deleted
     */
    void delete_reference_recursive(RHeader *parent, RHeader *child, uint64_t key)
    {
        uint8_t partial_key = get_key(key, child->depth);
        RHeader *grand_child = (RHeader *)get_next_page(child, partial_key);

        if (!grand_child)
        {
            parent->unfix_node();
            child->unfix_node();
            if (root == parent)
                root_lock.unlock();
        }
        else
        {
            grand_child->fix_node();
            if (grand_child->leaf)
            {
                node_delete(grand_child, get_key(key, 8));

                if (grand_child->current_size == 0)
                {
                    // automatically handles freeing of memory
                    node_delete(child, partial_key);

                    // because we have deleted an element from an inner node it can happen that we need to restore the compression
                    if (child->current_size == 1)
                    {
                        node_insert(parent, get_key(key, parent->depth), get_single_child(child));
                        free_node(child);
                    }
                    else if (!can_delete(child))
                    {
                        child = decrease_node_size(child);
                        node_insert(parent, get_key(key, parent->depth), child);
                    } else {
                        child->unfix_node(); 
                    }
                }
                else if (!can_delete(grand_child))
                {
                    grand_child = decrease_node_size(grand_child);
                    node_insert(child, partial_key, grand_child);
                    child->unfix_node();
                }
                else
                {
                    grand_child->unfix_node(); 
                    child->unfix_node();
                }

                parent->unfix_node();
                if (root == parent)
                    root_lock.unlock();
            }
            else
            {
                parent->unfix_node();
                if (root == parent)
                    root_lock.unlock();
                delete_reference_recursive(child, grand_child, key);
            }
        }
    }

    /**
     * @brief Deletes from a node
     * @param header The node from which will be deleted
     * @param key The key where values should be deleted from
     */
    void node_delete(RHeader *header, uint8_t key)
    {
        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            current_size -= node->delete_reference(key);
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            current_size -= node->delete_reference(key);
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            current_size -= node->delete_reference(key);
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            current_size -= node->delete_reference(key);
        }
        break;
        }
    }

    /**
     * @brief Get the single child of a node
     * @param header The node that only has one child
     * @return A pointer to single child
     */
    void *get_single_child(RHeader *header)
    {
        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            return node->get_single_child();
        }
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            return node->get_single_child();
        }
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            return node->get_single_child();
        }
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            return node->get_single_child();
        }
        }
        return nullptr;
    }

    /**
     * @brief Updates a range of values recurisvely, specified by values from and to
     * @param header of the current radix tree node
     * @param from key from which updates are applied
     * @param to key until which updates are applied
     * @param page_id page_id of the page that is updated
     * @param bheader the header to the page where the value can be found
     */
    void update_range_recursive(RHeader *header, uint64_t from, uint64_t to, uint64_t page_id, BHeader *bheader)
    {

        uint64_t from_key = get_intermediate_key(from, get_key(from, header->depth), header->depth);
        uint64_t to_key = get_intermediate_key(to, get_key(to, header->depth), header->depth);
        uint64_t intermediate_key;

        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            for (int i = 0; i < node->header.current_size; i++)
            {
                intermediate_key = get_intermediate_key(header->key, node->keys[i], header->depth);

                if (intermediate_key >= from_key && intermediate_key <= to_key)
                {
                    if (header->leaf)
                    {
                        node_insert(header, node->keys[i], header->key, page_id, bheader);
                    }
                    else
                    {
                        ((RHeader *)node->children[i])->fix_node();
                        update_range_recursive(((RHeader *)node->children[i]), from, to, page_id, bheader);
                    }
                }
            }
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            for (int i = 0; i < node->header.current_size; i++)
            {
                intermediate_key = get_intermediate_key(header->key, node->keys[i], header->depth);

                if (intermediate_key >= from_key && intermediate_key <= to_key)
                {
                    if (header->leaf)
                    {
                        node_insert(header, node->keys[i], header->key, page_id, bheader);
                    }
                    else
                    {
                        ((RHeader *)node->children[i])->fix_node();
                        update_range_recursive(((RHeader *)node->children[i]), from, to, page_id, bheader);
                    }
                }
            }
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 255)
                {

                    intermediate_key = get_intermediate_key(header->key, i, header->depth);
                    if (intermediate_key >= from_key && intermediate_key <= to_key)
                    {
                        if (header->leaf)
                        {
                            node_insert(header, i, header->key, page_id, bheader);
                        }
                        else
                        {
                            ((RHeader *)node->children[node->keys[i]])->fix_node();
                            update_range_recursive(((RHeader *)node->children[node->keys[i]]), from, to, page_id, bheader);
                        }
                    }
                }
            }
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->children[i])
                {

                    intermediate_key = get_intermediate_key(header->key, i, header->depth);
                    if (intermediate_key >= from_key && intermediate_key <= to_key)
                    {
                        if (header->leaf)
                        {
                            node_insert(header, i, header->key, page_id, bheader);
                        }
                        else
                        {
                            ((RHeader *)node->children[i])->fix_node();
                            update_range_recursive(((RHeader *)node->children[i]), from, to, page_id, bheader);
                        }
                    }
                }
            }
        }
        break;
        }
        header->unfix_node();
    }

    /**
     * @brief Frees a node and updates the byte count
     * @param header The node that should be deleted
     */
    void free_node(RHeader *header)
    {
        switch (header->type)
        {
        case 4:
            current_size -= size_4;
            break;
        case 16:
            current_size -= size_16;
            break;
        case 48:
            current_size -= size_48;
            break;
        case 256:
            current_size -= size_256;
            break;
        default:
            break;
        }
        free(header);
    }

    /**
     * @brief checks if the tree is compressed
     * @param header The current node
     * @return true if the tree is compressed false, if not
     */
    bool is_compressed(RHeader *header)
    {
        if (!header)
            return true;
        if (header->leaf)
            return true;
        else
        {
            if (header->current_size <= 1)
                return false;
        }

        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (!child->leaf)
                {
                    if (child->current_size <= 1 || !is_compressed(child))
                        return false;
                }
            }
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (!child->leaf)
                {
                    if (child->current_size <= 1 || !is_compressed(child))
                        return false;
                }
            }
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 255)
                {
                    RHeader *child = (RHeader *)node->children[node->keys[i]];
                    if (!child->leaf)
                    {
                        if (child->current_size <= 1 || !is_compressed(child))
                            return false;
                    }
                }
            }
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->children[i])
                {
                    RHeader *child = (RHeader *)node->children[i];
                    if (!child->leaf)
                    {
                        if (child->current_size <= 1 || !is_compressed(child))
                            return false;
                    }
                }
            }
        }
        break;
        default:
            return false;
        }
        return true;
    }

    /**
     * @brief checks if the depth of leaf nodes is correct
     * @param header The current node
     * @return true if the depth of the leaves is correct, false if not
     */
    bool leaf_depth_correct(RHeader *header)
    {
        if (!header)
            return true;
        if (header->leaf)
            return header->depth == 8;
        else
        {
            if (header->depth == 8)
                return false;
        }

        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (!child->leaf)
                {
                    if (!leaf_depth_correct(child))
                        return false;
                }
                else
                {
                    if (child->depth != 8)
                        return false;
                }
            }
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (!child->leaf)
                {
                    if (!leaf_depth_correct(child))
                        return false;
                }
                else
                {
                    if (child->depth != 8)
                        return false;
                }
            }
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 255)
                {
                    RHeader *child = (RHeader *)node->children[node->keys[i]];
                    if (!child->leaf)
                    {
                        if (!leaf_depth_correct(child))
                            return false;
                    }
                    else
                    {
                        if (child->depth != 8)
                            return false;
                    }
                }
            }
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->children[i])
                {
                    RHeader *child = (RHeader *)node->children[i];
                    if (!child->leaf)
                    {
                        if (!leaf_depth_correct(child))
                            return false;
                    }
                    else
                    {
                        if (child->depth != 8)
                            return false;
                    }
                }
            }
        }
        break;
        default:
            break;
        }
        return true;
    }

    /**
     * @brief checks if the prefixes of the keys in the tree match
     * @param header The current node
     * @return true if all key match, false if not
     */
    bool key_matches(RHeader *header)
    {
        if (!header)
            return true;

        if (header->leaf)
            return true;

        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (header->depth - 1 > longest_common_prefix(header->key, child->key) || !key_matches(child))
                    return false;
            }
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (header->depth - 1 > longest_common_prefix(header->key, child->key) || !key_matches(child))
                    return false;
            }
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 255)
                {
                    RHeader *child = (RHeader *)node->children[node->keys[i]];
                    if (header->depth - 1 > longest_common_prefix(header->key, child->key) || !key_matches(child))
                        return false;
                }
            }
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->children[i])
                {
                    RHeader *child = (RHeader *)node->children[i];
                    if (header->depth - 1 > longest_common_prefix(header->key, child->key) || !key_matches(child))
                        return false;
                }
            }
        }
        break;
        default:
            break;
        }
        return true;
    }

public:
    friend class RadixTreeTest;
    friend class Debuger;

    RadixTree(uint64_t radix_tree_size_arg, BufferManager *buffer_manager_arg) : radix_tree_size(radix_tree_size_arg), buffer_manager(buffer_manager_arg)
    {
        logger = spdlog::get("logger");
    }

    /**
     * @brief Insert an element into the radix_tree
     * @param key The key that will be inserted
     * @param page_id The page id of the page where the key is located
     * @param bheader The pointer to the header where the key is located
     */
    void insert(int64_t key, uint64_t page_id, BHeader *bheader)
    {
        if (current_size < radix_tree_size)
        {
            buffer[write] = key;
            write = (write + 1) % 256;
            if (write == read)
                read = (read + 1) % 256;

            if (root)
            {
                root->fix_node();
            }
            insert_recursive(root, transform(key), page_id, bheader);
        }
        else
        {
            if (read != write)
            {
                delete_reference(buffer[read]);
                read = (read + 1) % 256;
            }
        }
    }

    /**
     * @brief Delete the reference from the tree
     * @param s_key The key that will be deleted
     */
    void delete_reference(int64_t s_key)
    {
        uint64_t key = transform(s_key);
        if (!root)
            return;

        root_lock.lock();
        root->fix_node();

        if (root->leaf)
        {
            node_delete(root, get_key(key, root->depth));

            if (root->current_size == 0)
            {
                RHeader *temp = root;
                root = nullptr;
                free_node(temp);
            }
            else if (!can_delete(root))
            {
                root = decrease_node_size(root);
            }
            else
            {
                root->unfix_node();
            }

            root_lock.unlock();
        }
        else
        {
            uint8_t partial_key = get_key(key, root->depth);
            RHeader *child_header = (RHeader *)get_next_page(root, partial_key);

            if (child_header)
            {
                child_header->fix_node();
                if (child_header->leaf)
                {
                    node_delete(child_header, get_key(key, 8));

                    if (child_header->current_size == 0)
                    {
                        // automatically handles freeing of memory
                        node_delete(root, partial_key);

                        // because we have deleted an element from an inner node it can happen that we need to restore the compression
                        if (root->current_size == 1)
                        {
                            RHeader *temp = root;
                            root = (RHeader *)get_single_child(root);
                            free_node(temp);
                            root_lock.unlock();
                            return;
                        }
                        else if (!can_delete(root))
                        {
                            root = decrease_node_size(root);
                            root_lock.unlock();
                            return;
                        }
                    }
                    else if (!can_delete(child_header))
                    {
                        child_header = decrease_node_size(child_header);
                        node_insert(root, partial_key, child_header);
                    }
                    else
                    {
                        child_header->unfix_node();
                    }

                    root->unfix_node();
                    root_lock.unlock();
                }
                else
                {
                    delete_reference_recursive(root, child_header, key);
                }
            }
            else
            {
                root->unfix_node();
                root_lock.unlock();
            }
        }
    }

    /**
     * @brief Updates a range of values, specified by values from and to
     * @param from key from which updates are applied
     * @param to key until which updates are applied
     * @param page_id page_id of the page that is updated
     * @param bheader the header to the page where the value can be found
     */
    void update_range(int64_t from, int64_t to, int64_t page_id, BHeader *bheader)
    {
        if (!root)
            return;

        root->fix_node();

        update_range_recursive(root, transform(from), transform(to), page_id, bheader);
    }

    /**
     * @brief Frees every allocated node, needs to be called at the end
     */
    void destroy()
    {
        destroy_recursive(root);
    }

    /**
     * @brief Validates the radix tree
     * @return true if the tree is valid, false otherwise
     */
    bool valdidate()
    {
        std::cout << "Size of Radix Tree: " << current_size << std::endl;

        if (!is_compressed(root))
            return false;
        if (!leaf_depth_correct(root))
            return false;
        if (!key_matches(root))
            return false;

        return true;
    }

    /**
     * @brief Get a value corresponding to the key
     * @param key The key corresponding to the value
     * @return The value
     */
    int64_t get_value(int64_t key)
    {
        if (root)
        {
            root_lock.lock_shared();
            root->fix_node_read();
            BHeader *header = get_page_recursive(root, transform(key));
            if (header)
            {
                uint64_t value = ((BOuterNode<PAGE_SIZE> *)header)->get_value(key);
                buffer_manager->unfix_page(header->page_id, false);
                return value;
            }
        }
        return INT64_MIN;
    }

    /**
     * @brief Updates a key if it is cached
     * @param key The key to update
     * @param value The value to update
     * @return true if update was possible, false otherwise
     */
    bool update(int64_t key, int64_t value)
    {
        if (root)
        {
            root_lock.lock_shared();
            root->fix_node_read();
            BHeader *header = get_page_recursive(root, transform(key));
            if (header)
            {
                ((BOuterNode<PAGE_SIZE> *)header)->update(key, value);
                buffer_manager->unfix_page(header->page_id, true);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Performs a scan if the value is cached
     * @param key The key to start the scan from
     * @param range How many elements to scan
     * @return the sum of the scan, INT64_MIN otherwise
     */
    int64_t scan(int64_t key, int range)
    {
        if (root)
        {
            root->fix_node();
            BHeader *header = get_page_recursive(root, transform(key));
            if (header)
            {
                buffer_manager->fix_page(header->page_id);
                return TreeOperations::scan<PAGE_SIZE>(buffer_manager, nullptr, header, key, range);
            }
        }
        return INT64_MIN;
    }

    /**
     * @brief Deletes a value from the tree when the page is cached
     * @param key The key corresponding to the value that will be deleted
     * @return true if it can delete the value, false otherwise
     */
    bool delete_value(int64_t key)
    {
        if (root)
        {
            root->fix_node();
            BHeader *header = get_page_recursive(root, transform(key));
            if (header)
            {
                BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)header;
                if (node->can_delete())
                {
                    buffer_manager->fix_page(header->page_id);
                    node->delete_value(key);
                    delete_reference(key);
                    buffer_manager->unfix_page(header->page_id, true);
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief Returns the current size of the cache
     * @return the size of the cache
     */
    uint64_t get_cache_size()
    {
        return current_size;
    }
};