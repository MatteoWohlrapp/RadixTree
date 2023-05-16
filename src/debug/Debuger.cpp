
#include "Debuger.h"
#include <iostream>
#include <queue>
#include <sstream>
#include "../Configuration.h"

Debuger::Debuger(std::shared_ptr<BufferManager> buffer_manager_arg) : buffer_manager(buffer_manager_arg)
{
    logger = spdlog::get("logger");
}

// BFS
void Debuger::traverse_tree(BPlus *tree)
{
    if (!tree || !tree->root)
    {
        logger->info("Empty tree");
        return;
    }

    std::queue<Header *> nodes_queue;
    nodes_queue.push(tree->root);
    int level = 0;

    while (!nodes_queue.empty())
    {
        int level_size = nodes_queue.size();
        logger->info("Level {} :", level);

        for (int i = 0; i < level_size; i++)
        {
            Header *current = nodes_queue.front();
            nodes_queue.pop();

            if (!current->inner)
            {
                std::ostringstream node;
                OuterNode<Configuration::page_size> *outer_node = (OuterNode<Configuration::page_size> *)current;
                node << "OuterNode:  " << outer_node->header.page_id << " {";
                for (int j = 0; j < outer_node->current_index; j++)
                {
                    node << " (Key: " << outer_node->keys[j] << ", Value: " << outer_node->values[j] << ")";
                }
                node << " }";
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

                for (int j = 0; j <= inner_node->current_index; j++)
                {
                    nodes_queue.push(buffer_manager->request_page(inner_node->child_ids[j]));
                }
            }
        }

        level++;
    }
} 