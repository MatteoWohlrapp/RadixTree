
#include "BPlus.h"
#include <iostream>
#include <math.h>
#include "../Configuration.h"

BPlus::BPlus(std::shared_ptr<BufferManager> buffer_manager_arg) : buffer_manager(buffer_manager_arg)
{
    logger = spdlog::get("logger"); 
    root = buffer_manager->create_new_page();
    new (root) OuterNode<Configuration::page_size>(root);
};

void BPlus::insert(int key, int value)
{
    buffer_manager->fix_page(root->page_id);
    recursive_insert(root->page_id, key, value);
}

// TODO, fix and unfix pages!
void BPlus::recursive_insert(uint32_t page_id, int key, int value)
{
    Header *header = buffer_manager->request_page(page_id);
    if (!header->inner)
    {
        // outer node
        OuterNode<Configuration::page_size> *node = (OuterNode<Configuration::page_size> *)header;
        // case where the tree only has one outer node
        if (root->page_id == node->header.page_id && node->is_full())
        {
            // only when outer leaf is full, otherwise we will split node before
            // split outer node and save key
            int split_index = get_split_index(node->max_size);
            // Because the node size has a lower limit, this does not cause issues
            int split_key = node->keys[split_index - 1];
            uint32_t new_outer_id = split_outer_node(page_id, split_index);

            // create new inner node for root
            InnerNode<Configuration::page_size> *new_root_node_address = (InnerNode<Configuration::page_size> *)buffer_manager->create_new_page();
            InnerNode<Configuration::page_size> *new_root_node = new (new_root_node_address) InnerNode<Configuration::page_size>((Header *)new_root_node_address);

            new_root_node->keys[0] = split_key;
            new_root_node->child_ids[0] = node->header.page_id;
            new_root_node->child_ids[1] = new_outer_id;
            new_root_node->current_index++;

            root = (Header *)new_root_node;

            // fixing root and unfixing current page
            buffer_manager->fix_page(root->page_id);
            buffer_manager->unfix_page(page_id);
            // insert into new root
            recursive_insert(root->page_id, key, value);
        }
        else
        {
            node->insert(key, value);
            // finished inserting, so page can be unfixed
            buffer_manager->unfix_page(page_id);
        }
    }
    else
    {
        // Inner node
        InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;

        if (node->header.page_id == root->page_id && node->is_full())
        {
            // current inner node is root
            int split_index = get_split_index(node->max_size);
            // Because the node size has a lower limit, this does not cause issues
            int split_key = node->keys[split_index - 1];
            uint32_t new_inner_id = split_inner_node(node->header.page_id, split_index);
            // create new inner node for root
            InnerNode<Configuration::page_size> *new_root_node_address = (InnerNode<Configuration::page_size> *)buffer_manager->create_new_page();
            InnerNode<Configuration::page_size> *new_root_node = new (new_root_node_address) InnerNode<Configuration::page_size>((Header *)new_root_node_address);

            new_root_node->keys[0] = split_key;
            new_root_node->child_ids[0] = node->header.page_id;
            new_root_node->child_ids[1] = new_inner_id;
            new_root_node->current_index++;

            root = (Header *)new_root_node;

            // fixing root and unfixing current page
            buffer_manager->fix_page(root->page_id);
            buffer_manager->unfix_page(page_id);
            // insert into new root
            recursive_insert(root->page_id, key, value);
        }

        // find next page to insert
        uint32_t next_page = node->next_page(key);
        Header *child_address = buffer_manager->request_page(next_page);
        buffer_manager->fix_page(next_page);

        if (child_address->inner)
        {
            InnerNode<Configuration::page_size> *child = (InnerNode<Configuration::page_size> *)child_address;
            if (child->is_full())
            {
                // split the inner node
                int split_index = get_split_index(child->max_size);
                // Because the node size has a lower limit, this does not cause issues
                int split_key = child->keys[split_index - 1];
                uint32_t new_inner_id = split_inner_node(child->header.page_id, split_index);

                node->insert(split_key, new_inner_id);

                // unfix previous child which could have been changed due to splitting
                buffer_manager->unfix_page(next_page);
                // find correct new child and fix it, then unfix current child
                next_page = node->next_page(key);
                buffer_manager->fix_page(next_page);
                buffer_manager->unfix_page(node->header.page_id);

                recursive_insert(next_page, key, value);
            }
            else
            {
                // child correctly fixed before, just need to unfix current node
                buffer_manager->unfix_page(node->header.page_id);
                recursive_insert(child->header.page_id, key, value);
            }
        }
        else
        {
            OuterNode<Configuration::page_size> *child = (OuterNode<Configuration::page_size> *)child_address;

            if (child->is_full())
            {
                // split the outer node
                int split_index = get_split_index(child->max_size);
                // Because the node size has a lower limit, this does not cause issues
                int split_key = child->keys[split_index - 1];
                uint32_t new_outer_id = split_outer_node(child->header.page_id, split_index);
                // Insert new child and then call function again
                node->insert(new_outer_id, split_key);

                // unfix previous child which could have been changed due to splitting
                buffer_manager->unfix_page(next_page);
                // find correct new child and fix it, then unfix current child
                next_page = node->next_page(key);
                buffer_manager->fix_page(next_page);
                buffer_manager->unfix_page(node->header.page_id);

                recursive_insert(node->next_page(key), key, value);
            }
            else
            {
                // child correctly fixed before, just need to unfix current node
                buffer_manager->unfix_page(node->header.page_id);
                recursive_insert(child->header.page_id, key, value);
            }
        }
    }
}

// TODO, make use of pointers, unfix new page after use
uint32_t BPlus::split_outer_node(uint32_t page_id, int index_to_split)
{
    Header *header = buffer_manager->request_page(page_id);
    assert(!header->inner && "Splitting node which is not an outer node");

    OuterNode<Configuration::page_size> *node = (OuterNode<Configuration::page_size> *)header;
    assert(index_to_split < node->max_size && "Splitting at an index which is bigger than the size");

    // Create new outer node
    Header *new_header = buffer_manager->create_new_page();
    OuterNode<Configuration::page_size> *new_outer_node = new (new_header) OuterNode<Configuration::page_size>(new_header);

    // It is important that index_to_split is already increases by 2
    for (int i = index_to_split; i < node->max_size; i++)
    {
        new_outer_node->insert(node->keys[i], node->values[i]);
        node->current_index--;
    }
    // set correct chaining
    node->next_lef_id = new_outer_node->header.page_id;

    // unfixing the new page as we finished writing
    buffer_manager->unfix_page(new_header->page_id);

    return new_outer_node->header.page_id;
}

// TODO, make use of pointers, unfix new page after use
uint32_t BPlus::split_inner_node(uint32_t page_id, int index_to_split)
{
    Header *header = buffer_manager->request_page(page_id);
    assert(header->inner && "Splitting node which is not an inner node");

    InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;
    assert(index_to_split < node->max_size && "Splitting at an index which is bigger than the size");

    // Create new inner node
    Header *new_header = buffer_manager->create_new_page();
    InnerNode<Configuration::page_size> *new_inner_node = new (new_header) InnerNode<Configuration::page_size>(new_header);

    for (int i = index_to_split; i < node->max_size; i++)
    {
        new_inner_node->insert(node->child_ids[i + 1], node->keys[i]);
        node->current_index--;
    }
    node->current_index--;

    // unfixing the new page as we finished writing
    buffer_manager->unfix_page(new_header->page_id);

    return new_inner_node->header.page_id;
}

int BPlus::get_value(int key)
{
    return recursive_get_value(root->page_id, key);
}

int BPlus::recursive_get_value(uint32_t page_id, int key)
{
    Header *header = buffer_manager->request_page(page_id);
    if (!header->inner)
    {
        OuterNode<Configuration::page_size> *node = (OuterNode<Configuration::page_size> *)header;
        return node->get_value(key);
    }
    else
    {
        InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;
        return recursive_get_value(node->next_page(key), key);
    }
}

int BPlus::get_split_index(int max_size)
{
    if (max_size % 2 == 0)
        return max_size / 2;
    else
        return max_size / 2 + 1;
}
