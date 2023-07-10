/**
 * @file    RadixTree.h
 *
 * @author  Matteo Wohlrapp
 * @date    17.06.2023
 */

#pragma once

#include "../model/BHeader.h"
#include "../model/RHeader.h"
#include "spdlog/spdlog.h"

/// friend class
class RadixTreeTest;

class RadixTree
{
private:
    std::shared_ptr<spdlog::logger> logger;

    /**
     * @brief Insert an element into the radixtree recursively
     * @param rheader The current node of the radix tree
     * @param key The key that will be inserted
     * @param page_id The page id of the page where the key is located
     * @param bheader The pointer to the header where the key is located
     */
    void insert_recursive(RHeader *rheader, int64_t key, uint64_t page_id, BHeader *bheader);

    /**
     * @brief Check if you can insert into a header
     * @param header The current node of the radix tree
     * @return whether or not into the node can be inserted
     */
    bool can_insert(RHeader *header);

    /**
     * @brief Check if you can delete from a header
     * @param header The current node of the radix tree
     * @return whether or not from the node can be deleted
     */
    bool can_delete(RHeader *header);

    /**
     * @brief Cleanup the memory recursively
     * @param header The current node of the radix tree
     */
    void destroy_recursive(RHeader *header);

    /**
     * @brief Check if you can insert into a header
     * @param key The complete key
     * @param depth Which bits are currently relevant
     * @return the 8 bit part of the key that apply to the current node
     */
    uint8_t get_key(int64_t key, int depth);

    /**
     * @brief Increase the node size of the current node
     * @param header The current node of the radix tree
     * @return The new node
     */
    RHeader *increase_node_size(RHeader *header);

    /**
     * @brief Decrease the node size of the current node
     * @param header The current node of the radix tree
     * @return The new node
     */
    RHeader *decrease_node_size(RHeader *header);

    /**
     * @brief Finds the longest possible byte prefix, longest possible is 7 - last insert is handled in node directly
     * @param key_a The first key
     * @param key_b The second key
     * @return How many bytes they have in common
     */
    int longest_common_prefix(int64_t key_a, int64_t key_b);

    /**
     * @brief Insert the child node into the parent node
     * @param parent The node in which will be inserted
     * @param key The key where insertion will take place
     * @param child The child pointer that will be inserted
     */
    void node_insert(RHeader *parent, uint8_t key, void *child);

    /**
     * @brief Insert the page_id and bheader into the tree
     * @param parent The node in which will be inserted
     * @param partial_key The key where insertion will take place at the current depth
     * @param key The full key
     * @param page_id The page id of the page where the data is stored
     * @param bheader The bheader where the data is stored
     */
    void node_insert(RHeader *parent, uint8_t partial_key, int64_t key, uint64_t page_id, BHeader *bheader);

    /**
     * @brief Get the next page in the tree
     * @param header The radix tree node
     * @param key The key where the next child is
     * @return A pointer to the next page
     */
    void *get_next_page(RHeader *header, uint8_t key);

    /**
     * @brief Get the value for a key
     * @param header The radix tree node
     * @param key The key where the value is stored at
     * @return The value corresponding to the key or INT64_MIN if not present
     */
    int64_t get_value_recursive(RHeader *header, int64_t key);

    /**
     * @brief Deletes a value from the tree
     * @param parent The parent radix tree node
     * @param child The child node of parent
     * @param key The key that will be deleted
     */
    void delete_reference_recursive(RHeader *parent, RHeader *child, int64_t key);

    /**
     * @brief Deletes from a node
     * @param header The node from which will be deleted
     * @param key The key where values should be deleted from
     */
    void node_delete(RHeader *header, uint8_t key);

    /**
     * @brief Get the single child of a node
     * @param header The node that only has one child
     * @return A pointer to single child
     */
    void *get_single_child(RHeader *header);

    int size_4 = 64;     /// size for node 4 in bytes
    int size_16 = 168;   /// size for node 16 in bytes
    int size_48 = 920;   /// size for node 48 in bytes
    int size_256 = 2072; /// size for node 256 in bytes

public:
    friend class RadixTreeTest;
    // Root Node
    RHeader *root = nullptr;

    RadixTree();

    /**
     * @brief Insert an element into the radixtree
     * @param key The key that will be inserted
     * @param page_id The page id of the page where the key is located
     * @param bheader The pointer to the header where the key is located
     */
    void insert(int64_t key, uint64_t page_id, BHeader *bheader);

    /**
     * @brief Delete the reference from the tree
     * @param key The key that will be deleted
     */
    void delete_reference(int64_t key);

    /**
     * @brief Get a value corresponding to the key
     * @param key The key corresponding to the value
     * @return The value
     */
    int64_t get_value(int64_t key);

    /**
     * @brief Frees every allocated node, needs to be called at the end
     */
    void destroy();
};