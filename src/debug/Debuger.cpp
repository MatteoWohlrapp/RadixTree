
#include "Debuger.h"
#include "../Configuration.h"
#include "../radixtree/RNodes.h"
#include <iostream>
#include <queue>
#include <sstream>
#include <set>

Debuger::Debuger(std::shared_ptr<BufferManager> buffer_manager_arg) : buffer_manager(buffer_manager_arg)
{
    logger = spdlog::get("logger");
}

// BFS
void Debuger::traverse_btree(BPlus<Configuration::page_size> *tree)
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
            BHeader *current = buffer_manager->request_page(current_id);

            if (!current->inner)
            {
                std::ostringstream node;
                BOuterNode<Configuration::page_size> *outer_node = (BOuterNode<Configuration::page_size> *)current;
                node << "BOuterNode:  " << outer_node->header.page_id << " {";
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
                BInnerNode<Configuration::page_size> *inner_node = (BInnerNode<Configuration::page_size> *)current;
                node << "BInnerNode: " << inner_node->header.page_id << " {";
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

bool Debuger::are_all_child_ids_unique(BPlus<Configuration::page_size> *tree)
{
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

        BHeader *current = buffer_manager->request_page(current_id);

        if (current->inner)
        {
            BInnerNode<Configuration::page_size> *inner_node = (BInnerNode<Configuration::page_size> *)current;

            for (int j = 0; j <= inner_node->current_index; j++)
            {
                if (unique_child_ids.find(inner_node->child_ids[j]) != unique_child_ids.end())
                {
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

bool Debuger::contains_key(BPlus<Configuration::page_size> *tree, uint64_t key)
{
    if (!tree)
    {
        logger->info("Empty tree");
        return false;
    }

    std::queue<uint64_t> nodes_queue;
    nodes_queue.push(tree->root_id);

    while (!nodes_queue.empty())
    {
        uint64_t current_id = nodes_queue.front();
        nodes_queue.pop();

        BHeader *current = buffer_manager->request_page(current_id);

        if (current->inner)
        {
            BInnerNode<Configuration::page_size> *inner_node = (BInnerNode<Configuration::page_size> *)current;

            for (int j = 0; j < inner_node->current_index; j++)
            {
                if (inner_node->keys[j] == key)
                {
                    buffer_manager->unfix_page(current_id, false);
                    return true;
                }
            }

            for (int j = 0; j <= inner_node->current_index; j++)
            {
                nodes_queue.push(inner_node->child_ids[j]);
            }
        }
        else
        {
            BOuterNode<Configuration::page_size> *outer_node = (BOuterNode<Configuration::page_size> *)current;

            for (int j = 0; j < outer_node->current_index; j++)
            {
                if (outer_node->keys[j] == key)
                {
                    buffer_manager->unfix_page(current_id, false);
                    return true;
                }
            }
        }

        buffer_manager->unfix_page(current_id, false);
    }

    return false;
}

void Debuger::traverse_rtree(RadixTree *radixtree)
{
    if (!radixtree->root)
    {
        logger->info("RTree null"); 
        return; 
    }
    std::queue<RHeader *> nodes_queue;
    nodes_queue.push(radixtree->root);

    while (!nodes_queue.empty())
    {
        RHeader *header = nodes_queue.front();
        nodes_queue.pop();

        std::stringstream ss;
        ss << "node_type: " << header->type
           << ", node_address: " << header
           << ", leaf: " << (header->leaf ? "yes" : "no")
           << ", depth: " << static_cast<int>(header->depth)
           << ", current_size: " << header->current_size
           << ", key: [";

        for (int i = 7; i >= (9 - header->depth); i--)
        {
            uint8_t byte = (header->key >> (8 * i)) & 0xFF;
            ss << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(byte);
            if (i > 0)
            {
                ss << ", ";
            }
        }

        ss << "]";
        logger->info(ss.str());

        if (!header->leaf)
        {
            // Cast and process according to node type
            switch (header->type)
            {
            case 4:
            {
                RNode4 *node = (RNode4 *)header;
                for (int i = 0; i < header->current_size; ++i)
                {
                    std::stringstream ss_frame;
                    ss_frame << "child_key: " << static_cast<unsigned int>(node->keys[i]) << ", child_address: " << node->children[i];
                    logger->info(ss_frame.str());
                    nodes_queue.push((RHeader *)node->children[i]);
                }
                break;
            }
            case 16:
            {
                RNode16 *node = (RNode16 *)header;
                for (int i = 0; i < header->current_size; ++i)
                {
                    std::stringstream ss_frame;
                    ss_frame << "child_key: " << static_cast<unsigned int>(node->keys[i]) << ", child_address: " << node->children[i];
                    logger->info(ss_frame.str());
                    nodes_queue.push((RHeader *)node->children[i]);
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
                        std::stringstream ss_frame;
                        ss_frame << "child_key: " << static_cast<unsigned int>(i) << ", child_address: " << node->children[i];
                        logger->info(ss_frame.str());
                        nodes_queue.push((RHeader *)node->children[i]);
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
                        std::stringstream ss_frame;
                        ss_frame << "child_key: " << static_cast<unsigned int>(i) << ", child_address: " << node->children[i];
                        logger->info(ss_frame.str());
                        nodes_queue.push((RHeader *)node->children[i]);
                    }
                }
                break;
            }
            }
        }
        else
        {
            // It's a leaf node, so we log its frames
            switch (header->type)
            {
            case 4:
            {
                RNode4 *node = (RNode4 *)header;
                for (int i = 0; i < header->current_size; ++i)
                {
                    RFrame *frame = (RFrame *)node->children[i];
                    std::stringstream ss_frame;
                    ss_frame << "leaf_child_frame_id: " << frame->page_id;
                    logger->info(ss_frame.str());
                }
                break;
            }
            case 16:
            {
                RNode16 *node = (RNode16 *)header;
                for (int i = 0; i < header->current_size; ++i)
                {

                    RFrame *frame = (RFrame *)node->children[i];
                    std::stringstream ss_frame;
                    ss_frame << "leaf_child_frame_id: " << frame->page_id;
                    logger->info(ss_frame.str());
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
                        RFrame *frame = (RFrame *)node->children[i];
                        std::stringstream ss_frame;
                        ss_frame << "leaf_child_frame_id: " << frame->page_id;
                        logger->info(ss_frame.str());
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
                        RFrame *frame = (RFrame *)node->children[i];
                        std::stringstream ss_frame;
                        ss_frame << "leaf_child_frame_id: " << frame->page_id;
                        logger->info(ss_frame.str());
                    }
                }
                break;
            }
            }
        }
    }
}