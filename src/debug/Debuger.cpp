
#include "Debuger.h"
#include "../Configuration.h"
#include <iostream>
#include <queue>
#include <sstream>
#include <set>

Debuger::Debuger(std::shared_ptr<BufferManager> buffer_manager_arg) : buffer_manager(buffer_manager_arg)
{
    logger = spdlog::get("logger");
}

// BFS
void Debuger::traverse_tree(BPlus<Configuration::page_size> *tree)
{
    if (!tree)
    {
        logger->info("Empty tree");
        return;
    }

    std::queue<uint64_t> nodes_queue;
    nodes_queue.push(tree->root_id);
    int level = 0;

    logger->info("Starting traversing on root node: {}", tree->root_id);
    while (!nodes_queue.empty())
    {
        int level_size = nodes_queue.size();
        logger->info("Level {} :", level);

        for (int i = 0; i < level_size; i++)
        {
            uint64_t current_id = nodes_queue.front();
            nodes_queue.pop();
            Header* current = buffer_manager->request_page(current_id); 

            if (!current->inner)
            {
                std::ostringstream node;
                OuterNode<Configuration::page_size> *outer_node = (OuterNode<Configuration::page_size> *)current;
                node << "OuterNode:  " << outer_node->header.page_id << " {";
                for (int j = 0; j < outer_node->current_index; j++)
                {
                    node << " (Key: " << outer_node->keys[j] << ", Value: " << outer_node->values[j] << ")";
                }
                node << "; Next Leaf: " << outer_node->next_lef_id << " }";
                logger->info(node.str());
            }
            else
            {
                std::ostringstream node;
                InnerNode<Configuration::page_size> *inner_node = (InnerNode<Configuration::page_size> *)current;
                node << "InnerNode: " << inner_node->header.page_id << " {";
                node << " (Child_id: " << inner_node->child_ids[0] << ", ";
                for (int j = 0; j < inner_node->current_index; j++)
                {
                    node << " Key: " << inner_node->keys[j] << ", Child_id: " << inner_node->child_ids[j + 1] << "";
                }
                node << ") }";
                logger->info(node.str());

                // <= because the child array is one position bigger
                for (int j = 0; j <= inner_node->current_index; j++)
                {
                    nodes_queue.push(inner_node->child_ids[j]);
                }
            }
            buffer_manager->unfix_page(current_id, false); 
        }

        level++;
    }
    logger->info("Finished traversing");
}

bool Debuger::are_all_child_ids_unique(BPlus<Configuration::page_size> *tree) {
    if (!tree)
    {
        logger->info("Empty tree");
        return true; 
    }

    std::set<uint64_t> unique_child_ids; 
    std::queue<uint64_t> nodes_queue;
    nodes_queue.push(tree->root_id);

    while (!nodes_queue.empty())
    {
        uint64_t current_id = nodes_queue.front();
        nodes_queue.pop();

        Header* current = buffer_manager->request_page(current_id); 

        if (current->inner) {
            InnerNode<Configuration::page_size> *inner_node = (InnerNode<Configuration::page_size> *)current;

            for (int j = 0; j <= inner_node->current_index; j++)
            {
                if (unique_child_ids.find(inner_node->child_ids[j]) != unique_child_ids.end()) {
                    buffer_manager->unfix_page(current_id, false);
                    return false;  
                }

                unique_child_ids.insert(inner_node->child_ids[j]);

                nodes_queue.push(inner_node->child_ids[j]);
            }
        }
        buffer_manager->unfix_page(current_id, false);
    }

    return true;
}

bool Debuger::contains_key(BPlus<Configuration::page_size> *tree, uint64_t key) {
    if (!tree) {
        logger->info("Empty tree");
        return false; 
    }

    std::queue<uint64_t> nodes_queue;
    nodes_queue.push(tree->root_id);

    while (!nodes_queue.empty()) {
        uint64_t current_id = nodes_queue.front();
        nodes_queue.pop();

        Header* current = buffer_manager->request_page(current_id); 

        if (current->inner) {
            InnerNode<Configuration::page_size> *inner_node = (InnerNode<Configuration::page_size> *)current;

            for (int j = 0; j < inner_node->current_index; j++) {
                if (inner_node->keys[j] == key) {
                    buffer_manager->unfix_page(current_id, false);
                    return true;  
                }
            }

            for (int j = 0; j <= inner_node->current_index; j++) {
                nodes_queue.push(inner_node->child_ids[j]);
            }
        } else {
            OuterNode<Configuration::page_size> *outer_node = (OuterNode<Configuration::page_size> *)current;

            for (int j = 0; j < outer_node->current_index; j++) {
                if (outer_node->keys[j] == key) {
                    buffer_manager->unfix_page(current_id, false);
                    return true; 
                }
            }
        }

        buffer_manager->unfix_page(current_id, false);
    }

    return false;
}