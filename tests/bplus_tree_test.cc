#include "gtest/gtest.h"
#include "../src/bplus_tree/bplus_tree.h"
#include "../src/data/buffer_manager.h"
#include "../src/configuration.h"
#include <random>
#include <queue>
#include <unordered_set>

constexpr int PAGE_SIZE = 96;

class BPlusTreeTest : public ::testing::Test
{
    friend class BPlusTree<PAGE_SIZE>;
    friend class BufferManager;

protected:
    int buffer_size = 5;
    BPlusTree<PAGE_SIZE> *bplus_tree;
    BufferManager *buffer_manager;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::path data = "data.bin";
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

    void SetUp() override
    {
        std::filesystem::remove(base_path / data);
        buffer_manager = new BufferManager(new StorageManager(base_path, PAGE_SIZE), buffer_size, PAGE_SIZE);
        bplus_tree = new BPlusTree<PAGE_SIZE>(buffer_manager);
    }

    void TearDown() override
    {
    }

    uint64_t get_root_id()
    {
        return bplus_tree->root_id;
    }

    void set_root_id(uint64_t new_root_id)
    {
        bplus_tree->root_id = new_root_id;
    }

    bool all_pages_unfixed()
    {
        for (auto &pair : buffer_manager->page_id_map)
        {
            if (pair.second->fix_count != 0)
                return false;
        }
        return true;
    }

    bool is_balanced()
    {
        return bplus_tree->is_balanced();
    }

    bool is_ordered()
    {
        return bplus_tree->is_ordered();
    }

    bool is_concatenated(int num_elements)
    {
        return bplus_tree->is_concatenated(num_elements);
    }

    uint64_t find_leftmost()
    {
        return bplus_tree->find_leftmost(get_root_id());
    }

    bool all(std::function<bool(BHeader *)> predicate)
    {
        std::queue<uint64_t> nodes_queue;
        nodes_queue.push(get_root_id());
        int level = 0;

        logger->debug("Starting traversing on root node: {}", get_root_id());
        while (!nodes_queue.empty())
        {
            int level_size = nodes_queue.size();
            logger->debug("Level {} :", level);

            for (int i = 0; i < level_size; i++)
            {
                uint64_t current_id = nodes_queue.front();
                nodes_queue.pop();
                BHeader *current = buffer_manager->request_page(current_id);

                if (!current->inner)
                {
                    std::ostringstream node;
                    BOuterNode<PAGE_SIZE> *outer_node = (BOuterNode<PAGE_SIZE> *)current;
                    node << "BOuterNode:  " << outer_node->header.page_id << " {";
                    for (int j = 0; j < outer_node->current_index; j++)
                    {
                        node << " (Key: " << outer_node->keys[j] << ", Value: " << outer_node->values[j] << ")";
                    }
                    node << "; Next Leaf: " << outer_node->next_lef_id << " }";
                    logger->debug(node.str());
                    if (!predicate(current))
                    {
                        logger->debug("Predicate failed");
                        buffer_manager->unfix_page(current_id, false);
                        return false;
                    }
                }
                else
                {
                    std::ostringstream node;
                    BInnerNode<PAGE_SIZE> *inner_node = (BInnerNode<PAGE_SIZE> *)current;
                    node << "BInnerNode: " << inner_node->header.page_id << " {";
                    node << " (Child_id: " << inner_node->child_ids[0] << ", ";
                    for (int j = 0; j < inner_node->current_index; j++)
                    {
                        node << " Key: " << inner_node->keys[j] << ", Child_id: " << inner_node->child_ids[j + 1] << "";
                    }
                    node << ") }";
                    logger->debug(node.str());
                    if (!predicate(current))
                    {
                        logger->debug("Predicate failed");
                        buffer_manager->unfix_page(current_id, false);
                        return false;
                    }

                    // <= because the child array is one position bigger
                    for (int j = 0; j <= inner_node->current_index; j++)
                    {
                        nodes_queue.push(inner_node->child_ids[j]);
                    }
                }
                buffer_manager->unfix_page(current_id, false);
            }

            level++;
        }
        return true;
    }

    bool minimum_size()
    {
        std::function<bool(BHeader *)> predicate = [this](BHeader *header)
        {
            if (header->inner)
            {
                BInnerNode<PAGE_SIZE> *inner = (BInnerNode<PAGE_SIZE> *)header;
                if (get_root_id() == header->page_id)
                    return inner->current_index > 0;
                return !inner->is_too_empty();
            }
            else
            {
                BOuterNode<PAGE_SIZE> *outer = (BOuterNode<PAGE_SIZE> *)header;
                if (get_root_id() == header->page_id)
                    return true;
                return !outer->is_too_empty();
            }
        };
        return all(predicate);
    }

    bool not_contains(int64_t key)
    {
        std::function<bool(BHeader *)> predicate = [&key](BHeader *header)
        {
            if (header->inner)
            {
                BInnerNode<PAGE_SIZE> *inner = (BInnerNode<PAGE_SIZE> *)header;
                return !inner->contains(key);
            }
            else
            {
                BOuterNode<PAGE_SIZE> *outer = (BOuterNode<PAGE_SIZE> *)header;
                return outer->get_value(key) == INT64_MIN;
            }
        };
        return all(predicate);
    }
};

// Test the controll functions for insert
TEST_F(BPlusTreeTest, BalanceCorrect)
{
    BHeader *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, true);
    BInnerNode<PAGE_SIZE> *inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, true);
    inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 5;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, true);
    inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 5;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, true);
    BOuterNode<PAGE_SIZE> *outer = new (header) BOuterNode<PAGE_SIZE>();

    set_root_id(2);
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, BalanceIncorrect)
{
    BHeader *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, true);
    BInnerNode<PAGE_SIZE> *inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, true);
    inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 5;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, true);
    inner = new (header) BInnerNode<PAGE_SIZE>();

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, true);
    BOuterNode<PAGE_SIZE> *outer = new (header) BOuterNode<PAGE_SIZE>();

    set_root_id(2);
    ASSERT_FALSE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, OrderCorrect)
{
    // root
    BHeader *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, true);
    BInnerNode<PAGE_SIZE> *inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->keys[0] = 10;
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    // left
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, true);
    inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->keys[0] = 5;
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 6;
    inner->current_index++;

    // right
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, true);
    inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->keys[0] = 15;
    inner->child_ids[0] = 7;
    inner->child_ids[1] = 8;
    inner->current_index++;

    // first outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, true);
    BOuterNode<PAGE_SIZE> *outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 1;
    outer->current_index++;
    outer->next_lef_id = 6;

    // second outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(6, true);
    outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 9;
    outer->current_index++;
    outer->next_lef_id = 7;

    // third outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(7, true);
    outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 15;
    outer->current_index++;
    outer->next_lef_id = 8;

    // fourth outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(8, true);
    outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 21;
    outer->current_index++;
    outer->next_lef_id = 0;

    set_root_id(2);
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(is_concatenated(4));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, OrderIncorrectAtLeaf)
{
    // root
    BHeader *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, true);
    BInnerNode<PAGE_SIZE> *inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->keys[0] = 10;
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    // left
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, true);
    inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->keys[0] = 5;
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 6;
    inner->current_index++;

    // right
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, true);
    inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->keys[0] = 15;
    inner->child_ids[0] = 7;
    inner->child_ids[1] = 8;
    inner->current_index++;

    // first outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, true);
    BOuterNode<PAGE_SIZE> *outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 1;
    outer->current_index++;
    outer->next_lef_id = 6;

    // second outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(6, true);
    outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 9;
    outer->current_index++;
    outer->next_lef_id = 7;

    // third outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(7, true);
    outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 9;
    outer->current_index++;
    outer->next_lef_id = 0;

    // fourth outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(8, true);
    outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 21;
    outer->current_index++;

    set_root_id(2);
    ASSERT_FALSE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_FALSE(is_concatenated(4));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, OrderIncorrectInner)
{
    // root
    BHeader *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, false);
    BInnerNode<PAGE_SIZE> *inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->keys[0] = 10;
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    // left
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, false);
    inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->keys[0] = 11;
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 6;
    inner->current_index++;

    // right
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, false);
    inner = new (header) BInnerNode<PAGE_SIZE>();
    inner->keys[0] = 15;
    inner->child_ids[0] = 7;
    inner->child_ids[1] = 8;
    inner->current_index++;

    // first outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, false);
    BOuterNode<PAGE_SIZE> *outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 1;
    outer->current_index++;

    // second outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(6, false);
    outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 9;
    outer->current_index++;

    // third outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(7, false);
    outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 15;
    outer->current_index++;

    // fourth outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(8, false);
    outer = new (header) BOuterNode<PAGE_SIZE>();
    outer->keys[0] = 21;
    outer->current_index++;

    set_root_id(2);
    ASSERT_FALSE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

// Test insert
TEST_F(BPlusTreeTest, CorrectRootNodeType)
{
    ASSERT_FALSE(buffer_manager->request_page(get_root_id())->inner);
    buffer_manager->unfix_page(get_root_id(), false);

    for (int i = 0; i < 6; i++)
    {
        bplus_tree->insert(i, i);
    }

    ASSERT_TRUE(buffer_manager->request_page(get_root_id())->inner);
}

TEST_F(BPlusTreeTest, EmptyAtBeginning)
{
    BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)buffer_manager->request_page(get_root_id());

    ASSERT_EQ(node->current_index, 0);
}

TEST_F(BPlusTreeTest, InsertBoundaries)
{
    for (int i = -20; i <= 20; i++)
    {
        bplus_tree->insert(i, i);
    }
    bplus_tree->insert(INT64_MIN + 1, INT64_MIN + 1);
    bplus_tree->insert(INT64_MAX, INT64_MAX);

    ASSERT_EQ(bplus_tree->get_value(INT64_MIN + 1), INT64_MIN + 1);
    ASSERT_EQ(bplus_tree->get_value(INT64_MAX), INT64_MAX);
    ASSERT_TRUE(is_concatenated(43));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, InsertRepeated)
{
    for (int i = 0; i < 20; i++)
    {
        bplus_tree->insert(1, 1);
    }
    ASSERT_EQ(bplus_tree->get_value(1), 1);

    ASSERT_TRUE(is_concatenated(20));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, InsertAscending)
{
    for (int i = -20; i <= 20; i++)
    {
        logger->debug("Inserting {}", i);
        logger->flush();
        bplus_tree->insert(i, i);
    }
    for (int i = -20; i >= 20; i++)
    {
        ASSERT_EQ(bplus_tree->get_value(i), i);
    }
    ASSERT_TRUE(is_concatenated(41));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, InsertDescending)
{
    for (int i = 20; i >= -20; i--)
    {
        bplus_tree->insert(i, i);
    }
    for (int i = 20; i >= -20; i--)
    {
        ASSERT_EQ(bplus_tree->get_value(i), i);
    }
    ASSERT_TRUE(is_concatenated(41));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, InsertRandom)
{
    std::random_device rd;
    std::uniform_int_distribution<int64_t> dist = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX);

    int64_t values[100];

    for (int i = 0; i < 100; i++)
    {
        int64_t value = dist(rd);
        values[i] = value;
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 100; i++)
    {
        ASSERT_EQ(bplus_tree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_concatenated(100));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

// Test the additional control functions for delete
TEST_F(BPlusTreeTest, NotContains)
{
    std::random_device rd;
    std::uniform_int_distribution<int64_t> dist = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX);

    for (int i = 0; i < 100; i++)
    {
        int64_t value = dist(rd);
        while (value == 42)
            value = dist(rd);
        bplus_tree->insert(value, value);
    }

    ASSERT_TRUE(not_contains(42));
    ASSERT_TRUE(all_pages_unfixed());
}

// Test the additional control functions for delete
TEST_F(BPlusTreeTest, Contains)
{
    std::random_device rd;
    std::uniform_int_distribution<int64_t> dist = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX);

    for (int i = 0; i < 99; i++)
    {
        int64_t value = dist(rd);
        bplus_tree->insert(value, value);
    }
    bplus_tree->insert(42, 42);

    ASSERT_FALSE(not_contains(42));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, MinimumSize)
{
    ASSERT_TRUE(minimum_size());

    std::random_device rd;
    std::uniform_int_distribution<int64_t> dist = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX);

    for (int i = 0; i < 100; i++)
    {
        int64_t value = dist(rd);
        bplus_tree->insert(value, value);
    }

    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, NotMinimumSizeInnerRoot)
{
    std::random_device rd;
    std::uniform_int_distribution<int64_t> dist = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX);

    for (int i = 0; i < 5; i++)
    {
        int64_t value = dist(rd);
        bplus_tree->insert(value, value);
    }

    ASSERT_TRUE(minimum_size());

    BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)buffer_manager->request_page(get_root_id());
    node->current_index = 0;
    buffer_manager->unfix_page(node->header.page_id, true);

    ASSERT_FALSE(minimum_size());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, NotMinimumSizeInnerChild)
{

    for (int i = 0; i < 20; i++)
    {
        bplus_tree->insert(i, i);
    }

    ASSERT_TRUE(minimum_size());

    BInnerNode<PAGE_SIZE> *node = (BInnerNode<PAGE_SIZE> *)buffer_manager->request_page(get_root_id());
    buffer_manager->unfix_page(node->header.page_id, false);
    node = (BInnerNode<PAGE_SIZE> *)buffer_manager->request_page(node->child_ids[0]);
    node->current_index = 0;
    buffer_manager->unfix_page(node->header.page_id, true);

    ASSERT_FALSE(minimum_size());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, NotMinimumSizeLeaf)
{
    for (int i = 0; i < 20; i++)
    {
        bplus_tree->insert(i, i);
    }

    ASSERT_TRUE(minimum_size());

    auto leftmost = find_leftmost();
    BOuterNode<PAGE_SIZE> *node = (BOuterNode<PAGE_SIZE> *)buffer_manager->request_page(leftmost);
    node->current_index = 0;
    buffer_manager->unfix_page(node->header.page_id, true);

    ASSERT_FALSE(minimum_size());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, DeleteFromOuterRoot)
{
    bplus_tree->insert(1, 1);
    bplus_tree->insert(4, 4);
    bplus_tree->insert(2, 2);
    bplus_tree->insert(3, 3);

    bplus_tree->delete_value(3);
    ASSERT_TRUE(not_contains(3));

    bplus_tree->delete_value(1);
    ASSERT_TRUE(not_contains(1));

    bplus_tree->delete_value(4);
    ASSERT_TRUE(not_contains(4));

    bplus_tree->delete_value(2);
    ASSERT_TRUE(not_contains(2));

    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, DeleteInnerOneLevelExchange)
{
    for (int i = 1; i < 7; i++)
    {
        bplus_tree->insert(i * 2, i * 2);
    }
    bplus_tree->insert(3, 3);
    bplus_tree->insert(7, 7);

    bplus_tree->delete_value(4);
    ASSERT_TRUE(not_contains(4));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(7));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());

    bplus_tree->delete_value(8);
    ASSERT_TRUE(not_contains(8));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(6));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, DeleteInnerOneLevelSubstituteLeftMiddle)
{
    for (int i = 1; i < 8; i++)
    {
        bplus_tree->insert(i * 2, i * 2);
    }
    bplus_tree->insert(3, 3);

    bplus_tree->delete_value(8);
    ASSERT_TRUE(not_contains(8));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(7));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, DeleteInnerOneLevelSubstituteLeftRightBoundary)
{
    for (int i = 1; i < 8; i++)
    {
        bplus_tree->insert(i * 2, i * 2);
    }
    bplus_tree->insert(7, 7);

    bplus_tree->delete_value(14);
    bplus_tree->delete_value(12);

    ASSERT_TRUE(not_contains(12));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(6));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, DeleteInnerOneLevelSubstituteRightMiddle)
{
    for (int i = 1; i < 8; i++)
    {
        bplus_tree->insert(i * 2, i * 2);
    }

    bplus_tree->delete_value(8);
    ASSERT_TRUE(not_contains(8));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(6));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, DeleteInnerOneLevelSubstituteRightLeftBoundary)
{
    for (int i = 1; i < 8; i++)
    {
        bplus_tree->insert(i * 2, i * 2);
    }

    bplus_tree->insert(7, 7);

    bplus_tree->delete_value(2);
    ASSERT_TRUE(not_contains(2));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(7));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, DeleteInnerOneLevelMergeLeftMiddle)
{
    for (int i = 1; i < 8; i++)
    {
        bplus_tree->insert(i * 2, i * 2);
    }
    bplus_tree->delete_value(14);
    bplus_tree->delete_value(8);
    ASSERT_TRUE(not_contains(8));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(5));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, DeleteInnerOneLevelMergeLeftRightBoundary)
{
    for (int i = 1; i < 8; i++)
    {
        bplus_tree->insert(i * 2, i * 2);
    }

    bplus_tree->delete_value(14);
    bplus_tree->delete_value(12);

    ASSERT_TRUE(not_contains(12));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(5));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, DeleteInnerOneLevelMergeRightLeftBoundary)
{
    for (int i = 1; i < 8; i++)
    {
        bplus_tree->insert(i * 2, i * 2);
    }

    bplus_tree->delete_value(2);
    ASSERT_TRUE(not_contains(2));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(6));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, OneLevelDecreaseDepth)
{
    for (int i = 1; i < 6; i++)
    {
        bplus_tree->insert(i * 2, i * 2);
    }
    bplus_tree->delete_value(4);
    bplus_tree->delete_value(6);

    ASSERT_TRUE(not_contains(4));
    ASSERT_TRUE(not_contains(6));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(3));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, SubstituteRight)
{
    bplus_tree->insert(20, 20);
    bplus_tree->insert(15, 15);
    bplus_tree->insert(10, 10);
    bplus_tree->insert(5, 5);
    bplus_tree->insert(4, 4);
    bplus_tree->insert(3, 3);
    bplus_tree->insert(2, 2);
    bplus_tree->insert(11, 11);
    bplus_tree->insert(12, 12);
    bplus_tree->insert(13, 13);
    bplus_tree->insert(21, 21);
    bplus_tree->insert(22, 22);
    bplus_tree->delete_value(4);

    ASSERT_TRUE(not_contains(4));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(11));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, SubstituteLeft)
{
    bplus_tree->insert(20, 20);
    bplus_tree->insert(15, 15);
    bplus_tree->insert(10, 10);
    bplus_tree->insert(5, 5);
    bplus_tree->insert(4, 4);
    bplus_tree->insert(3, 3);
    bplus_tree->insert(2, 2);
    bplus_tree->insert(11, 11);
    bplus_tree->insert(12, 12);
    bplus_tree->insert(6, 6);
    bplus_tree->insert(7, 7);
    bplus_tree->insert(8, 8);
    bplus_tree->insert(9, 9);
    bplus_tree->insert(1, 1);
    bplus_tree->insert(0, 0);
    bplus_tree->delete_value(10);

    ASSERT_TRUE(not_contains(10));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(14));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, MergeRightInnerNode)
{
    bplus_tree->insert(20, 20);
    bplus_tree->insert(15, 15);
    bplus_tree->insert(10, 10);
    bplus_tree->insert(5, 5);
    bplus_tree->insert(4, 4);
    bplus_tree->insert(3, 3);
    bplus_tree->insert(2, 2);
    bplus_tree->insert(11, 11);
    bplus_tree->insert(12, 12);
    bplus_tree->insert(6, 6);
    bplus_tree->insert(7, 7);
    bplus_tree->insert(8, 8);
    bplus_tree->insert(9, 9);
    bplus_tree->delete_value(4);

    ASSERT_TRUE(not_contains(4));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(12));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, MergeLeftInnerNode)
{
    bplus_tree->insert(20, 20);
    bplus_tree->insert(15, 15);
    bplus_tree->insert(10, 10);
    bplus_tree->insert(5, 5);
    bplus_tree->insert(4, 4);
    bplus_tree->insert(3, 3);
    bplus_tree->insert(2, 2);
    bplus_tree->insert(11, 11);
    bplus_tree->insert(12, 12);
    bplus_tree->insert(6, 6);
    bplus_tree->insert(7, 7);
    bplus_tree->insert(8, 8);
    bplus_tree->insert(9, 9);
    bplus_tree->insert(21, 21);
    bplus_tree->insert(22, 21);
    bplus_tree->insert(23, 23);
    bplus_tree->insert(24, 24);
    bplus_tree->delete_value(10);

    ASSERT_TRUE(not_contains(10));
    ASSERT_TRUE(minimum_size());
    ASSERT_TRUE(is_concatenated(16));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, InsertAndDeleteRandom)
{
    std::random_device rd;
    std::uniform_int_distribution<int64_t> dist(INT64_MIN + 1, INT64_MAX);
    std::unordered_set<int64_t> unique_values;
    int64_t values[100];

    for (int i = 0; i < 100; i++)
    {
        int64_t value;
        do
        {
            value = dist(rd);
        } while (unique_values.count(value) > 0);

        unique_values.insert(value);
        values[i] = value;
        logger->debug("Inserting {}", value);
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 100; i++)
    {
        ASSERT_EQ(bplus_tree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_concatenated(100));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());

    for (int i = 0; i < 50; i++)
    {
        logger->debug("Deleting {} at index {}", values[i], i);
        bplus_tree->delete_value(values[i]);
    }
    for (int i = 0; i < 50; i++)
    {
        ASSERT_EQ(bplus_tree->get_value(values[i]), INT64_MIN);
    }
    ASSERT_TRUE(is_concatenated(50));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());

    unique_values.clear();
    for (int i = 0; i < 50; i++)
    {
        int64_t value;
        do
        {
            value = dist(rd);
        } while (unique_values.count(value) > 0);

        unique_values.insert(value);
        values[i] = value;
        bplus_tree->insert(value, value);
    }
    for (int i = 0; i < 50; i++)
    {
        ASSERT_EQ(bplus_tree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_concatenated(100));
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());

    for (int i = 0; i < 100; i++)
    {
        bplus_tree->delete_value(values[i]);
    }
    for (int i = 0; i < 100; i++)
    {
        ASSERT_EQ(bplus_tree->get_value(values[i]), INT64_MIN);
    }
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTreeTest, InsertDelteWithSeed42)
{
    std::mt19937 generator(42); // 42 is the seed value
    std::uniform_int_distribution<int64_t> dist(-1000, 1000);
    std::unordered_set<int64_t> unique_values;
    int64_t values[100];

    for (int i = 0; i < 100; i++)
    {
        int64_t value = dist(generator); // generate a random number

        // Ensure we have a unique value, if not, generate another one
        while (unique_values.count(value))
        {
            value = dist(generator);
        }

        unique_values.insert(value);
        values[i] = value;
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 100; i++)
    {
        logger->debug("Deleting {} at index {}", values[i], i);
        bplus_tree->delete_value(values[i]);
        ASSERT_TRUE(is_ordered());
        ASSERT_TRUE(is_balanced());
        ASSERT_EQ(bplus_tree->get_value(values[i]), INT64_MIN);
    }
}

TEST_F(BPlusTreeTest, UpdateWithSeed42)
{
    std::mt19937 generator(42); // 42 is the seed value
    std::uniform_int_distribution<int64_t> dist(-1000, 1000);
    std::unordered_set<int64_t> unique_values;
    int64_t values[100];

    for (int i = 0; i < 100; i++)
    {
        int64_t value = dist(generator); // generate a random number

        // Ensure we have a unique value, if not, generate another one
        while (unique_values.count(value))
        {
            value = dist(generator);
        }

        unique_values.insert(value);
        values[i] = value;
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 100; i++)
    {
        bplus_tree->update(values[i], values[i] + 1);
        ASSERT_TRUE(is_ordered());
        ASSERT_TRUE(is_balanced());
        ASSERT_TRUE(all_pages_unfixed());
    }

    for (int i = 0; i < 100; i++)
    {
        ASSERT_EQ(bplus_tree->get_value(values[i]), values[i] + 1);
    }
}

TEST_F(BPlusTreeTest, ScanWithSeed42)
{
    std::mt19937 generator(42); // 42 is the seed value
    std::uniform_int_distribution<int64_t> dist(-1000, 1000);
    std::unordered_set<int64_t> unique_values;
    int64_t values[100];

    for (int i = 0; i < 100; i++)
    {
        int64_t value = dist(generator); // generate a random number

        // Ensure we have a unique value, if not, generate another one
        while (unique_values.count(value))
        {
            value = dist(generator);
        }

        unique_values.insert(value);
        values[i] = value;
        bplus_tree->insert(value, value);
    }

    std::sort(values, values + 100);

    int sum = 0;
    for (int i = 0; i < 70; i++)
    {
        sum ^= values[i];
    }

    ASSERT_EQ(bplus_tree->scan(values[0], 70), sum);
    ASSERT_TRUE(is_ordered());
    ASSERT_TRUE(is_balanced());
    ASSERT_TRUE(all_pages_unfixed());
}