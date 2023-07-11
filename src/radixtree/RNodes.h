/**
 * @file    RNodes.h
 *
 * @author  Matteo Wohlrapp
 * @date    18.06.2023
 */

#pragma once

#include "../model/RHeader.h"
#include "../model/RFrame.h"
#include <cassert>
#include <iostream>

/**
 * @brief Structure for the node with size of 4
 */
struct RNode4
{
    RHeader header;
    /// 8 byte part of the key
    uint8_t keys[4];
    /// Pointer to the children
    void *children[4];

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     */
    RNode4(bool leaf) : header{4, leaf, 0, 0, 0}
    {
    }

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     * @param depth the depth the node is in
     * @param key the key which is equal for each child until depth
     * @param current_size how many elements there are in the node currently
     */
    RNode4(bool leaf, uint8_t depth, int64_t key, uint16_t current_size) : header{4, leaf, depth, key, current_size}
    {
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param child the element that should be inserted under key
     */
    void insert(uint8_t key, void *child)
    {

        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                children[i] = child;
                return;
            }
        }
        assert(header.current_size < 4 && "Trying to insert into full node");

        keys[header.current_size] = key;
        children[header.current_size] = child;

        header.current_size++;
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param page_id the page_id where the information can be found
     * @param bheader the btree node where the value can be found
     */
    void insert(uint8_t key, uint64_t page_id, BHeader *bheader)
    {

        assert(header.leaf && "Inserting a new frame in a non leaf node");

        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                ((RFrame *)children[i])->page_id = page_id;
                ((RFrame *)children[i])->header = bheader;
                return;
            }
        }

        assert(header.current_size < 4 && "Trying to insert into full node");

        RFrame *frame = (RFrame *)malloc(16);
        frame->page_id = page_id;
        frame->header = bheader;
        keys[header.current_size] = key;
        children[header.current_size] = frame;
        header.current_size++;
    }

    /**
     * @brief finds and returns the next node in the tree
     * @param key the key where the next node is located
     * @return the pointer to the next node
     */
    void *get_next_page(uint8_t key)
    {
        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                return children[i];
            }
        }
        return nullptr;
    }

    /**
     * @brief deletes an element from the tree
     * @brief the key that corresponds to the child that should be deleted
     */
    void delete_pair(uint8_t key)
    {
        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                free(children[i]);
                keys[i] = keys[header.current_size - 1];
                children[i] = children[header.current_size - 1];
                header.current_size--;
                return;
            }
        }
    }

    /**
     * @brief Returns the only element of that node
     * @return a pointer to the only element in the node
     */
    void *get_single_child()
    {
        assert(header.current_size == 1 && "More or less than 1 child left.");

        return children[0];
    }

    /**
     * @brief shows if there is enough space to insert another element
     */
    bool can_insert()
    {
        return header.current_size < 4;
    }

    /**
     * @brief Checks if you can delete from the node
     * @return true if an element can be deleted, false if not
     */
    bool can_delete()
    {
        return header.current_size > 0;
    }
};

/**
 * @brief Structure for the node with size of 16
 */
struct RNode16
{
    RHeader header;

    /// 8 byte part of the key
    uint8_t keys[16];

    /// Pointer to the children
    void *children[16];

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     */
    RNode16(bool leaf) : header{16, leaf, 0, 0, 0}
    {
    }

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     * @param depth the depth the node is in
     * @param key the key which is equal for each child until depth
     * @param current_size how many elements there are in the node currently
     */
    RNode16(bool leaf, uint8_t depth, int64_t key, uint16_t current_size) : header{16, leaf, depth, key, current_size}
    {
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param child the element that should be inserted under key
     */
    void insert(uint8_t key, void *child)
    {

        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                children[i] = child;
                return;
            }
        }
        assert(header.current_size < 16 && "Trying to insert into full node");

        keys[header.current_size] = key;
        children[header.current_size] = child;

        header.current_size++;
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param page_id the page_id where the information can be found
     * @param bheader the btree node where the value can be found
     */
    void insert(uint8_t key, uint64_t page_id, BHeader *bheader)
    {

        assert(header.leaf && "Inserting a new frame in a non leaf node");

        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                ((RFrame *)children[i])->page_id = page_id;
                ((RFrame *)children[i])->header = bheader;
                return;
            }
        }
        assert(header.current_size < 16 && "Trying to insert into full node");

        RFrame *frame = (RFrame *)malloc(16);
        frame->page_id = page_id;
        frame->header = bheader;
        keys[header.current_size] = key;
        children[header.current_size] = frame;
        header.current_size++;
    }

    /**
     * @brief finds and returns the next node in the tree
     * @param key the key where the next node is located
     * @return the pointer to the next node
     */
    void *get_next_page(uint8_t key)
    {
        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                return children[i];
            }
        }
        return nullptr;
    }

    /**
     * @brief deletes an element from the tree
     * @brief the key that corresponds to the child that should be deleted
     */
    void delete_pair(uint8_t key)
    {
        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                free(children[i]);
                if (i != header.current_size - 1)
                {
                    keys[i] = keys[header.current_size - 1];
                    children[i] = children[header.current_size - 1];
                }
                header.current_size--;
                return;
            }
        }
    }

    /**
     * @brief Returns the only element of that node
     * @return a pointer to the only element in the node
     */
    void *get_single_child()
    {
        assert(header.current_size == 1 && "More or less than 1 child left.");

        return children[0];
    }

    /**
     * @brief Checks if you can delete from the node
     * @return true if an element can be deleted, false if not
     */
    bool can_delete()
    {
        return header.current_size > 2;
    }

    /**
     * @brief shows if there is enough space to insert another element
     */
    bool can_insert()
    {
        return header.current_size < 16;
    }
};

/**
 * @brief Structure for the node with size of 48
 */
struct RNode48
{
    RHeader header;
    /// 8 byte part of the key
    uint16_t keys[256];
    /// Pointer to the children
    void *children[48];

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     */
    RNode48(bool leaf) : header{48, leaf, 0, 0, 0}
    {
        std::fill_n(keys, 256, 256);
    }

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     * @param depth the depth the node is in
     * @param key the key which is equal for each child until depth
     * @param current_size how many elements there are in the node currently
     */
    RNode48(bool leaf, uint8_t depth, int64_t key, uint16_t current_size) : header{48, leaf, depth, key, current_size}
    {
        // need to fill the node because 256 cant be initialized into every element
        std::fill_n(keys, 256, 256);
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param child the element that should be inserted under key
     */
    void insert(uint8_t key, void *child)
    {

        if (keys[key] != 256)
        {
            children[keys[key]] = child;
        }
        else
        {
            assert(header.current_size < 48 && "Trying to insert into full node");
            keys[key] = header.current_size;
            children[header.current_size] = child;
            header.current_size++;
        }
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param page_id the page_id where the information can be found
     * @param bheader the btree node where the value can be found
     */
    void insert(uint8_t key, uint64_t page_id, BHeader *bheader)
    {
        assert(header.leaf && "Inserting a new frame in a non leaf node");

        if (keys[key] == 256)
        {
            assert(header.current_size < 48 && "Trying to insert into full node");
            RFrame *frame = (RFrame *)malloc(16);
            frame->page_id = page_id;
            frame->header = bheader;
            children[header.current_size] = frame;
            keys[key] = header.current_size;
            header.current_size++;
        }
        else
        {
            ((RFrame *)children[keys[key]])->page_id = page_id;
            ((RFrame *)children[keys[key]])->header = bheader;
        }
    }

    /**
     * @brief finds and returns the next node in the tree
     * @param key the key where the next node is located
     * @return the pointer to the next node
     */
    void *get_next_page(uint8_t key)
    {
        if (keys[key] == 256)
            return nullptr;
        else
            return children[keys[key]];
    }

    /**
     * @brief deletes an element from the tree
     * @brief the key that corresponds to the child that should be deleted
     */
    void delete_pair(uint8_t key)
    {
        if (keys[key] != 256)
        {
            free(children[keys[key]]);
            keys[key] = 256;
            header.current_size--;
            return;
        }
    }

    /**
     * @brief Returns the only element of that node
     * @return a pointer to the only element in the node
     */
    void *get_single_child()
    {
        assert(header.current_size == 1 && "More or less than 1 child left.");

        for (int i = 0; i < 256; i++)
        {
            if (keys[i] != 256)
                return children[keys[i]];
        }
        return nullptr;
    }

    /**
     * @brief Checks if you can delete from the node
     * @return true if an element can be deleted, false if not
     */
    bool can_delete()
    {
        return header.current_size > 8;
    }

    /**
     * @brief shows if there is enough space to insert another element
     */
    bool can_insert()
    {
        return header.current_size < 48;
    }
};

/**
 * @brief Structure for the node with size of 256
 */
struct RNode256
{
    RHeader header;
    /// Pointer to the children
    void *children[256] = {nullptr};

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     */
    RNode256(bool leaf) : header{256, leaf, 0, 0, 0}
    {
    }

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     * @param depth the depth the node is in
     * @param key the key which is equal for each child until depth
     * @param current_size how many elements there are in the node currently
     */
    RNode256(bool leaf, uint8_t depth, int64_t key, uint16_t current_size) : header{256, leaf, depth, key, current_size}
    {
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param child the element that should be inserted under key
     */
    void insert(uint8_t key, void *child)
    {

        if (children[key] == nullptr)
            header.current_size++;

        children[key] = child;
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param page_id the page_id where the information can be found
     * @param bheader the btree node where the value can be found
     */
    void insert(uint8_t key, uint64_t page_id, BHeader *bheader)
    {
        assert(header.leaf && "Inserting a new frame in a non leaf node");

        if (children[key] == nullptr)
        {
            header.current_size++;
            RFrame *frame = (RFrame *)malloc(16);
            frame->page_id = page_id;
            frame->header = bheader;
            children[key] = frame;
        }
        else
        {
            ((RFrame *)children[key])->page_id = page_id;
            ((RFrame *)children[key])->header = bheader;
        }
    }

    /**
     * @brief finds and returns the next node in the tree
     * @param key the key where the next node is located
     * @return the pointer to the next node
     */
    void *get_next_page(uint8_t key)
    {
        return children[key];
    }

    /**
     * @brief deletes an element from the tree
     * @brief the key that corresponds to the child that should be deleted
     */
    void delete_pair(uint8_t key)
    {
        if (children[key])
        {
            free(children[key]);
            children[key] = nullptr;
            header.current_size--;
            return;
        }
    }

    /**
     * @brief Returns the only element of that node
     * @return a pointer to the only element in the node
     */
    void *get_single_child()
    {
        assert(header.current_size == 1 && "More or less than 1 child left.");

        for (int i = 0; i < 256; i++)
        {
            if (children[i])
                return children[i];
        }
        return nullptr;
    }

    /**
     * @brief Checks if you can delete from the node
     * @return true if an element can be deleted, false if not
     */
    bool can_delete()
    {
        return header.current_size > 44;
    }

    /**
     * @brief shows if there is enough space to insert another element
     */
    bool can_insert()
    {
        return header.current_size < 256;
    }
};