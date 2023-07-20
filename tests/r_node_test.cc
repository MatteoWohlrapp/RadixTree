#include "gtest/gtest.h"
#include "../src/radix_tree/radix_tree.h"
#include "../src/radix_tree/r_nodes.h"

class RNodeTest : public ::testing::Test
{

protected:
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

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
        node->insert(i, (void *)(i + 1));
    }
    ASSERT_FALSE(node->can_insert());

    for (int64_t i = 0; i < 256; i++)
    {
        ASSERT_EQ(node->children[i], (void *)(i + 1));
    }
}

TEST_F(RNodeTest, Insert4CreateFrameAndDelete)
{
    int bytes = 0;
    RHeader *header = (RHeader *)malloc(size_4);
    RNode4 *node = new (header) RNode4(true, 0, 0, 0);

    bytes = node->insert(1, 1, (BHeader *)1);
    ASSERT_EQ(bytes, 16);
    ASSERT_EQ(header->current_size, 1);

    bytes = node->insert(2, 2, (BHeader *)2);
    ASSERT_EQ(bytes, 16);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->keys[0], 1);
    ASSERT_EQ(node->keys[1], 2);
    ASSERT_EQ(((RFrame *)node->children[0])->page_id, 1);
    ASSERT_EQ(((RFrame *)node->children[1])->page_id, 2);

    bytes = node->insert(1, 2, (BHeader *)2);
    ASSERT_EQ(bytes, 0);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(((RFrame *)node->children[0])->page_id, 2);
    ASSERT_EQ(((RFrame *)node->children[0])->header, (BHeader *)2);
    ASSERT_TRUE(node->can_insert());

    bytes = node->insert(3, 3, (BHeader *)1);
    ASSERT_EQ(bytes, 16);
    bytes = node->insert(4, 4, (BHeader *)1);
    ASSERT_EQ(bytes, 16);
    ASSERT_FALSE(node->can_insert());

    // Deleting
    for (int i = 1; i <= 4; i++)
    {
        bytes = node->delete_reference(i);
        ASSERT_EQ(bytes, 16);
    }

    for (int i = 1; i <= 4; i++)
    {
        bytes = node->delete_reference(i);
        ASSERT_EQ(bytes, 0);
        ASSERT_EQ(node->get_next_page(i), nullptr);
    }
}

TEST_F(RNodeTest, Insert16CreateFrameAndDelete)
{
    int bytes = 0;
    RHeader *header = (RHeader *)malloc(size_16);
    RNode16 *node = new (header) RNode16(true, 0, 0, 0);

    bytes = node->insert(1, 1, (BHeader *)1);
    ASSERT_EQ(bytes, 16);
    ASSERT_EQ(header->current_size, 1);

    bytes = node->insert(2, 2, (BHeader *)2);
    ASSERT_EQ(bytes, 16);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->keys[0], 1);
    ASSERT_EQ(node->keys[1], 2);
    ASSERT_EQ(((RFrame *)node->children[0])->page_id, 1);
    ASSERT_EQ(((RFrame *)node->children[1])->page_id, 2);

    bytes = node->insert(1, 2, (BHeader *)2);
    ASSERT_EQ(bytes, 0);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(((RFrame *)node->children[0])->page_id, 2);
    ASSERT_EQ(((RFrame *)node->children[0])->header, (BHeader *)2);

    ASSERT_TRUE(node->can_insert());

    for (int64_t i = 3; i <= 16; i++)
    {
        ASSERT_TRUE(node->can_insert());
        bytes = node->insert(i, i, (BHeader *)i);
        ASSERT_EQ(bytes, 16);
    }
    ASSERT_FALSE(node->can_insert());

    for (int64_t i = 2; i < 16; i++)
    {
        ASSERT_EQ(((RFrame *)node->children[i])->page_id, i + 1);
        ASSERT_EQ(((RFrame *)node->children[i])->header, (BHeader *)(i + 1));
        ASSERT_EQ(node->keys[i], i + 1);
    }

    // Deleting
    for (int i = 1; i <= 16; i++)
    {
        bytes = node->delete_reference(i);
        ASSERT_EQ(bytes, 16);
    }

    for (int i = 1; i <= 16; i++)
    {
        bytes = node->delete_reference(i);
        ASSERT_EQ(bytes, 0);
        ASSERT_EQ(node->get_next_page(i), nullptr);
    }
}

TEST_F(RNodeTest, Insert48CreateFrameAndDelete)
{
    int bytes = 0;
    RHeader *header = (RHeader *)malloc(size_48);
    RNode48 *node = new (header) RNode48(true, 0, 0, 0);

    bytes = node->insert(1, 1, (BHeader *)1);
    ASSERT_EQ(bytes, 16);
    ASSERT_EQ(header->current_size, 1);

    bytes = node->insert(2, 2, (BHeader *)2);
    ASSERT_EQ(bytes, 16);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(node->keys[1], 0);
    ASSERT_EQ(node->keys[2], 1);
    ASSERT_EQ(((RFrame *)node->children[0])->page_id, 1);
    ASSERT_EQ(((RFrame *)node->children[1])->page_id, 2);

    bytes = node->insert(1, 2, (BHeader *)2);
    ASSERT_EQ(bytes, 0);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(((RFrame *)node->children[0])->page_id, 2);
    ASSERT_EQ(((RFrame *)node->children[0])->header, (BHeader *)2);

    ASSERT_TRUE(node->can_insert());

    for (int64_t i = 3; i <= 48; i++)
    {
        ASSERT_TRUE(node->can_insert());
        bytes = node->insert(i, i, (BHeader *)i);
        ASSERT_EQ(bytes, 16);
    }
    ASSERT_FALSE(node->can_insert());

    for (int64_t i = 3; i <= 48; i++)
    {
        ASSERT_EQ(((RFrame *)node->children[i - 1])->page_id, i);
        ASSERT_EQ(((RFrame *)node->children[i - 1])->header, (BHeader *)i);
        ASSERT_EQ(node->keys[i], i - 1);
    }

    // Deleting
    for (int i = 1; i <= 48; i++)
    {
        bytes = node->delete_reference(i);
        ASSERT_EQ(bytes, 16);
    }

    for (int i = 1; i <= 48; i++)
    {
        bytes = node->delete_reference(i);
        ASSERT_EQ(bytes, 0);
        ASSERT_EQ(node->get_next_page(i), nullptr);
    }
}

TEST_F(RNodeTest, Insert256CreateFrameAndDelete)
{
    int bytes = 0;
    RHeader *header = (RHeader *)malloc(size_256);
    RNode256 *node = new (header) RNode256(true, 0, 0, 0);

    bytes = node->insert(0, 1, (BHeader *)1);
    ASSERT_EQ(bytes, 16);
    ASSERT_EQ(header->current_size, 1);

    bytes = node->insert(1, 2, (BHeader *)2);
    ASSERT_EQ(bytes, 16);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(((RFrame *)node->children[0])->page_id, 1);
    ASSERT_EQ(((RFrame *)node->children[1])->page_id, 2);

    bytes = node->insert(0, 2, (BHeader *)2);
    ASSERT_EQ(bytes, 0);
    ASSERT_EQ(header->current_size, 2);
    ASSERT_EQ(((RFrame *)node->children[0])->page_id, 2);
    ASSERT_EQ(((RFrame *)node->children[1])->header, (BHeader *)2);

    ASSERT_TRUE(node->can_insert());

    for (int64_t i = 2; i < 256; i++)
    {
        ASSERT_TRUE(node->can_insert());
        bytes = node->insert(i, i, (BHeader *)i);
        ASSERT_EQ(bytes, 16);
    }
    ASSERT_FALSE(node->can_insert());

    for (int64_t i = 2; i < 256; i++)
    {
        ASSERT_EQ(((RFrame *)node->children[i])->page_id, i);
        ASSERT_EQ(((RFrame *)node->children[i])->header, (BHeader *)i);
    }

    // Deleting
    for (int i = 0; i < 256; i++)
    {
        bytes = node->delete_reference(i);
        ASSERT_EQ(bytes, 16);
    }

    for (int i = 0; i < 256; i++)
    {
        bytes = node->delete_reference(i);
        ASSERT_EQ(bytes, 0);
        ASSERT_EQ(node->get_next_page(i), nullptr);
    }
}

TEST_F(RNodeTest, GetNextChild4)
{
    RHeader *header = (RHeader *)malloc(size_4);
    RNode4 *node = new (header) RNode4(true, 0, 0, 0);

    for (int64_t i = 0; i < 4; i++)
    {
        node->insert(i, (void *)(i + 1));
    }

    for (int64_t i = 0; i < 4; i++)
    {
        ASSERT_EQ(node->get_next_page(i), (void *)(i + 1));
    }
}

TEST_F(RNodeTest, GetNextChild16)
{
    RHeader *header = (RHeader *)malloc(size_16);
    RNode16 *node = new (header) RNode16(true, 0, 0, 0);

    for (int64_t i = 0; i < 16; i++)
    {
        node->insert(i, (void *)(i + 1));
    }

    for (int64_t i = 0; i < 16; i++)
    {
        ASSERT_EQ(node->get_next_page(i), (void *)(i + 1));
    }
}

TEST_F(RNodeTest, GetNextChild48)
{
    RHeader *header = (RHeader *)malloc(size_48);
    RNode48 *node = new (header) RNode48(true, 0, 0, 0);

    for (int64_t i = 1; i <= 48; i++)
    {
        logger->info("Inserting into Node48: {}", i);
        node->insert(i, (void *)i);
    }

    for (int64_t i = 1; i <= 48; i++)
    {
        ASSERT_EQ(node->get_next_page(i), (void *)i);
    }
}

TEST_F(RNodeTest, GetNextChild256)
{
    RHeader *header = (RHeader *)malloc(size_256);
    RNode256 *node = new (header) RNode256(true, 0, 0, 0);

    for (int64_t i = 0; i < 256; i++)
    {
        node->insert(i, (void *)(i + 1));
    }

    for (int64_t i = 0; i < 256; i++)
    {
        ASSERT_EQ(node->get_next_page(i), (void *)(i + 1));
    }
}
