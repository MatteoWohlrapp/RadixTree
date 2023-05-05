
#include "Debuger.h"
#include <iostream>
#include <queue>

Debuger::Debuger(std::shared_ptr<BufferManager> buffer_manager_arg) : buffer_manager(buffer_manager_arg){}

// BFS
void Debuger::traverse_tree(BPlus* tree){
    if (!tree || !tree->root) {
        std::cout << "Empty tree." << std::endl;
        return;
    }

    std::queue<Header*> nodes_queue;
    nodes_queue.push(tree->root);
    int level = 0;

    while (!nodes_queue.empty()) {
        int level_size = nodes_queue.size();
        std::cout << "Level " << level << ":" << std::endl;

        for (int i = 0; i < level_size; i++) {
            Header *current = nodes_queue.front();
            nodes_queue.pop();

            if (!current->inner) {
                OuterNode<NODE_SIZE> *outer_node = (OuterNode<NODE_SIZE>*) current;
                std::cout << "OuterNode:  " << outer_node->header.page_id << " {";
                for (int j = 0; j < outer_node->current_index; j ++) {
                    std::cout << " (Key: " << outer_node->keys[j] << ", Value: " << outer_node->values[j] << ")";
                }
                std::cout << " }" << std::endl;
            } else {
                InnerNode<NODE_SIZE> *inner_node = (InnerNode<NODE_SIZE>*) current;
                std::cout << "InnerNode: " << inner_node->header.page_id << " {";
                std::cout << " (Child_id: " << inner_node->child_ids[0] << ", " ;
                for (int j = 0; j < inner_node->current_index; j++) {
                    std::cout << " Key: " << inner_node->keys[j] << ", Child_id: " << inner_node->child_ids[j+1] << "";
                }
                std::cout << ") }" << std::endl;

                for (int j = 0; j <= inner_node->current_index; j++) {
                    nodes_queue.push(buffer_manager->request_page(inner_node->child_ids[j]));
                }
            }
        }

        level++;
        std::cout << std::endl;
    }
}