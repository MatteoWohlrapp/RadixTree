
#include "BPlus.h"
#include "../Configuration.h"
#include <iostream>
#include <math.h>

BPlus::BPlus(std::shared_ptr<BufferManager> buffer_manager_arg) : buffer_manager(buffer_manager_arg)
{
    logger = spdlog::get("logger");
    logger->debug("Called in BPlus tree");
    Header *root = buffer_manager->create_new_page();
    buffer_manager->unfix_page(root->page_id, true);
    new (root) OuterNode<Configuration::page_size>();
    root_id = root->page_id;
};

void BPlus::insert(int64_t key, int64_t value)
{
    recursive_insert(buffer_manager->request_page(root_id), key, value);
    buffer_manager->destroy();
}

void BPlus::recursive_insert(Header *header, int64_t key, int64_t value)
{
    logger->debug("Recursive insert into page: {}", header->page_id);
    if (!header->inner)
    {
        // outer node
        OuterNode<Configuration::page_size> *node = (OuterNode<Configuration::page_size> *)header;
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
            InnerNode<Configuration::page_size> *new_root_node = new (new_root_node_address) InnerNode<Configuration::page_size>();

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
        InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;

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
            InnerNode<Configuration::page_size> *new_root_node = new (new_root_node_address) InnerNode<Configuration::page_size>();

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
                InnerNode<Configuration::page_size> *child = (InnerNode<Configuration::page_size> *)child_header;
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
                OuterNode<Configuration::page_size> *child = (OuterNode<Configuration::page_size> *)child_header;

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

uint64_t BPlus::split_outer_node(Header *header, int index_to_split)
{
    assert(!header->inner && "Splitting node which is not an outer node");

    OuterNode<Configuration::page_size> *node = (OuterNode<Configuration::page_size> *)header;
    assert(index_to_split < node->max_size && "Splitting at an index which is bigger than the size");

    // Create new outer node
    Header *new_header = buffer_manager->create_new_page();
    OuterNode<Configuration::page_size> *new_outer_node = new (new_header) OuterNode<Configuration::page_size>();

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

uint64_t BPlus::split_inner_node(Header *header, int index_to_split)
{
    assert(header->inner && "Splitting node which is not an inner node");

    InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;
    assert(index_to_split < node->max_size && "Splitting at an index which is bigger than the size");

    // Create new inner node
    Header *new_header = buffer_manager->create_new_page();
    InnerNode<Configuration::page_size> *new_inner_node = new (new_header) InnerNode<Configuration::page_size>();

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

int64_t BPlus::get_value(int64_t key)
{
    return recursive_get_value(buffer_manager->request_page(root_id), key);
}

int64_t BPlus::recursive_get_value(Header *header, int64_t key)
{
    if (!header->inner)
    {
        OuterNode<Configuration::page_size> *node = (OuterNode<Configuration::page_size> *)header;
        int64_t value = node->get_value(key);
        buffer_manager->unfix_page(header->page_id, false);
        return value;
    }
    else
    {
        InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;
        Header *child_header = buffer_manager->request_page(node->next_page(key));
        buffer_manager->unfix_page(header->page_id, false);
        return recursive_get_value(child_header, key);
    }
}

int BPlus::get_split_index(int max_size)
{
    if (max_size % 2 == 0)
        return max_size / 2;
    else
        return max_size / 2 + 1;
}