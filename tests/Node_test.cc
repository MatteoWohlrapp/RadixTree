#include "gtest/gtest.h"
#include "../src/btree/BPlus.h"
#include "../src/data/BufferManager.h"
#include "../src/Configuration.h"

constexpr int node_test_size = 84; 

class NodeTest : public ::testing::Test
{
    friend class BPlus;

protected:
    int buffer_size = 2;
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(NodeTest, InnerNodeConstructor)
{
    Header *header = (Header *) malloc(node_test_size); 
    header->inner = false; 
    header->page_id = 3; 
    InnerNode<node_test_size> *node = new (header) InnerNode<node_test_size>(); 

    ASSERT_EQ(node->header.inner, true); 
    ASSERT_EQ(node->header.page_id, 3); 
    ASSERT_EQ(node->current_index, 0);
    ASSERT_EQ(node->max_size, ((node_test_size - 24) / 2) / 8 - 1); 
    ASSERT_EQ(sizeof(node->keys)/sizeof(node->keys[0]), ((node_test_size - 24) / 2) / 8); 
    ASSERT_EQ(sizeof(node->child_ids)/sizeof(node->child_ids[0]), ((node_test_size - 24) / 2) / 8); 
}