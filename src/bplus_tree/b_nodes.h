/**
 * @file    b_nodes.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

/**
 * @brief Structure for the inner node
 */
template <int PAGE_SIZE>
struct BInnerNode
{
    // 16 bytes
    BHeader header;
    // 4 bytes
    /// info about current capacity of node
    int current_index;
    // 4 bytes - size of keys, not child ids
    /// maximum capacity of node
    int max_size;
    // 4 bytes padding to make it the same size as outer node
    char padding[8];

    int64_t keys[((PAGE_SIZE - 32) / 2) / 8];
    uint64_t child_ids[((PAGE_SIZE - 32) / 2) / 8];

    /**
     * @brief Constructor for the inner node
     */
    BInnerNode()
    {
        header.inner = true;
        current_index = 0;
        max_size = ((PAGE_SIZE - 32) / 2) / 8 - 1;
        assert(max_size > 2 && "Node size is too small");
    }

    int binary_search(int64_t key)
    {
        int left = 0, right = current_index;

        while (left < right)
        {
            int middle = left + (right - left) / 2;

            if (keys[middle] < key)
                left = middle + 1;
            else
                right = middle;
        }
        return left;
    }

    /**
     * @brief Finds the next page specified by the key
     * @param key The key to the next page
     * @return The id of the next page
     */
    uint64_t next_page(int64_t key)
    {
        int index = binary_search(key);

        return child_ids[index];
    }

    /**
     * @brief Ineserting a new key and child: the child id will be inserted on the right side of the key, as this will always correspond to the created node after splitting which is the ones with higher values
     * @param child_id The id of the new child
     * @param key Key corresponding to the child
     */
    void insert(int64_t key, uint64_t child_id)
    {
        assert(!is_full() && "Inserting into inner node when its full.");
        // find index where to insert
        int index = binary_search(key);;

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
     * @brief Inserting a new key and child at the first position -> child_ids[0], keys[0]
     * @param key The key to insert
     * @param child_id The child id that should be inserted
     */
    void insert_first(int64_t key, uint64_t child_id)
    {
        child_ids[current_index + 1] = child_ids[current_index];
        for (int i = current_index; i > 0; i--)
        {
            keys[i] = keys[i - 1];
            child_ids[i] = child_ids[i - 1];
        }

        keys[0] = key;
        child_ids[0] = child_id;
        current_index++;
    }

    /**
     * @brief Ineserting a key and child pair: the child id will be deleted on the right side of the key
     * @param key The key to delete
     */
    void delete_pair(int64_t key)
    {
        // we always delete key and the right child because the new elements will be in the left node from the key
        int index = binary_search(key);;

        for (int i = index + 1; i < current_index; i++)
        {
            keys[i - 1] = keys[i];
            child_ids[i] = child_ids[i + 1];
        }
        current_index--;
    }

    /**
     * @brief Deleting the first key and value at index 0
     */
    void delete_first_pair()
    {
        for (int i = 1; i < current_index; i++)
        {
            keys[i - 1] = keys[i];
            child_ids[i - 1] = child_ids[i];
        }
        child_ids[current_index - 1] = child_ids[current_index];
        current_index--;
    }

    /**
     * @brief Checks the keys if the given key is included
     * @param key The key to check if contained
     */
    bool contains(int64_t key)
    {
        int index = binary_search(key);;

        return index != current_index && keys[index] == key;
    }

    /**
     * @brief Exchanging a key with another key
     * @param key The current key contained
     * @param exchange_key The new key that replaces the current one
     */
    void exchange(int64_t key, int64_t exchange_key)
    {
        int index = binary_search(key);
        keys[index] = exchange_key;
    }

    /**
     * @brief Checks if the node is full
     * @return true if it is false if it is not full
     */
    bool is_full()
    {
        return current_index >= max_size;
    }

    /**
     * @brief Checks if you can delete from the node
     * @return true if an element can be deleted, false if not
     */
    bool can_delete()
    {
        return current_index >= (max_size + 1) / 2;
    }

    /**
     * @brief Checks if a node is empty
     * @return true if it is empty, false if not
     */
    bool is_too_empty()
    {
        return current_index < ((max_size + 1) / 2) - 1;
    }
};

/**
 * @brief Structure for the outer node
 */
template <int PAGE_SIZE>
struct BOuterNode
{
    // 16 bytes
    BHeader header;
    // 4 bytes
    /// info about current capacity of node
    int current_index;
    // 4 bytes
    /// maximum capacity of node
    int max_size;
    // 8 bytes
    /// Id of next outer leaf
    uint64_t next_lef_id;

    int64_t keys[((PAGE_SIZE - 32) / 2) / 8];
    int64_t values[((PAGE_SIZE - 32) / 2) / 8];

    /**
     * @brief Constructor for the outer node
     */
    BOuterNode()
    {
        header.inner = false;
        current_index = 0;
        max_size = ((PAGE_SIZE - 32) / 2) / 8;
        assert(max_size > 2 && "Node size is too small");
        next_lef_id = 0;
    }

    int binary_search(int64_t key)
    {
        int left = 0, right = current_index;

        while (left < right)
        {
            int middle = left + (right - left) / 2;

            if (keys[middle] < key)
                left = middle + 1;
            else
                right = middle;
        }
        return left;
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
        int index = binary_search(key);
        
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
     * @brief Deleting key and value pair
     * @param key Key that should be deleted
     */
    void delete_pair(int64_t key)
    {
        int index = binary_search(key);

        for (int i = index + 1; i < current_index; i++)
        {
            keys[i - 1] = keys[i];
            values[i - 1] = values[i];
        }
        current_index--;
    }

    /**
     * @brief Return the value for a key, if key is not found, minimum number is returned
     * @return the value
     */
    int64_t get_value(int64_t key)
    {
        int index = binary_search(key);

        if (index != current_index && keys[index] == key)
        {
            return values[index];
        }

        return INT64_MIN;
    }

    /**
     * @brief Checks if the node is full
     * @return true if it is false if it is not full
     */
    bool is_full()
    {
        return current_index >= max_size;
    }

    /**
     * @brief Checks if you can delete from the node
     * @return true if an element can be deleted, false if not
     */
    bool can_delete()
    {
        return current_index > max_size / 2;
    }

    /**
     * @brief Checks if a node is empty
     * @return true if it is empty, false if not
     */
    bool is_too_empty()
    {
        return current_index < max_size / 2;
    }
};
