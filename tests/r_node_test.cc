#include "gtest/gtest.h"
#include "../src/radix_tree/radix_tree.h"
#include "../src/radix_tree/r_nodes.h"

class RNodeTest : public ::testing::Test
{

protected:
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

    // sizes for nodes
    int size_4 = 64;
    int size_16 = 168;
    int size_48 = 920;
    int size_256 = 2072;

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(RNodeTest, Constructors)
{
    RHeader *header4 = (RHeader *)malloc(size_4);
    new (header4) RNode4(true, 2, 3, 4);
    ASSERT_EQ(header4->leaf, true);
    ASSERT_EQ(header4->type, 4);
    ASSERT_EQ(header4->depth, 2);
    ASSERT_EQ(header4->key, 3);
    ASSERT_EQ(header4->current_size, 4);

    RHeader *header16 = (RHeader *)malloc(size_16);
    new (header16) RNode16(true, 2, 3, 4);
    ASSERT_EQ(header16->leaf, true);
    ASSERT_EQ(header16->type, 16);
    ASSERT_EQ(header16->depth, 2);
    ASSERT_EQ(header16->key, 3);
    ASSERT_EQ(header16->current_size, 4);

    RHeader *header48 = (RHeader *)malloc(size_48);
    new (header48) RNode48(true, 2, 3, 4);
    ASSERT_EQ(header48->leaf, true);
    ASSERT_EQ(header48->type, 48);
    ASSERT_EQ(header48->depth, 2);
    ASSERT_EQ(header48->key, 3);
    ASSERT_EQ(header48->current_size, 4);

    RHeader *header256 = (RHeader *)malloc(size_256);
    new (header256) RNode256(true, 2, 3, 4);
    ASSERT_EQ(header256->leaf, true);
    ASSERT_EQ(header256->type, 256);
    ASSERT_EQ(header256->depth, 2);
    ASSERT_EQ(header256->key, 3);
    ASSERT_EQ(header256->current_size, 4);
}

TEST_F(RNodeTest, Insert4)
{
    RHeader *header = (RHeader *)malloc(size_4);
    RNode4 *node = new (header) RNode4(true, 0, 0, 0);

    void *ptr = (void *)1;
    node->insert(1, ptr);
    ASSERT_EQ(header->current_size, 1);

    ptr = (void *)2;
    node->insert(2, ptr);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->keys[0], 1);
    ASSERT_EQ(node->keys[1], 2);
    ASSERT_EQ(node->children[0], (void *)1);
    ASSERT_EQ(node->children[1], (void *)2);

    node->insert(1, ptr);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->children[0], (void *)2);

    ASSERT_TRUE(node->can_insert());

    node->insert(3, ptr);
    node->insert(4, ptr);

    ASSERT_FALSE(node->can_insert());
}

TEST_F(RNodeTest, Insert16)
{
    RHeader *header = (RHeader *)malloc(size_16);
    RNode16 *node = new (header) RNode16(true, 0, 0, 0);

    void *ptr = (void *)1;
    node->insert(1, ptr);
    ASSERT_EQ(header->current_size, 1);

    ptr = (void *)2;
    node->insert(2, ptr);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->keys[0], 1);
    ASSERT_EQ(node->keys[1], 2);
    ASSERT_EQ(node->children[0], (void *)1);
    ASSERT_EQ(node->children[1], (void *)2);

    node->insert(1, ptr);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->children[0], (void *)2);

    ASSERT_TRUE(node->can_insert());

    for (int64_t i = 3; i <= 16; i++)
    {
        ASSERT_TRUE(node->can_insert());
        node->insert(i, (void *)i);
    }
    ASSERT_FALSE(node->can_insert());

    for (int64_t i = 2; i < 16; i++)
    {
        ASSERT_EQ(node->keys[i], i + 1);
        ASSERT_EQ(node->children[i], (void *)(i + 1));
    }
}

TEST_F(RNodeTest, Insert48)
{
    RHeader *header = (RHeader *)malloc(size_48);
    RNode48 *node = new (header) RNode48(true, 0, 0, 0);

    void *ptr = (void *)1;
    node->insert(1, ptr);
    ASSERT_EQ(header->current_size, 1);

    ptr = (void *)2;
    node->insert(2, ptr);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->keys[1], 0);
    ASSERT_EQ(node->keys[2], 1);
    ASSERT_EQ(node->children[0], (void *)1);
    ASSERT_EQ(node->children[1], (void *)2);

    node->insert(1, ptr);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->children[0], (void *)2);

    ASSERT_TRUE(node->can_insert());

    for (int64_t i = 1; i <= 48; i++)
    {
        ASSERT_TRUE(node->can_insert());
        node->insert(i, (void *)i);
    }
    ASSERT_FALSE(node->can_insert());

    for (int64_t i = 1; i <= 48; i++)
    {
        ASSERT_EQ(node->keys[i], i - 1);
        ASSERT_EQ(node->children[i - 1], (void *)i);
    }
}

TEST_F(RNodeTest, Insert256)
{
    RHeader *header = (RHeader *)malloc(size_256);
    RNode256 *node = new (header) RNode256(true, 0, 0, 0);

    void *ptr = (void *)1;
    node->insert(1, ptr);
    ASSERT_EQ(header->current_size, 1);

    ptr = (void *)2;
    node->insert(2, ptr);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->children[1], (void *)1);
    ASSERT_EQ(node->children[2], (void *)2);

    node->insert(1, ptr);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->children[1], (void *)2);

    ASSERT_TRUE(node->can_insert());

    for (int64_t i = 0; i < 256; i++)
    {
        ASSERT_TRUE(node->can_insert());
        node->insert(i, (void *)i);
    }
    ASSERT_FALSE(node->can_insert());

    for (int64_t i = 0; i < 256; i++)
    {
        ASSERT_EQ(node->children[i], (void *)i);
    }
}