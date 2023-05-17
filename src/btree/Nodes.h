/**
 * @file    Nodes.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

/**
 * @brief Structure for the inner node
 */
template <int PAGE_SIZE>
struct InnerNode
{
    // 8 bytes
    Header header;
    // 4 bytes
    /// info about current capacity of node
    int current_index;
    // 4 bytes - size of keys, not child ids
    /// maximum capacity of node
    int max_size;
    // 4 bytes padding to make it the same size as outer node
    char padding[4];

    int keys[((PAGE_SIZE - 20) / 2) / 4];
    uint32_t child_ids[((PAGE_SIZE - 20) / 2) / 4];

    /**
     * @brief Constructor for the inner node
     * @param header_arg Reference to the header from which the node will be constructed
     */
    InnerNode(Header *header_arg) : header(*new(header_arg) Header(header_arg))
    {
        current_index = 0;
        max_size = ((PAGE_SIZE - 20) / 2) / 4 - 1;
        assert(max_size > 2 && "Node size is too small");
        header.inner = true;
    }

    /**
     * @brief Finds the next page specified by the key
     * @param key The key to the next page
     * @return The id of the next page
     */
    uint32_t next_page(int key)
    {
        int index = 0;
        while (index < current_index)
        {
            // If key is less than or equal to the current key, return child_id
            if (key <= keys[index])
            {
                return child_ids[index];
            }
            index++;
        }
        return child_ids[index];
    }

    /**
     * @brief Ineserting a new key and child: the child id will be inserted on the right side of the key, as this will always correspond to the created node after splitting which is the ones with higher values
     * @param child_id The id of the new child
     * @param key Key corresponding to the child
     */
    void insert(uint32_t child_id, int key)
    {
        assert(!is_full() && "Inserting into inner node when its full.");
        // find index where to insert
        int index = 0;

        while (index < current_index)
        {
            if (keys[index] < key)
            {
                index++;
            }
            else
            {
                break;
            }
        }
        // shift all keys bigger one space to the left
        for (int i = current_index; i > index; i--)
        {
            keys[i] = keys[i - 1];
            child_ids[i + 1] = child_ids[i];
        }

        // insert new values
        keys[index] = key;
        child_ids[index + 1] = child_id;
        current_index++;
    }

    /**
     * @brief Checks if the node is full
     * @return true if it is false if it is not full
     */
    bool is_full()
    {
        return current_index >= max_size;
    }
};

/**
 * @brief Structure for the outer node
 */
template <int PAGE_SIZE>
struct OuterNode
{
    // 8 bytes
    Header header;
    // 4 bytes
    /// info about current capacity of node
    int current_index;
    // 4 bytes
    /// maximum capacity of node
    int max_size;
    // 4 bytes
    /// Id of next outer leaf
    uint32_t next_lef_id;

    int keys[((PAGE_SIZE - 20) / 4) / 2];
    int values[((PAGE_SIZE - 20) / 4) / 2];

    /**
     * @brief Constructor for the outer node
     * @param header_arg Reference to the header from which the node will be constructed
     */
    OuterNode(Header *header_arg) : header(*new(header_arg) Header(header_arg))
    {
        current_index = 0;
        max_size = ((PAGE_SIZE - 20) / 4) / 2;
        assert(max_size > 2 && "Node size is too small");
        next_lef_id = 0;
        header.inner = false;
    }

    /**
     * @brief Ineserting a new key and value, can only insert when not full
     * @param key Key
     * @param value Value corresponding to the key
     */
    void insert(int key, int value)
    {
        assert(!is_full() && "Inserting into outer node when its full.");
        // find index where to insert
        int index = 0;
        while (index < current_index)
        {
            if (keys[index] < key)
            {
                index++;
            }
            else
            {
                break;
            }
        }

        // shift all keys bigger one space to the left
        for (int i = current_index; i > index; i--)
        {
            keys[i] = keys[i - 1];
            values[i] = values[i - 1];
        }

        // insert new values
        keys[index] = key;
        values[index] = value;
        current_index++;
    }

    /**
     * @brief Return the value for a key
     * @return the value
     */
    int get_value(int key)
    {
        int value = -1;
        int index = 0;
        while (index < current_index)
        {
            if (values[index] == key)
            {
                value = values[index];
                break;
            }
            index++;
        }
        return value;
    }

    /**
     * @brief Checks if the node is full
     * @return true if it is false if it is not full
     */
    bool is_full()
    {
        return current_index >= max_size;
    }
};
