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
    // 12 bytes
    Header header;
    // 4 bytes
    /// info about current capacity of node
    int current_index;
    // 4 bytes - size of keys, not child ids
    /// maximum capacity of node
    int max_size;
    // 4 bytes padding to make it the same size as outer node
    char padding[4];

    int64_t keys[((PAGE_SIZE - 24) / 2) / 8];
    uint64_t child_ids[((PAGE_SIZE - 24) / 2) / 8];

    /**
     * @brief Constructor for the inner node
     */
    InnerNode()
    {
        header.inner = true;
        current_index = 0;
        max_size = ((PAGE_SIZE - 24) / 2) / 8 - 1;
        assert(max_size > 2 && "Node size is too small");
    }

    /**
     * @brief Finds the next page specified by the key
     * @param key The key to the next page
     * @return The id of the next page
     */
    uint64_t next_page(int64_t key)
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
    void insert(uint64_t child_id, int64_t key)
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
    // 12 bytes
    Header header;
    // 4 bytes
    /// info about current capacity of node
    int current_index;
    // 4 bytes
    /// maximum capacity of node
    int max_size;
    // 4 bytes
    /// Id of next outer leaf
    uint64_t next_lef_id;

    int keys[((PAGE_SIZE - 24) / 2) / 8];
    int values[((PAGE_SIZE - 24) / 2) / 8];

    /**
     * @brief Constructor for the outer node
     */
    OuterNode()
    {
        header.inner = false;
        current_index = 0;
        max_size = ((PAGE_SIZE - 24) / 2) / 8;
        assert(max_size > 2 && "Node size is too small");
        next_lef_id = 0;
    }

    /**
     * @brief Ineserting a new key and value, can only insert when not full
     * @param key Key
     * @param value Value corresponding to the key
     */
    void insert(int64_t key, int64_t value)
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
    int64_t get_value(int64_t key)
    {
        int index = 0;
        while (index < current_index)
        {
            if (keys[index] == key)
            {
                return values[index];
            }
            index++;
        }
        return -1;
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
