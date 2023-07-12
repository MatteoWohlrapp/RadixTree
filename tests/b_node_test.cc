#include "gtest/gtest.h"
#include "../src/bplus_tree/bplus_tree.h"
#include "../src/data/buffer_manager.h"
#include "../src/configuration.h"

constexpr int node_test_size = 96;

class BNodeTest : public ::testing::Test
{

protected:
    int buffer_size = 2;
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");
    BHeader *header;

    void SetUp() override
    {
        header = (BHeader *)malloc(node_test_size);
    }

    void TearDown() override
    {
        free(header);
    }
};

TEST_F(BNodeTest, BBInnerNodeConstructor)
{
    header->inner = false;
    header->page_id = 3;
    BInnerNode<node_test_size> *node = new (header) BInnerNode<node_test_size>();

    ASSERT_EQ(sizeof(BInnerNode<node_test_size>), node_test_size);
    ASSERT_EQ(node->header.inner, true);
    ASSERT_EQ(node->header.page_id, 3);
    ASSERT_EQ(node->current_index, 0);
    ASSERT_EQ(node->max_size, ((node_test_size - 24) / 2) / 8 - 1);
    ASSERT_EQ(sizeof(node->keys) / sizeof(node->keys[0]), ((node_test_size - 24) / 2) / 8);
    ASSERT_EQ(sizeof(node->child_ids) / sizeof(node->child_ids[0]), ((node_test_size - 24) / 2) / 8);
}

TEST_F(BNodeTest, BInnerNodeFindNextPage)
{
    BInnerNode<node_test_size> *node = new (header) BInnerNode<node_test_size>();

    node->child_ids[0] = 0;
    node->insert(5, 5);
    node->insert(1, 1);
    node->insert(3, 3);

    ASSERT_EQ(node->next_page(0), 0);
    ASSERT_EQ(node->next_page(1), 0);
    ASSERT_EQ(node->next_page(2), 1);
    ASSERT_EQ(node->next_page(3), 1);
    ASSERT_EQ(node->next_page(4), 3);
    ASSERT_EQ(node->next_page(5), 3);
    ASSERT_EQ(node->next_page(6), 5);
}

TEST_F(BNodeTest, BInnerNodeInsert)
{
    BInnerNode<node_test_size> *node = new (header) BInnerNode<node_test_size>();
    node->child_ids[0] = 1;
    node->insert(4, 5);

    ASSERT_EQ(node->current_index, 1);
    ASSERT_EQ(node->keys[0], 4);
    ASSERT_EQ(node->child_ids[0], 1);
    ASSERT_EQ(node->child_ids[1], 5);

    node->insert(2, 3);
    ASSERT_EQ(node->current_index, 2);
    ASSERT_EQ(node->keys[0], 2);
    ASSERT_EQ(node->child_ids[0], 1);
    ASSERT_EQ(node->child_ids[1], 3);
    ASSERT_EQ(node->keys[1], 4);
    ASSERT_EQ(node->child_ids[2], 5);
}

TEST_F(BNodeTest, BInnerNodeFull)
{
    BInnerNode<node_test_size> *node = new (header) BInnerNode<node_test_size>();

    node->insert(3, 3);
    node->insert(1, 1);
    ASSERT_EQ(node->current_index, 2);
    ASSERT_FALSE(node->is_full());
    node->insert(2, 2);
    ASSERT_EQ(node->current_index, 3);
    ASSERT_TRUE(node->is_full());
}

TEST_F(BNodeTest, BOuterNodeConstructor)
{
    header->inner = true;
    header->page_id = 3;
    BOuterNode<node_test_size> *node = new (header) BOuterNode<node_test_size>();

    ASSERT_EQ(sizeof(BOuterNode<node_test_size>), node_test_size);
    ASSERT_EQ(node->header.inner, false);
    ASSERT_EQ(node->header.page_id, 3);
    ASSERT_EQ(node->current_index, 0);
    ASSERT_EQ(node->max_size, ((node_test_size - 24) / 2) / 8);
    ASSERT_EQ(sizeof(node->keys) / sizeof(node->keys[0]), ((node_test_size - 24) / 2) / 8);
    ASSERT_EQ(sizeof(node->values) / sizeof(node->values[0]), ((node_test_size - 24) / 2) / 8);
}

TEST_F(BNodeTest, BOuterNodeInsert)
{
    BOuterNode<node_test_size> *node = new (header) BOuterNode<node_test_size>();
    node->insert(3, 4);

    ASSERT_EQ(node->keys[0], 3);
    ASSERT_EQ(node->values[0], 4);
    ASSERT_EQ(node->current_index, 1);

    node->insert(1, 2);
    ASSERT_EQ(node->keys[0], 1);
    ASSERT_EQ(node->values[0], 2);
    ASSERT_EQ(node->keys[1], 3);
    ASSERT_EQ(node->values[1], 4);
    ASSERT_EQ(node->current_index, 2);
}

TEST_F(BNodeTest, BOuterNodeGetValue)
{
    BOuterNode<node_test_size> *node = new (header) BOuterNode<node_test_size>();
    node->insert(3, 4);
    node->insert(1, 2);

    ASSERT_EQ(node->get_value(3), 4);
}

TEST_F(BNodeTest, BOuterNodeFull)
{
    BOuterNode<node_test_size> *node = new (header) BOuterNode<node_test_size>();

    node->insert(3, 3);
    node->insert(1, 1);
    node->insert(2, 2);
    ASSERT_EQ(node->current_index, 3);
    ASSERT_FALSE(node->is_full());
    node->insert(2, 2);
    ASSERT_EQ(node->current_index, 4);
    ASSERT_TRUE(node->is_full());
}

TEST_F(BNodeTest, BInnerNodeCanDelete)
{
    BInnerNode<node_test_size> *node = new (header) BInnerNode<node_test_size>();

    node->insert(1, 1);
    ASSERT_FALSE(node->can_delete());
    node->insert(1, 1);
    ASSERT_TRUE(node->can_delete());
}

TEST_F(BNodeTest, BInnerNodeContains)
{
    BInnerNode<node_test_size> *node = new (header) BInnerNode<node_test_size>();

    node->insert(2, 2);
    node->insert(1, 1);
    node->insert(3, 3);
    ASSERT_TRUE(node->contains(3));
    ASSERT_TRUE(node->contains(2));
    ASSERT_TRUE(node->contains(1));
    ASSERT_FALSE(node->contains(4));

    node->delete_pair(2);
    node->delete_pair(3);
    node->delete_pair(1);

    ASSERT_FALSE(node->contains(3));
    ASSERT_FALSE(node->contains(2));
    ASSERT_FALSE(node->contains(1));
}

TEST_F(BNodeTest, BInnerNodeDelete)
{
    BInnerNode<node_test_size> *node = new (header) BInnerNode<node_test_size>();

    node->insert(2, 2);
    node->insert(1, 1);
    node->insert(3, 3);
    node->delete_pair(2);
    node->delete_pair(3);
    node->delete_pair(1);

    ASSERT_EQ(node->current_index, 0); 
    ASSERT_FALSE(node->contains(3));
    ASSERT_FALSE(node->contains(2));
    ASSERT_FALSE(node->contains(1));
}

TEST_F(BNodeTest, BInnerNodeExchange)
{
    BInnerNode<node_test_size> *node = new (header) BInnerNode<node_test_size>();

    node->insert(2, 2);
    node->insert(1, 1);
    node->insert(5, 5);

    node->exchange(2, 4);

    ASSERT_FALSE(node->contains(2));
    ASSERT_TRUE(node->contains(4));
}

TEST_F(BNodeTest, BOuterNodeCanDelete)
{
    BOuterNode<node_test_size> *node = new (header) BOuterNode<node_test_size>();

    node->insert(1, 1);
    node->insert(1, 1);
    ASSERT_FALSE(node->can_delete());
    node->insert(1, 1);
    ASSERT_TRUE(node->can_delete());
}

TEST_F(BNodeTest, BOuterNodeDelete)
{
    BOuterNode<node_test_size> *node = new (header) BOuterNode<node_test_size>();

    node->insert(1, 1);
    node->insert(3, 3);
    node->insert(2, 2);
    node->delete_pair(1);
    node->delete_pair(3);
    node->delete_pair(2);

    ASSERT_EQ(node->current_index, 0); 
    ASSERT_EQ(node->get_value(1), INT64_MIN);
    ASSERT_EQ(node->get_value(2), INT64_MIN);
    ASSERT_EQ(node->get_value(3), INT64_MIN);
}