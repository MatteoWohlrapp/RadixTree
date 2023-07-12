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
#include "spdlog/spdlog.h"

/// friend class
class RadixTreeTest;

template <int PAGE_SIZE>
class RadixTree
{
private:
    std::shared_ptr<spdlog::logger> logger;

    /**
     * @brief Insert an element into the radix_tree recursively
     * @param rheader The current node of the radix tree
     * @param key The key that will be inserted
     * @param page_id The page id of the page where the key is located
     * @param bheader The pointer to the header where the key is located
     */
    void insert_recursive(RHeader *rheader, int64_t key, uint64_t page_id, BHeader *bheader)
    {
        logger->debug("In recursive insert");
        if (!root)
        {
            logger->debug("Root is nullptr");
            // insert first element
            RHeader *new_root_header = (RHeader *)malloc(size_4);
            RNode4 *new_root = new (new_root_header) RNode4(true, 8, key, 0);
            logger->debug("Partial key; {}", get_key(key, 8));

            new_root->insert(get_key(key, 8), page_id, bheader);

            root = new_root_header;
        }
        else
        {
            // root edge cases
            if (rheader == root)
            {
                if (!can_insert(rheader))
                {
                    logger->debug("Cant insert into root");
                    root = increase_node_size(rheader);
                    insert(key, page_id, bheader);
                    return;
                }
                if (rheader->depth != 1)
                {
                    // compressed
                    logger->debug("Header depth not 1");
                    // check similarities between existing element and new key
                    int prefix_length = longest_common_prefix(root->key, key);

                    if (prefix_length + 1 < rheader->depth)
                    {
                        // add new node

                        // create new root node
                        RHeader *new_root_header = (RHeader *)malloc(size_4);
                        new (new_root_header) RNode4(false, prefix_length + 1, key, 0);

                        node_insert(new_root_header, get_key(root->key, prefix_length + 1), rheader);
                        node_insert(new_root_header, get_key(key, prefix_length + 1), key, page_id, bheader);
                        rheader->unfix_node();
                        // set new root
                        root = new_root_header;
                        return;
                    }
                }
            }
            // at this point, up to the current node, the values have the same prefix

            int partial_key = get_key(key, rheader->depth);

            if (rheader->leaf)
            {
                logger->debug("Header is leaf");
                // we reached a leaf node
                RFrame *leaf = (RFrame *)get_next_page(rheader, partial_key);
                // check if there is already an entry
                if (leaf)
                {
                    logger->debug("Leaf true, current size of node is: {}", rheader->type);
                    leaf->header = bheader;
                    leaf->page_id = page_id;
                }
                else
                {
                    logger->debug("Leaf true");
                    node_insert(rheader, partial_key, key, page_id, bheader);
                }
                rheader->unfix_node();
            }
            else
            {
                RHeader *child_header = (RHeader *)get_next_page(rheader, partial_key);

                if (!child_header)
                {
                    logger->debug("Child header null");
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
                        logger->debug("Prefix length is {} for child depth {}, so wrong for key {}, need to create new node", prefix_length + 1, child_header->depth, key);
                        // compression
                        // create new node
                        RHeader *new_node_header = (RHeader *)malloc(size_4);
                        new (new_node_header) RNode4(false, prefix_length + 1, key, 0);

                        node_insert(new_node_header, get_key(key, prefix_length + 1), key, page_id, bheader);
                        node_insert(new_node_header, get_key(child_header->key, prefix_length + 1), child_header);
                        node_insert(rheader, partial_key, new_node_header);
                        rheader->unfix_node();
                    }
                    else
                    {
                        child_header->fix_node();
                        logger->debug("Fixing header at depth: {} with key: {}", child_header->depth, child_header->key);
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
                for (int i = 0; i < 256; ++i)
                {
                    if (node->keys[i] != 256)
                    {
                        free(node->children[node->keys[i]]);
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
            return;
        }

        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            for (int i = 0; i < header->current_size; ++i)
            {
                destroy_recursive((RHeader *)node->children[i]);
                free(node->children[i]);
            }
            break;
        }
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            for (int i = 0; i < header->current_size; ++i)
            {
                destroy_recursive((RHeader *)node->children[i]);
                free(node->children[i]);
            }
            break;
        }
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            for (int i = 0; i < 256; ++i)
            {
                if (node->keys[i] != 256)
                {
                    destroy_recursive((RHeader *)node->children[node->keys[i]]);
                    free(node->children[node->keys[i]]);
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
                    free(node->children[i]);
                }
            }
            break;
        }
        }
    }

    /**
     * @brief Check if you can insert into a header
     * @param key The complete key
     * @param depth Which bits are currently relevant
     * @return the 8 bit part of the key that apply to the current node
     */
    uint8_t get_key(int64_t key, int depth)
    {
        return (key >> ((8 - depth) * 8)) & 0xFF;
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
            return new_header;
        }
        case 48:
        {
            RNode48 *node = (RNode48 *)header;

            RHeader *new_header = (RHeader *)malloc(size_256);
            RNode256 *new_node = new (new_header) RNode256(header->leaf, header->depth, header->key, 0);

            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 256)
                {
                    new_node->insert(i, node->children[node->keys[i]]);
                }
            }

            free(node);
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
                if (node->keys[i] != 256)
                {
                    new_node->insert(i, node->children[node->keys[i]]);
                }
            }

            free(node);
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
    int longest_common_prefix(int64_t key_a, int64_t key_b)
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
            logger->debug("Inserting child as void pointer");
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
    void node_insert(RHeader *parent, uint8_t partial_key, int64_t key, uint64_t page_id, BHeader *bheader)
    {
        void *new_header;
        if (parent->leaf)
        {
            new_header = malloc(16);
            RFrame *frame = (RFrame *)new_header;
            frame->header = bheader;
            frame->page_id = page_id;
        }
        else
        {
            // parent is not leaf, meaning we create a node with lazy expansion and add it
            new_header = malloc(size_4);
            RNode4 *new_node = new (new_header) RNode4(true, 8, key, 0);
            new_node->insert(get_key(key, 8), page_id, bheader);
        }

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
     * @brief Get the value for a key
     * @param header The radix tree node
     * @param key The key where the value is stored at
     * @return The value corresponding to the key or INT64_MIN if not present
     */
    int64_t get_value_recursive(RHeader *header, int64_t key)
    {
        if (header == nullptr)
            return INT64_MIN;

        void *next = get_next_page(header, get_key(key, header->depth));
        if (next)
        {
            if (header->leaf)
            {
                logger->debug("Header is leaf");
                RFrame *frame = (RFrame *)next;
                if (frame->header->page_id == frame->page_id)
                {
                    int64_t value = ((BOuterNode<PAGE_SIZE> *)frame->header)->get_value(key);
                    logger->debug("Page id {} matching, value is: {}", frame->page_id, value);
                    header->unfix_node();
                    return value;
                }
                else
                {
                    logger->debug("Page id from header {} not matching saved id {}", frame->header->page_id, frame->page_id);
                    header->unfix_node();
                    // Delete from tree as the reference is not matching and the page_id is wrong
                    delete_reference(key);
                    return INT64_MIN;
                }
            }
            else
            {
                logger->debug("Header is not leaf");
                RHeader *child = (RHeader *)next;
                child->fix_node();
                header->unfix_node();
                return get_value_recursive(child, key);
            }
        }
        else
        {
            logger->debug("Next was null");
            header->unfix_node();
            return INT64_MIN;
        }
    }

    /**
     * @brief Deletes a value from the tree
     * @param parent The parent radix tree node
     * @param child The child node of parent
     * @param key The key that will be deleted
     */
    void delete_reference_recursive(RHeader *parent, RHeader *child, int64_t key)
    {
        int partial_key = get_key(key, child->depth);
        RHeader *grand_child = (RHeader *)get_next_page(child, partial_key);

        if (!grand_child)
        {
            parent->unfix_node();
        }
        else
        {
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
                        free(child);
                    }
                    else if (!can_delete(child))
                    {
                        child = decrease_node_size(child);
                        node_insert(parent, get_key(key, parent->depth), child);
                    }
                }
                else if (!can_delete(grand_child))
                {
                    grand_child = decrease_node_size(grand_child);
                    node_insert(child, partial_key, grand_child);
                }

                parent->unfix_node();
            }
            else
            {
                parent->unfix_node();
                child->fix_node();
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
            node->delete_reference(key);
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            node->delete_reference(key);
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            node->delete_reference(key);
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            node->delete_reference(key);
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

    int size_4 = 64;     /// size for node 4 in bytes
    int size_16 = 168;   /// size for node 16 in bytes
    int size_48 = 920;   /// size for node 48 in bytes
    int size_256 = 2072; /// size for node 256 in bytes

public:
    friend class RadixTreeTest;
    // Root Node
    RHeader *root = nullptr;

    RadixTree()
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
        if (root)
        {
            root->fix_node();
        }
        insert_recursive(root, key, page_id, bheader);
    }

    /**
     * @brief Delete the reference from the tree
     * @param key The key that will be deleted
     */
    void delete_reference(int64_t key)
    {
        logger->debug("Deleting: {}", key);
        if (!root)
            return;

        root->fix_node();

        if (root->leaf)
        {
            node_delete(root, get_key(key, root->depth));

            if (root->current_size == 0)
            {
                RHeader *temp = root;
                root = nullptr;
                free(temp);
            }
            else if (!can_delete(root))
            {
                root = decrease_node_size(root);
            }
            else
            {
                root->unfix_node();
            }
        }
        else
        {
            int partial_key = get_key(key, root->depth);
            RHeader *child_header = (RHeader *)get_next_page(root, partial_key);

            if (child_header)
            {
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
                            free(temp);
                            return;
                        }
                        else if (!can_delete(root))
                        {
                            root = decrease_node_size(root);
                            return;
                        }
                    }
                    else if (!can_delete(child_header))
                    {
                        child_header = decrease_node_size(child_header);
                        node_insert(root, partial_key, child_header);
                    }

                    root->unfix_node();
                }
                else
                {
                    delete_reference_recursive(root, child_header, key);
                }
            }
            else
            {
                root->unfix_node();
            }
        }
    }

    /**
     * @brief Get a value corresponding to the key
     * @param key The key corresponding to the value
     * @return The value
     */
    int64_t get_value(int64_t key)
    {
        if (!root)
            return INT64_MIN;
        root->fix_node();
        return get_value_recursive(root, key);
    }

    /**
     * @brief Frees every allocated node, needs to be called at the end
     */
    void destroy()
    {
        destroy_recursive(root);
        free(root);
    }
};