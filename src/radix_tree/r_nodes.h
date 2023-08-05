/**
 * @file    r_nodes.h
 *
 * @author  Matteo Wohlrapp
 * @date    18.06.2023
 */

#pragma once

#include "../model/r_header.h"
#include "../model/r_frame.h"
#include <cassert>
#include <iostream>
#include <emmintrin.h>
#include <nmmintrin.h>

constexpr int size_4 = 64;     /// size for node 4 in bytes
constexpr int size_16 = 168;   /// size for node 16 in bytes
constexpr int size_48 = 920;   /// size for node 48 in bytes
constexpr int size_256 = 2072; /// size for node 256 in bytes

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
    RNode4(bool leaf, uint8_t depth, uint64_t key, uint16_t current_size) : header{4, leaf, depth, key, current_size}
    {
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param child the element that should be inserted under key
     */
    void insert(uint8_t key, void *child)
    {
        assert(child && "Trying to insert null pointer.");
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
     * @param bheader the bplus tree node where the value can be found
     * @return the number of bytes allocated
     */
    int insert(uint8_t key, uint64_t page_id, BHeader *bheader)
    {

        assert(header.leaf && "Inserting a new frame in a non leaf node");

        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                ((RFrame *)children[i])->page_id = page_id;
                ((RFrame *)children[i])->header = bheader;
                return 0;
            }
        }

        assert(header.current_size < 4 && "Trying to insert into full node");

        RFrame *frame = (RFrame *)malloc(16);
        frame->page_id = page_id;
        frame->header = bheader;
        keys[header.current_size] = key;
        children[header.current_size] = frame;
        header.current_size++;
        return 16;
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
     * @return the number of bytes freed
     */
    int delete_reference(uint8_t key)
    {
        int deleted_bytes = 0;
        for (int i = 0; i < header.current_size; i++)
        {
            if (keys[i] == key)
            {
                if (header.leaf)
                {
                    deleted_bytes = 16;
                }
                else
                {
                    switch (((RHeader *)children[i])->type)
                    {
                    case 4:
                        deleted_bytes = size_4;
                        break;
                    case 16:
                        deleted_bytes = size_16;
                        break;
                    case 48:
                        deleted_bytes = size_48;
                        break;
                    case 256:
                        deleted_bytes = size_256;
                        break;
                    default:
                        break;
                    }
                }
                free(children[i]);
                if (i != header.current_size - 1)
                {
                    keys[i] = keys[header.current_size - 1];
                    children[i] = children[header.current_size - 1];
                }
                header.current_size--;
                return deleted_bytes;
            }
        }
        return deleted_bytes;
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
    RNode16(bool leaf, uint8_t depth, uint64_t key, uint16_t current_size) : header{16, leaf, depth, key, current_size}
    {
    }

    /**
     * @brief Searches through the keys and returns the index, the current size if not available
     * @param key the key to look for
     * @return the index of the key
     */
    uint16_t get_index(uint8_t key)
    {
        __m128i keys_simd = _mm_loadu_si128(reinterpret_cast<const __m128i *>(keys));
        __m128i key_simd = _mm_set1_epi8(key);
        __m128i cmp = _mm_cmpeq_epi8(key_simd, keys_simd);
        int mask = (1 << header.current_size) - 1;
        int bitfield = _mm_movemask_epi8(cmp) & mask;
        if (bitfield != 0)
        {

            return __builtin_ctz(bitfield);
        }
        return header.current_size;
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param child the element that should be inserted under key
     */
    void insert(uint8_t key, void *child)
    {
        assert(child && "Trying to insert null pointer.");
        uint16_t index = get_index(key);

        if (index < header.current_size && keys[index] == key)
        {
            children[index] = child;
            return;
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
     * @param bheader the bplus tree node where the value can be found
     * @return the number of bytes allocated
     */
    int insert(uint8_t key, uint64_t page_id, BHeader *bheader)
    {

        assert(header.leaf && "Inserting a new frame in a non leaf node");

        uint16_t index = get_index(key);

        if (index < header.current_size && keys[index] == key)
        {
            ((RFrame *)children[index])->page_id = page_id;
            ((RFrame *)children[index])->header = bheader;
            return 0;
        }

        assert(header.current_size < 16 && "Trying to insert into full node");

        RFrame *frame = (RFrame *)malloc(16);
        frame->page_id = page_id;
        frame->header = bheader;
        keys[header.current_size] = key;
        children[header.current_size] = frame;
        header.current_size++;
        return 16;
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
     * @return the number of freed bytes
     */
    int delete_reference(uint8_t key)
    {
        int deleted_bytes = 0;
        uint16_t index = get_index(key);

        if (index < header.current_size && keys[index] == key)
        {
            if (header.leaf)
            {
                deleted_bytes = 16;
            }
            else
            {
                switch (((RHeader *)children[index])->type)
                {
                case 4:
                    deleted_bytes = size_4;
                    break;
                case 16:
                    deleted_bytes = size_16;
                    break;
                case 48:
                    deleted_bytes = size_48;
                    break;
                case 256:
                    deleted_bytes = size_256;
                    break;
                default:
                    break;
                }
            }
            free(children[index]);
            if (index != header.current_size - 1)
            {
                keys[index] = keys[header.current_size - 1];
                children[index] = children[header.current_size - 1];
            }
            header.current_size--;
        }

        return deleted_bytes;
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
        return header.current_size > 4;
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
    /// 8 byte part of the key, 255 means not filled
    uint8_t keys[256];
    /// Pointer to the children
    void *children[48] = {nullptr};

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     */
    RNode48(bool leaf) : header{48, leaf, 0, 0, 0}
    {
        // need to fill the node because 255 cant be initialized into every element
        std::fill_n(keys, 256, 255);
    }

    /**
     * @brief Constructor
     * @param leaf if the node is a leaf or not
     * @param depth the depth the node is in
     * @param key the key which is equal for each child until depth
     * @param current_size how many elements there are in the node currently
     */
    RNode48(bool leaf, uint8_t depth, uint64_t key, uint16_t current_size) : header{48, leaf, depth, key, current_size}
    {
        // need to fill the node because 255 cant be initialized into every element
        std::fill_n(keys, 256, 255);
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param child the element that should be inserted under key
     */
    void insert(uint8_t key, void *child)
    {
        assert(child && "Trying to insert null pointer.");
        if (keys[key] != 255)
        {
            children[keys[key]] = child;
        }
        else
        {
            assert(header.current_size < 48 && "Trying to insert into full node");
            for (int i = 0; i < 48; i++)
            {
                if (!children[i])
                {
                    children[i] = child;
                    keys[key] = i;
                    header.current_size++;
                    return;
                }
            }
        }
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param page_id the page_id where the information can be found
     * @param bheader the bplus tree node where the value can be found
     * @return the number of bytes allocated
     */
    int insert(uint8_t key, uint64_t page_id, BHeader *bheader)
    {
        assert(header.leaf && "Inserting a new frame in a non leaf node");

        if (keys[key] == 255)
        {
            assert(header.current_size < 48 && "Trying to insert into full node");
            for (int i = 0; i < 48; i++)
            {
                if (!children[i])
                {
                    RFrame *frame = (RFrame *)malloc(16);
                    frame->page_id = page_id;
                    frame->header = bheader;
                    children[i] = frame;
                    keys[key] = i;
                    header.current_size++;
                    return 16;
                }
            }
            return 0;
        }
        else
        {
            ((RFrame *)children[keys[key]])->page_id = page_id;
            ((RFrame *)children[keys[key]])->header = bheader;
            return 0;
        }
    }

    /**
     * @brief finds and returns the next node in the tree
     * @param key the key where the next node is located
     * @return the pointer to the next node
     */
    void *get_next_page(uint8_t key)
    {
        if (keys[key] == 255)
            return nullptr;
        else
            return children[keys[key]];
    }

    /**
     * @brief deletes an element from the tree
     * @brief the key that corresponds to the child that should be deleted
     * @return the number of freed bytes
     */
    int delete_reference(uint8_t key)
    {
        int deleted_bytes = 0;
        if (keys[key] != 255)
        {
            if (header.leaf)
            {
                deleted_bytes = 16;
            }
            else
            {
                switch (((RHeader *)children[keys[key]])->type)
                {
                case 4:
                    deleted_bytes = size_4;
                    break;
                case 16:
                    deleted_bytes = size_16;
                    break;
                case 48:
                    deleted_bytes = size_48;
                    break;
                case 256:
                    deleted_bytes = size_256;
                    break;
                default:
                    break;
                }
            }
            free(children[keys[key]]);
            children[keys[key]] = nullptr;
            keys[key] = 255;
            header.current_size--;
            return deleted_bytes;
        }
        return deleted_bytes;
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
            if (keys[i] != 255)
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
        return header.current_size > 16;
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
    RNode256(bool leaf, uint8_t depth, uint64_t key, uint16_t current_size) : header{256, leaf, depth, key, current_size}
    {
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param child the element that should be inserted under key
     */
    void insert(uint8_t key, void *child)
    {
        assert(child && "Trying to insert null pointer.");
        if (children[key] == nullptr)
            header.current_size++;

        children[key] = child;
    }

    /**
     * @brief insert a new element with key
     * @param key the key which identifies the value
     * @param page_id the page_id where the information can be found
     * @param bheader the bplus tree node where the value can be found
     * @return the number of bytes allocated
     */
    int insert(uint8_t key, uint64_t page_id, BHeader *bheader)
    {
        assert(header.leaf && "Inserting a new frame in a non leaf node");

        if (children[key] == nullptr)
        {
            header.current_size++;
            RFrame *frame = (RFrame *)malloc(16);
            frame->page_id = page_id;
            frame->header = bheader;
            children[key] = frame;
            return 16;
        }
        else
        {
            ((RFrame *)children[key])->page_id = page_id;
            ((RFrame *)children[key])->header = bheader;
            return 0;
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
     * @return the number of freed bytes
     */
    int delete_reference(uint8_t key)
    {
        int deleted_bytes = 0;
        if (children[key])
        {
            if (header.leaf)
            {
                deleted_bytes = 16;
            }
            else
            {
                switch (((RHeader *)children[key])->type)
                {
                case 4:
                    deleted_bytes = size_4;
                    break;
                case 16:
                    deleted_bytes = size_16;
                    break;
                case 48:
                    deleted_bytes = size_48;
                    break;
                case 256:
                    deleted_bytes = size_256;
                    break;
                default:
                    break;
                }
            }
            free(children[key]);
            children[key] = nullptr;
            header.current_size--;
            return deleted_bytes;
        }
        return deleted_bytes;
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
        return header.current_size > 48;
    }

    /**
     * @brief shows if there is enough space to insert another element
     */
    bool can_insert()
    {
        return true;
    }
};