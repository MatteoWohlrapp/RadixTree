#include "gtest/gtest.h"
#include "../src/radixtree/RadixTree.h"
#include "../src/radixtree/RNodes.h"
#include "../src/data/BufferManager.h"
#include "../src/data/StorageManager.h"
#include "../src/bplustree/BPlusTree.h"
#include <unordered_set>

constexpr int node_test_size = 96;
int buffer_size = 1000;

class RadixTreeTest : public ::testing::Test
{
    friend class RadixTree;

protected:
    RadixTree *radixtree;
    StorageManager *storage_manager;
    BufferManager *buffer_manager;
    BPlusTree<Configuration::page_size> *bplustree;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::path bitmap = "bitmap.bin";
    std::filesystem::path data = "data.bin";
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

    void SetUp() override
    {
        std::filesystem::remove(base_path / bitmap);
        std::filesystem::remove(base_path / data);
        radixtree = new RadixTree();
        storage_manager = new StorageManager(base_path, node_test_size);
        buffer_manager = new BufferManager(storage_manager, buffer_size, node_test_size);
        bplustree = new BPlusTree<node_test_size>(buffer_manager, radixtree);
    }

    void TearDown() override
    {
    }

    uint8_t get_key_test(int64_t key, int depth)
    {
        return radixtree->get_key(key, depth);
    }

    int longest_common_prefix_test(int64_t key_a, int64_t key_b)
    {
        return radixtree->longest_common_prefix(key_a, key_b);
    }

    RHeader *get_root()
    {
        return radixtree->root;
    }

    bool is_compressed(RHeader *header)
    {
        if (!header)
            return true;
        if (header->leaf)
            return true;
        else
        {
            if (header->current_size <= 1)
                return false;
        }

        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (!child->leaf)
                {
                    if (child->current_size <= 1 || !is_compressed(child))
                        return false;
                }
            }
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (!child->leaf)
                {
                    if (child->current_size <= 1 || !is_compressed(child))
                        return false;
                }
            }
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 256)
                {
                    RHeader *child = (RHeader *)node->children[node->keys[i]];
                    if (!child->leaf)
                    {
                        if (child->current_size <= 1 || !is_compressed(child))
                            return false;
                    }
                }
            }
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->children[i])
                {
                    RHeader *child = (RHeader *)node->children[i];
                    if (!child->leaf)
                    {
                        if (child->current_size <= 1 || !is_compressed(child))
                            return false;
                    }
                }
            }
        }
        break;
        default:
            break;
        }
        return true;
    }

    bool leaf_depth_correct(RHeader *header)
    {
        if (!header)
            return true;
        if (header->leaf)
            return header->depth == 8;
        else
        {
            if (header->depth == 8)
                return false;
        }

        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (!child->leaf)
                {
                    if (!leaf_depth_correct(child))
                        return false;
                }
                else
                {
                    if (child->depth != 8)
                        return false;
                }
            }
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (!child->leaf)
                {
                    if (!leaf_depth_correct(child))
                        return false;
                }
                else
                {
                    if (child->depth != 8)
                        return false;
                }
            }
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 256)
                {
                    RHeader *child = (RHeader *)node->children[node->keys[i]];
                    if (!child->leaf)
                    {
                        if (!leaf_depth_correct(child))
                            return false;
                    }
                    else
                    {
                        if (child->depth != 8)
                            return false;
                    }
                }
            }
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->children[i])
                {
                    RHeader *child = (RHeader *)node->children[i];
                    if (!child->leaf)
                    {
                        if (!leaf_depth_correct(child))
                            return false;
                    }
                    else
                    {
                        if (child->depth != 8)
                            return false;
                    }
                }
            }
        }
        break;
        default:
            break;
        }
        return true;
    }

    bool key_matches(RHeader *header)
    {
        if (!header)
            return true;

        if (header->leaf)
            return true;

        switch (header->type)
        {
        case 4:
        {
            RNode4 *node = (RNode4 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (header->depth - 1 > radixtree->longest_common_prefix(header->key, child->key) || !key_matches(child))
                {
                    logger->debug("Not matching for keys: {}, and {}; depth {}, lcp {}", header->key, child->key, header->depth, radixtree->longest_common_prefix(header->key, child->key));
                    return false;
                }
            }
        }
        break;
        case 16:
        {
            RNode16 *node = (RNode16 *)header;
            for (int i = 0; i < header->current_size; i++)
            {
                RHeader *child = (RHeader *)node->children[i];
                if (header->depth - 1 > radixtree->longest_common_prefix(header->key, child->key) || !key_matches(child))
                    return false;
            }
        }
        break;
        case 48:
        {
            RNode48 *node = (RNode48 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->keys[i] != 256)
                {
                    RHeader *child = (RHeader *)node->children[node->keys[i]];
                    if (header->depth - 1 > radixtree->longest_common_prefix(header->key, child->key) || !key_matches(child))
                        return false;
                }
            }
        }
        break;
        case 256:
        {
            RNode256 *node = (RNode256 *)header;
            for (int i = 0; i < 256; i++)
            {
                if (node->children[i])
                {
                    RHeader *child = (RHeader *)node->children[i];
                    if (header->depth - 1 > radixtree->longest_common_prefix(header->key, child->key) || !key_matches(child))
                        return false;
                }
            }
        }
        break;
        default:
            break;
        }
        return true;
    }
};

TEST_F(RadixTreeTest, GetKey)
{
    int64_t highest_bit = 72057594037927936;
    int64_t lowest_bit = 1;

    ASSERT_EQ(get_key_test(highest_bit, 8), 0);
    ASSERT_EQ(get_key_test(lowest_bit, 8), 1);

    ASSERT_EQ(get_key_test(highest_bit, 1), 1);
    ASSERT_EQ(get_key_test(lowest_bit, 1), 0);
}

TEST_F(RadixTreeTest, LongestCommonPrefix)
{

    ASSERT_EQ(longest_common_prefix_test(72057594037927936, 0), 0);

    // not the same because otherwise not added to the same node
    ASSERT_EQ(longest_common_prefix_test(1, 1), 7);

    ASSERT_EQ(longest_common_prefix_test(4294967296, 0), 3);
}

TEST_F(RadixTreeTest, IsCompressed)
{
    radixtree->insert(0, 0, 0);
    radixtree->insert(256, 0, 0);
    radixtree->insert(65536, 0, 0);
    radixtree->insert(16777216, 0, 0);
    radixtree->insert(4294967296, 0, 0);
    radixtree->insert(1099511627776, 0, 0);
    radixtree->insert(281474976710656, 0, 0);
    radixtree->insert(72057594037927936, 0, 0);

    ASSERT_TRUE(is_compressed(get_root()));

    get_root()->current_size = 1;
    ASSERT_FALSE(is_compressed(get_root()));

    get_root()->current_size = 2;
    ASSERT_TRUE(is_compressed(get_root()));

    RNode4 *node = (RNode4 *)((RNode4 *)get_root())->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];
    node->header.current_size = 1;
    ASSERT_FALSE(is_compressed(get_root()));
}

TEST_F(RadixTreeTest, LeafDepthCorrect)
{
    radixtree->insert(0, 0, 0);
    radixtree->insert(256, 0, 0);
    radixtree->insert(65536, 0, 0);
    radixtree->insert(16777216, 0, 0);
    radixtree->insert(4294967296, 0, 0);
    radixtree->insert(1099511627776, 0, 0);
    radixtree->insert(281474976710656, 0, 0);
    radixtree->insert(72057594037927936, 0, 0);

    ASSERT_TRUE(leaf_depth_correct(get_root()));

    RNode4 *node = (RNode4 *)((RNode4 *)get_root())->children[1];
    node->header.depth = 7;
    ASSERT_FALSE(leaf_depth_correct(get_root()));

    node->header.depth = 8;
    ASSERT_TRUE(leaf_depth_correct(get_root()));

    node = (RNode4 *)((RNode4 *)get_root())->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];

    node->header.leaf = true;
    ASSERT_FALSE(leaf_depth_correct(get_root()));

    node->header.leaf = false;
    ASSERT_TRUE(leaf_depth_correct(get_root()));

    node->header.depth = 8;
    ASSERT_FALSE(leaf_depth_correct(get_root()));
}

TEST_F(RadixTreeTest, KeyMatches)
{
    radixtree->insert(0, 0, 0);
    radixtree->insert(256, 0, 0);
    radixtree->insert(65536, 0, 0);
    radixtree->insert(16777216, 0, 0);
    radixtree->insert(4294967296, 0, 0);
    radixtree->insert(1099511627776, 0, 0);
    radixtree->insert(281474976710656, 0, 0);
    radixtree->insert(72057594037927936, 0, 0);

    ASSERT_TRUE(key_matches(get_root()));

    RNode4 *node = (RNode4 *)((RNode4 *)get_root())->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];
    node->header.key = 72057594037927936;
    ASSERT_FALSE(key_matches(get_root()));
}

// Isolated testing of a few use cases that I identified as correct during manual testing
TEST_F(RadixTreeTest, InsertLazyLeaf)
{
    radixtree->insert(0, 0, 0);
    radixtree->insert(256, 0, 0);
    radixtree->insert(65536, 0, 0);
    radixtree->insert(16777216, 0, 0);
    radixtree->insert(4294967296, 0, 0);
    radixtree->insert(1099511627776, 0, 0);
    radixtree->insert(281474976710656, 0, 0);
    radixtree->insert(72057594037927936, 0, 0);
    radixtree->insert(72057594054705152, 0, 0);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, InsertResizeNonLazyLeaf)
{
    radixtree->insert(1, 0, 0);
    radixtree->insert(256, 0, 0);
    radixtree->insert(65536, 0, 0);
    radixtree->insert(16777216, 0, 0);
    radixtree->insert(4294967296, 0, 0);
    radixtree->insert(1099511627776, 0, 0);
    radixtree->insert(281474976710656, 0, 0);
    radixtree->insert(72057594037927936, 0, 0);
    radixtree->insert(2, 0, 0);
    radixtree->insert(3, 0, 0);
    radixtree->insert(4, 0, 0);
    radixtree->insert(5, 0, 0);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, InsertResizeLazyLeaf)
{
    radixtree->insert(1, 0, 0);
    radixtree->insert(256, 0, 0);
    radixtree->insert(65536, 0, 0);
    radixtree->insert(16777216, 0, 0);
    radixtree->insert(4294967296, 0, 0);
    radixtree->insert(1099511627776, 0, 0);
    radixtree->insert(281474976710656, 0, 0);
    radixtree->insert(72057594037927936, 0, 0);
    radixtree->insert(72057594037927937, 0, 0);
    radixtree->insert(72057594037927938, 0, 0);
    radixtree->insert(72057594037927939, 0, 0);
    radixtree->insert(72057594037927940, 0, 0);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, InsertResizeInnerNode)
{
    radixtree->insert(1, 0, 0);
    radixtree->insert(256, 0, 0);
    radixtree->insert(65536, 0, 0);
    radixtree->insert(16777216, 0, 0);
    radixtree->insert(4294967296, 0, 0);
    radixtree->insert(1099511627776, 0, 0);
    radixtree->insert(281474976710656, 0, 0);
    radixtree->insert(72057594037927936, 0, 0);
    radixtree->insert(8589934592, 0, 0);
    radixtree->insert(12884901888, 0, 0);
    radixtree->insert(17179869184, 0, 0);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, InsertResizeRoot)
{
    radixtree->insert(1, 0, 0);
    radixtree->insert(256, 0, 0);
    radixtree->insert(65536, 0, 0);
    radixtree->insert(16777216, 0, 0);
    radixtree->insert(4294967296, 0, 0);
    radixtree->insert(1099511627776, 0, 0);
    radixtree->insert(281474976710656, 0, 0);
    radixtree->insert(72057594037927936, 0, 0);
    radixtree->insert(144115188075855872, 0, 0);
    radixtree->insert(216172782113783808, 0, 0);
    radixtree->insert(288230376151711744, 0, 0);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

// Testing get value with use cases identified above
TEST_F(RadixTreeTest, InsertLazyLeafBPlusTree)
{
    bplustree->insert(1, 1);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);
    bplustree->insert(72057594054705152, 72057594054705152);

    ASSERT_EQ(radixtree->get_value(0), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1), 1);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);
    ASSERT_EQ(radixtree->get_value(72057594054705152), 72057594054705152);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, InsertResizeNonLazyLeafBPlusTree)
{
    bplustree->insert(1, 1);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);
    bplustree->insert(2, 2);
    bplustree->insert(3, 3);
    bplustree->insert(4, 4);
    bplustree->insert(5, 5);

    ASSERT_EQ(radixtree->get_value(0), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1), 1);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);
    ASSERT_EQ(radixtree->get_value(2), 2);
    ASSERT_EQ(radixtree->get_value(3), 3);
    ASSERT_EQ(radixtree->get_value(4), 4);
    ASSERT_EQ(radixtree->get_value(5), 5);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, InsertResizeLazyLeafBPlusTree)
{
    bplustree->insert(1, 1);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);
    bplustree->insert(72057594037927937, 72057594037927937);
    bplustree->insert(72057594037927938, 72057594037927938);
    bplustree->insert(72057594037927939, 72057594037927939);
    bplustree->insert(72057594037927940, 72057594037927940);

    ASSERT_EQ(radixtree->get_value(0), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1), 1);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);
    ASSERT_EQ(radixtree->get_value(72057594037927937), 72057594037927937);
    ASSERT_EQ(radixtree->get_value(72057594037927938), 72057594037927938);
    ASSERT_EQ(radixtree->get_value(72057594037927939), 72057594037927939);
    ASSERT_EQ(radixtree->get_value(72057594037927940), 72057594037927940);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, InsertResizeInnerNodeBPlusTree)
{
    bplustree->insert(1, 1);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);
    bplustree->insert(8589934592, 8589934592);
    bplustree->insert(12884901888, 12884901888);
    bplustree->insert(17179869184, 17179869184);

    ASSERT_EQ(radixtree->get_value(0), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1), 1);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);
    ASSERT_EQ(radixtree->get_value(8589934592), 8589934592);
    ASSERT_EQ(radixtree->get_value(12884901888), 12884901888);
    ASSERT_EQ(radixtree->get_value(17179869184), 17179869184);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, InsertResizeRootBPlusTree)
{
    bplustree->insert(1, 1);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);
    bplustree->insert(144115188075855872, 144115188075855872);
    bplustree->insert(216172782113783808, 216172782113783808);
    bplustree->insert(288230376151711744, 288230376151711744);

    ASSERT_EQ(radixtree->get_value(0), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1), 1);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);
    ASSERT_EQ(radixtree->get_value(144115188075855872), 144115188075855872);
    ASSERT_EQ(radixtree->get_value(216172782113783808), 216172782113783808);
    ASSERT_EQ(radixtree->get_value(288230376151711744), 288230376151711744);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

// Testing insert and get value with a random tests, first with seed, then completely random
TEST_F(RadixTreeTest, InsertWithSeed42)
{
    std::mt19937 generator(42); // 42 is the seed value
    std::uniform_int_distribution<int64_t> dist(INT64_MIN + 1, INT64_MAX);
    std::unordered_set<int64_t> unique_values;
    int64_t values[1000];

    for (int i = 0; i < 1000; i++)
    {
        int64_t value = dist(generator); // generate a random number

        // Ensure we have a unique value, if not, generate another one
        while (unique_values.count(value))
        {
            value = dist(generator);
        }

        unique_values.insert(value);
        values[i] = value;
        logger->debug("Inserting {}", value);
        bplustree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        logger->debug("i: {}, value: {}", i, values[i]);
        ASSERT_EQ(radixtree->get_value(values[i]), values[i]);
        ASSERT_TRUE(is_compressed(get_root()));
        ASSERT_TRUE(leaf_depth_correct(get_root()));
        ASSERT_TRUE(key_matches(get_root()));
    }

    // testing if values are not in there
    for (int i = 0; i < 1000; i++)
    {
        int64_t value = dist(generator); // generate a random number

        while (unique_values.count(value))
        {
            value = dist(generator);
        }
        ASSERT_EQ(radixtree->get_value(value), INT64_MIN);
        ASSERT_TRUE(is_compressed(get_root()));
        ASSERT_TRUE(leaf_depth_correct(get_root()));
        ASSERT_TRUE(key_matches(get_root()));
    }
}

TEST_F(RadixTreeTest, InsertRandom)
{
    std::random_device rd;
    std::uniform_int_distribution<int64_t> dist(INT64_MIN + 1, INT64_MAX);
    std::unordered_set<int64_t> unique_values;
    int64_t values[1000];

    for (int i = 0; i < 1000; i++)
    {
        int64_t value = dist(rd); // generate a random number

        // Ensure we have a unique value, if not, generate another one
        while (unique_values.count(value))
        {
            value = dist(rd);
        }

        unique_values.insert(value);
        values[i] = value;
        logger->debug("Inserting {}", value);
        bplustree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        logger->debug("i: {}, value: {}", i, values[i]);
        ASSERT_EQ(radixtree->get_value(values[i]), values[i]);
        ASSERT_TRUE(is_compressed(get_root()));
        ASSERT_TRUE(leaf_depth_correct(get_root()));
        ASSERT_TRUE(key_matches(get_root()));
    }

    // testing if values are not in there
    for (int i = 0; i < 1000; i++)
    {
        int64_t value = dist(rd); // generate a random number

        while (unique_values.count(value))
        {
            value = dist(rd);
        }
        ASSERT_EQ(radixtree->get_value(value), INT64_MIN);
        ASSERT_TRUE(is_compressed(get_root()));
        ASSERT_TRUE(leaf_depth_correct(get_root()));
        ASSERT_TRUE(key_matches(get_root()));
    }
}

// Isolated testing of a few use cases that I identified as correct during manual testing
TEST_F(RadixTreeTest, DeleteWithoutResizing)
{
    bplustree->insert(0, 0);
    bplustree->insert(1, 1);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);

    ASSERT_EQ(radixtree->get_value(0), 0);

    bplustree->delete_pair(0);

    ASSERT_EQ(radixtree->get_value(0), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1), 1);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, DeleteWithResizing)
{
    bplustree->insert(0, 0);
    bplustree->insert(1, 1);
    bplustree->insert(2, 2);
    bplustree->insert(3, 3);
    bplustree->insert(4, 4);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);

    ASSERT_EQ(radixtree->get_value(2), 2);
    ASSERT_EQ(radixtree->get_value(3), 3);
    ASSERT_EQ(radixtree->get_value(4), 4);

    bplustree->delete_pair(2);
    bplustree->delete_pair(3);
    bplustree->delete_pair(4);

    ASSERT_EQ(radixtree->get_value(0), 0);
    ASSERT_EQ(radixtree->get_value(1), 1);
    ASSERT_EQ(radixtree->get_value(2), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(3), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(4), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, DeleteWithRemovalOfNode)
{
    bplustree->insert(0, 0);
    bplustree->insert(1, 1);
    bplustree->insert(2, 2);
    bplustree->insert(3, 3);
    bplustree->insert(4, 4);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);

    ASSERT_EQ(radixtree->get_value(2), 2);
    ASSERT_EQ(radixtree->get_value(3), 3);
    ASSERT_EQ(radixtree->get_value(4), 4);
    ASSERT_EQ(radixtree->get_value(0), 0);
    ASSERT_EQ(radixtree->get_value(1), 1);

    bplustree->delete_pair(2);
    bplustree->delete_pair(3);
    bplustree->delete_pair(4);
    bplustree->delete_pair(0);
    bplustree->delete_pair(1);

    ASSERT_EQ(radixtree->get_value(0), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(2), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(3), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(4), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, DeleteWithRemovalOfNodeFurtherUpTheTree)
{
    bplustree->insert(0, 0);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(4294967297, 4294967297);
    bplustree->insert(4294967298, 4294967298);
    bplustree->insert(4294967299, 4294967299);
    bplustree->insert(4294967300, 4294967300);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);

    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(4294967297), 4294967297);
    ASSERT_EQ(radixtree->get_value(4294967298), 4294967298);
    ASSERT_EQ(radixtree->get_value(4294967299), 4294967299);
    ASSERT_EQ(radixtree->get_value(4294967300), 4294967300);

    bplustree->delete_pair(4294967296);
    bplustree->delete_pair(4294967297);
    bplustree->delete_pair(4294967298);
    bplustree->delete_pair(4294967299);
    bplustree->delete_pair(4294967300);

    ASSERT_EQ(radixtree->get_value(0), 0);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(4294967297), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(4294967298), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(4294967299), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(4294967300), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, DeleteWithDecreaseAndDeleteOfInnerNode)
{
    bplustree->insert(0, 0);
    bplustree->insert(256, 256);
    bplustree->insert(65536, 65536);
    bplustree->insert(16777216, 16777216);
    bplustree->insert(4294967296, 4294967296);
    bplustree->insert(8589934592, 8589934592);
    bplustree->insert(12884901888, 12884901888);
    bplustree->insert(17179869184, 17179869184);
    bplustree->insert(21474836480, 21474836480);
    bplustree->insert(1099511627776, 1099511627776);
    bplustree->insert(281474976710656, 281474976710656);
    bplustree->insert(72057594037927936, 72057594037927936);

    ASSERT_EQ(radixtree->get_value(4294967296), 4294967296);
    ASSERT_EQ(radixtree->get_value(8589934592), 8589934592);
    ASSERT_EQ(radixtree->get_value(12884901888), 12884901888);
    ASSERT_EQ(radixtree->get_value(17179869184), 17179869184);
    ASSERT_EQ(radixtree->get_value(21474836480), 21474836480);

    bplustree->delete_pair(4294967296);
    bplustree->delete_pair(8589934592);
    bplustree->delete_pair(12884901888);
    bplustree->delete_pair(17179869184);
    bplustree->delete_pair(21474836480);

    ASSERT_EQ(radixtree->get_value(0), 0);
    ASSERT_EQ(radixtree->get_value(256), 256);
    ASSERT_EQ(radixtree->get_value(65536), 65536);
    ASSERT_EQ(radixtree->get_value(16777216), 16777216);
    ASSERT_EQ(radixtree->get_value(4294967296), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(8589934592), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(12884901888), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(17179869184), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(21474836480), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1099511627776), 1099511627776);
    ASSERT_EQ(radixtree->get_value(281474976710656), 281474976710656);
    ASSERT_EQ(radixtree->get_value(72057594037927936), 72057594037927936);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));
}

TEST_F(RadixTreeTest, DeleteFromLeafRoot)
{
    bplustree->insert(0, 0);
    bplustree->insert(1, 1);
    bplustree->insert(2, 2);
    bplustree->insert(3, 3);
    bplustree->insert(4, 4);

    ASSERT_EQ(radixtree->get_value(0), 0);
    ASSERT_EQ(radixtree->get_value(1), 1);
    ASSERT_EQ(radixtree->get_value(2), 2);
    ASSERT_EQ(radixtree->get_value(3), 3);
    ASSERT_EQ(radixtree->get_value(4), 4);

    bplustree->delete_pair(0);
    bplustree->delete_pair(1);
    bplustree->delete_pair(2);
    bplustree->delete_pair(3);

    ASSERT_EQ(radixtree->get_value(0), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(2), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(3), INT64_MIN);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));

    bplustree->delete_pair(4);
    ASSERT_EQ(radixtree->get_value(4), INT64_MIN);
}

TEST_F(RadixTreeTest, DeleteWithDepthTwo)
{
    bplustree->insert(256, 256);
    bplustree->insert(0, 0);
    bplustree->insert(1, 1);
    bplustree->insert(2, 2);
    bplustree->insert(3, 3);
    bplustree->insert(4, 4);

    ASSERT_EQ(radixtree->get_value(0), 0);
    ASSERT_EQ(radixtree->get_value(1), 1);
    ASSERT_EQ(radixtree->get_value(2), 2);
    ASSERT_EQ(radixtree->get_value(3), 3);
    ASSERT_EQ(radixtree->get_value(4), 4);

    bplustree->delete_pair(0);
    bplustree->delete_pair(1);
    bplustree->delete_pair(2);
    bplustree->delete_pair(3);

    ASSERT_EQ(radixtree->get_value(0), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(1), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(2), INT64_MIN);
    ASSERT_EQ(radixtree->get_value(3), INT64_MIN);

    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));

    bplustree->delete_pair(4);
    ASSERT_EQ(radixtree->get_value(4), INT64_MIN);
}

// Testing insert and get value with a random tests, first with seed, then completely random
TEST_F(RadixTreeTest, InsertAndDeleteWithSeed42)
{
    std::mt19937 generator(42); // 42 is the seed value
    std::uniform_int_distribution<int64_t> dist(INT64_MIN + 1, INT64_MAX);
    std::unordered_set<int64_t> unique_values;
    int64_t values[1000];

    for (int i = 0; i < 1000; i++)
    {
        int64_t value = dist(generator); // generate a random number

        // Ensure we have a unique value, if not, generate another one
        while (unique_values.count(value))
        {
            value = dist(generator);
        }

        unique_values.insert(value);
        values[i] = value;
        bplustree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        ASSERT_EQ(radixtree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));

    // delete 500 elements
    for (int i = 0; i < 500; i++)
    {
        int64_t value = values[i];
        unique_values.erase(value);
        bplustree->delete_pair(value);
    }

    for (int i = 0; i < 500; i++)
    {
        ASSERT_EQ(radixtree->get_value(values[i]), INT64_MIN);
    }
    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));

    // insert elements again
    for (int i = 0; i < 500; i++)
    {
        int64_t value = dist(generator); // generate a random number

        // Ensure we have a unique value, if not, generate another one
        while (unique_values.count(value))
        {
            value = dist(generator);
        }

        unique_values.insert(value);
        values[i] = value;
        bplustree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        ASSERT_EQ(radixtree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));

    // delete all
    for (int i = 0; i < 1000; i++)
    {
        int64_t value = values[i];
        unique_values.erase(value);
        bplustree->delete_pair(value);

        ASSERT_TRUE(is_compressed(get_root()));
        ASSERT_TRUE(leaf_depth_correct(get_root()));
        ASSERT_TRUE(key_matches(get_root()));
    }

    ASSERT_FALSE(radixtree->root);
}

TEST_F(RadixTreeTest, InsertAndDeleteRandom)
{
    std::random_device rd;
    std::uniform_int_distribution<int64_t> dist(INT64_MIN + 1, INT64_MAX);
    std::unordered_set<int64_t> unique_values;
    int64_t values[1000];

    for (int i = 0; i < 1000; i++)
    {
        int64_t value = dist(rd); // generate a random number

        // Ensure we have a unique value, if not, generate another one
        while (unique_values.count(value))
        {
            value = dist(rd);
        }

        unique_values.insert(value);
        values[i] = value;
        bplustree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        ASSERT_EQ(radixtree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));

    // delete 500 elements
    for (int i = 0; i < 500; i++)
    {
        int64_t value = values[i];
        unique_values.erase(value);
        bplustree->delete_pair(value);
    }

    for (int i = 0; i < 500; i++)
    {
        ASSERT_EQ(radixtree->get_value(values[i]), INT64_MIN);
    }
    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));

    // insert elements again
    for (int i = 0; i < 500; i++)
    {
        int64_t value = dist(rd); // generate a random number

        // Ensure we have a unique value, if not, generate another one
        while (unique_values.count(value))
        {
            value = dist(rd);
        }

        unique_values.insert(value);
        values[i] = value;
        bplustree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        ASSERT_EQ(radixtree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_compressed(get_root()));
    ASSERT_TRUE(leaf_depth_correct(get_root()));
    ASSERT_TRUE(key_matches(get_root()));

    // delete all
    for (int i = 0; i < 1000; i++)
    {
        int64_t value = values[i];
        unique_values.erase(value);
        bplustree->delete_pair(value);

        ASSERT_TRUE(is_compressed(get_root()));
        ASSERT_TRUE(leaf_depth_correct(get_root()));
        ASSERT_TRUE(key_matches(get_root()));
    }

    ASSERT_FALSE(radixtree->root);
}