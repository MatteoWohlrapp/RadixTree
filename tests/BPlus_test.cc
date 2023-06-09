#include "gtest/gtest.h"
#include "../src/btree/BPlus.h"
#include "../src/data/BufferManager.h"
#include "../src/Configuration.h"
#include <random>

constexpr int node_test_size = 96;

class BPlusTest : public ::testing::Test
{
    friend class BPlus;
    friend class BufferManager;

protected:
    std::unique_ptr<BPlus> bplus;
    std::shared_ptr<BufferManager> buffer_manager;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::path bitmap = "bitmap.bin";
    std::filesystem::path data = "data.bin";
    int buffer_size = 5;
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

    void SetUp() override
    {
        std::filesystem::remove(base_path / bitmap);
        std::filesystem::remove(base_path / data);
        buffer_manager = std::make_shared<BufferManager>(std::make_shared<StorageManager>(base_path, node_test_size), buffer_size);
        bplus = std::make_unique<BPlus>(buffer_manager);
    }

    void TearDown() override
    {
    }

    uint64_t get_root_id()
    {
        return bplus->root_id;
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

    bool is_balanced(uint64_t root_id)
    {
        Header *root = buffer_manager->request_page(root_id);
        buffer_manager->unfix_page(root_id, false);
        int balanced = recursive_is_balanced(root);
        return balanced != -1;
    }

    int recursive_is_balanced(Header *header)
    {
        auto page_id = header->page_id;

        if (!header->inner)
            return 1;

        InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;

        auto current_index = node->current_index;
        uint64_t child_ids[current_index + 1];
        for (int i = 0; i <= current_index; i++)
        {
            child_ids[i] = node->child_ids[i];
        }

        int depth = 1;
        if (current_index > 0)
        {
            Header *child_header = buffer_manager->request_page(child_ids[0]);
            buffer_manager->unfix_page(child_ids[0], false);
            depth = recursive_is_balanced(child_header);

            for (int i = 1; i <= current_index; i++)
            {
                child_header = buffer_manager->request_page(child_ids[i]);
                buffer_manager->unfix_page(child_ids[i], false);
                bool balanced = (depth == recursive_is_balanced(child_header));

                if (!balanced)
                    return -1;
            }
            depth += 1;
        }
        return depth;
    }

    bool is_ordered(uint64_t root_id)
    {
        Header *root = buffer_manager->request_page(root_id);
        buffer_manager->unfix_page(root_id, false);
        bool ordered = recursive_is_ordered(root);
        return ordered;
    }

    bool recursive_is_ordered(Header *header)
    {
        logger->debug("Recursive is ordered for page {}", header->page_id);
        if (!header->inner)
            return true;

        InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;
        auto id = node->header.page_id;
        auto current_index = node->current_index;
        uint64_t child_ids[current_index + 1];
        for (int i = 0; i <= current_index; i++)
        {
            child_ids[i] = node->child_ids[i];
        }
        uint64_t keys[current_index];
        for (int i = 0; i < current_index; i++)
        {
            keys[i] = node->keys[i];
        }

        for (int i = 0; i < current_index; i++)
        {
            logger->debug("Ordered, for smaller child {}, key {}; bigger child {}, key {}", child_ids[i], keys[i], child_ids[i + 1], keys[i]);
            bool ordered = smaller_or_equal(child_ids[i], keys[i]) && bigger(child_ids[i + 1], keys[i]);
            logger->debug("Ordered = {}");
            if (!ordered)
                return false;
        }

        Header *child_header;
        for (int i = 0; i <= current_index; i++)
        {
            logger->debug("Ordered for node {}, child {}, index {}", id, child_ids[i], i);
            child_header = buffer_manager->request_page(child_ids[i]);
            buffer_manager->unfix_page(child_ids[i], false);
            bool ordered = recursive_is_ordered(child_header);
            logger->debug("Ordered = {}", ordered);
            if (!ordered)
                return false;
        }
        return true;
    }

    bool smaller_or_equal(uint64_t page_id, int64_t key)
    {
        logger->debug("Smaller or Equal for {}, with key {}", page_id, key);
        Header *header = buffer_manager->request_page(page_id);
        if (!header->inner)
        {
            buffer_manager->unfix_page(page_id, false);
            OuterNode<Configuration::page_size> *node = (OuterNode<Configuration::page_size> *)header;

            for (int i = 0; i < node->current_index; i++)
            {
                if (node->keys[i] > key)
                {
                    return false;
                }
            }
            return true;
        }

        InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;
        auto current_index = node->current_index;
        uint64_t child_ids[current_index + 1];
        for (int i = 0; i <= current_index; i++)
        {
            child_ids[i] = node->child_ids[i];
        }
        buffer_manager->unfix_page(page_id, false);

        for (int i = 0; i < node->current_index; i++)
        {
            logger->debug("Key: {}", node->keys[i]);
            if (node->keys[i] > key)
            {
                return false;
            }
        }

        for (int i = 0; i <= current_index; i++)
        {
            if (!smaller_or_equal(child_ids[0], key))
                return false;
        }

        return true;
    }

    bool bigger(uint64_t page_id, int64_t key)
    {
        logger->debug("Bigger for {}, with key {}", page_id, key);
        Header *header = buffer_manager->request_page(page_id);
        if (!header->inner)
        {
            buffer_manager->unfix_page(page_id, false);
            OuterNode<Configuration::page_size> *node = (OuterNode<Configuration::page_size> *)header;

            for (int i = 0; i < node->current_index; i++)
            {
                if (node->keys[i] < key)
                {
                    return false;
                }
            }
            return true;
        }

        InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;
        auto current_index = node->current_index;
        uint64_t child_ids[current_index + 1];
        for (int i = 0; i <= current_index; i++)
        {
            child_ids[i] = node->child_ids[i];
        }
        buffer_manager->unfix_page(page_id, false);

        for (int i = 0; i < node->current_index; i++)
        {
            logger->debug("Key: {}", node->keys[i]);
            if (node->keys[i] < key)
            {
                return false;
            }
        }

        for (int i = 0; i <= current_index; i++)
        {
            if (!bigger(child_ids[0], key))
                return false;
        }

        return true;
    }

    bool is_concatenated(uint64_t root_id, int num_elements)
    {
        uint64_t page_id = find_leftmost(root_id);
        logger->debug("Leftmost: {}", page_id);
        Header *header;
        OuterNode<Configuration::page_size> *node;
        int count = 0;
        while (page_id != 0)
        {
            header = buffer_manager->request_page(page_id);
            node = (OuterNode<Configuration::page_size> *)header;
            buffer_manager->unfix_page(page_id, false);
            logger->debug("Page: {}", page_id);
            page_id = node->next_lef_id;
            for (int i = 0; i < node->current_index; i++)
            {
                logger->debug("Key: {}", node->keys[i]);
                count++;
                if (i > 0)
                {
                    if (node->keys[i - 1] > node->keys[i])
                    {
                        logger->debug("Order of elements is off"); 
                        return false;
                    }
                }
            }
        }
        if (count != num_elements)
        {
            logger->debug("Count is off, count: {}, expected {}", count, num_elements); 
            return false;
        }

        return true;
    }

    uint64_t find_leftmost(uint64_t page_id)
    {
        Header *header = buffer_manager->request_page(page_id);
        if (!header->inner)
        {
            buffer_manager->unfix_page(page_id, false);
            return header->page_id;
        }

        InnerNode<Configuration::page_size> *node = (InnerNode<Configuration::page_size> *)header;
        auto child_id = node->child_ids[0];
        buffer_manager->unfix_page(page_id, false);
        return find_leftmost(child_id);
    }
};

TEST_F(BPlusTest, BalanceCorrect)
{
    Header *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, true);
    InnerNode<Configuration::page_size> *inner = new (header) InnerNode<Configuration::page_size>();
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, true);
    inner = new (header) InnerNode<Configuration::page_size>();
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 5;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, true);
    inner = new (header) InnerNode<Configuration::page_size>();
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 5;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, true);
    OuterNode<Configuration::page_size> *outer = new (header) OuterNode<Configuration::page_size>();

    ASSERT_TRUE(is_balanced(2));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTest, BalanceIncorrect)
{
    Header *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, true);
    InnerNode<Configuration::page_size> *inner = new (header) InnerNode<Configuration::page_size>();
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, true);
    inner = new (header) InnerNode<Configuration::page_size>();
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 5;
    inner->current_index++;

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, true);
    inner = new (header) InnerNode<Configuration::page_size>();

    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, true);
    OuterNode<Configuration::page_size> *outer = new (header) OuterNode<Configuration::page_size>();

    ASSERT_FALSE(is_balanced(2));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTest, OrderCorrect)
{
    // root
    Header *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, true);
    InnerNode<Configuration::page_size> *inner = new (header) InnerNode<Configuration::page_size>();
    inner->keys[0] = 10;
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    // left
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, true);
    inner = new (header) InnerNode<Configuration::page_size>();
    inner->keys[0] = 5;
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 6;
    inner->current_index++;

    // right
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, true);
    inner = new (header) InnerNode<Configuration::page_size>();
    inner->keys[0] = 15;
    inner->child_ids[0] = 7;
    inner->child_ids[1] = 8;
    inner->current_index++;

    // first outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, true);
    OuterNode<Configuration::page_size> *outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 1;
    outer->current_index++;
    outer->next_lef_id = 6;

    // second outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(6, true);
    outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 9;
    outer->current_index++;
    outer->next_lef_id = 7;

    // third outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(7, true);
    outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 15;
    outer->current_index++;
    outer->next_lef_id = 8;

    // fourth outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(8, true);
    outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 21;
    outer->current_index++;
    outer->next_lef_id = 0;

    ASSERT_TRUE(is_ordered(2));
    ASSERT_TRUE(is_balanced(2));
    ASSERT_TRUE(is_concatenated(2, 4));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTest, OrderIncorrectAtLeaf)
{
    // root
    Header *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, true);
    InnerNode<Configuration::page_size> *inner = new (header) InnerNode<Configuration::page_size>();
    inner->keys[0] = 10;
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    // left
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, true);
    inner = new (header) InnerNode<Configuration::page_size>();
    inner->keys[0] = 5;
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 6;
    inner->current_index++;

    // right
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, true);
    inner = new (header) InnerNode<Configuration::page_size>();
    inner->keys[0] = 15;
    inner->child_ids[0] = 7;
    inner->child_ids[1] = 8;
    inner->current_index++;

    // first outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, true);
    OuterNode<Configuration::page_size> *outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 1;
    outer->current_index++;
    outer->next_lef_id = 6;

    // second outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(6, true);
    outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 9;
    outer->current_index++;
    outer->next_lef_id = 7;

    // third outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(7, true);
    outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 9;
    outer->current_index++;
    outer->next_lef_id = 0;

    // fourth outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(8, true);
    outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 21;
    outer->current_index++;

    ASSERT_FALSE(is_ordered(2));
    ASSERT_TRUE(is_balanced(2));
    ASSERT_FALSE(is_concatenated(2, 4));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTest, OrderIncorrectInner)
{
    // root
    Header *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(2, false);
    InnerNode<Configuration::page_size> *inner = new (header) InnerNode<Configuration::page_size>();
    inner->keys[0] = 10;
    inner->child_ids[0] = 3;
    inner->child_ids[1] = 4;
    inner->current_index++;

    // left
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(3, false);
    inner = new (header) InnerNode<Configuration::page_size>();
    inner->keys[0] = 11;
    inner->child_ids[0] = 5;
    inner->child_ids[1] = 6;
    inner->current_index++;

    // right
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(4, false);
    inner = new (header) InnerNode<Configuration::page_size>();
    inner->keys[0] = 15;
    inner->child_ids[0] = 7;
    inner->child_ids[1] = 8;
    inner->current_index++;

    // first outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(5, false);
    OuterNode<Configuration::page_size> *outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 1;
    outer->current_index++;

    // second outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(6, false);
    outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 9;
    outer->current_index++;

    // third outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(7, false);
    outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 15;
    outer->current_index++;

    // fourth outer
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(8, false);
    outer = new (header) OuterNode<Configuration::page_size>();
    outer->keys[0] = 21;
    outer->current_index++;

    ASSERT_FALSE(is_ordered(2));
    ASSERT_TRUE(is_balanced(2));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTest, CorrectRootNodeType)
{
    ASSERT_FALSE(buffer_manager->request_page(get_root_id())->inner);

    for (int i = 0; i < 6; i++)
    {
        bplus->insert(i, i);
    }

    ASSERT_TRUE(buffer_manager->request_page(get_root_id())->inner);
}

TEST_F(BPlusTest, EmptyAtBeginning)
{
    OuterNode<node_test_size> *node = (OuterNode<node_test_size> *)buffer_manager->request_page(get_root_id());

    ASSERT_EQ(node->current_index, 0);
}

TEST_F(BPlusTest, InsertBoundaries)
{
    for (int i = -20; i <= 20; i++)
    {
        bplus->insert(i, i);
    }
    bplus->insert(INT64_MIN + 1, INT64_MIN + 1);
    bplus->insert(INT64_MAX, INT64_MAX);

    ASSERT_EQ(bplus->get_value(INT64_MIN + 1), INT64_MIN + 1);
    ASSERT_EQ(bplus->get_value(INT64_MAX), INT64_MAX);
    ASSERT_TRUE(is_concatenated(bplus->root_id, 43));
    ASSERT_TRUE(is_ordered(bplus->root_id));
    ASSERT_TRUE(is_balanced(bplus->root_id));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTest, InsertRepeated)
{
    for (int i = 0; i < 20; i++)
    {
        bplus->insert(1, 1);
    }
    ASSERT_EQ(bplus->get_value(1), 1);

    ASSERT_TRUE(is_concatenated(bplus->root_id, 20));
    ASSERT_TRUE(is_ordered(bplus->root_id));
    ASSERT_TRUE(is_balanced(bplus->root_id));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTest, InsertAscending)
{
    for (int i = -20; i <= 20; i++)
    {
        logger->info("Inserting {}", i);
        logger->flush();
        bplus->insert(i, i);
    }
    for (int i = -20; i >= 20; i++)
    {
        ASSERT_EQ(bplus->get_value(i), i);
    }
    ASSERT_TRUE(is_concatenated(bplus->root_id, 41));
    ASSERT_TRUE(is_ordered(bplus->root_id));
    ASSERT_TRUE(is_balanced(bplus->root_id));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTest, InsertDescending)
{
    for (int i = 20; i >= -20; i--)
    {
        bplus->insert(i, i);
    }
    for (int i = 20; i >= -20; i--)
    {
        ASSERT_EQ(bplus->get_value(i), i);
    }
    ASSERT_TRUE(is_concatenated(bplus->root_id, 41));
    ASSERT_TRUE(is_ordered(bplus->root_id));
    ASSERT_TRUE(is_balanced(bplus->root_id));
    ASSERT_TRUE(all_pages_unfixed());
}

TEST_F(BPlusTest, InsertRandom)
{
    std::random_device rd;
    std::uniform_int_distribution<int64_t> dist = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX);

    int64_t values[100];

    for (int i = 0; i < 100; i++)
    {
        int64_t value = dist(rd);
        values[i] = value;
        bplus->insert(value, value);
    }

    for (int i = 0; i < 100; i++)
    {
        ASSERT_EQ(bplus->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_concatenated(bplus->root_id, 100));
    ASSERT_TRUE(is_ordered(bplus->root_id));
    ASSERT_TRUE(is_balanced(bplus->root_id));
    ASSERT_TRUE(all_pages_unfixed());
}