#include "gtest/gtest.h"
#include "../src/radix_tree/radix_tree.h"
#include "../src/radix_tree/r_nodes.h"
#include "../src/data/buffer_manager.h"
#include "../src/data/data_manager.h"
#include "../src/data/storage_manager.h"
#include "../src/bplus_tree/bplus_tree.h"
#include <unordered_set>

constexpr int PAGE_SIZE = 96;

class RadixTreeTest : public ::testing::Test
{
    friend class RadixTree<PAGE_SIZE>;

protected:
    int buffer_size = 1000;
    RadixTree<PAGE_SIZE> *radix_tree;
    StorageManager *storage_manager;
    BufferManager *buffer_manager;
    BPlusTree<PAGE_SIZE> *bplus_tree;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::path bitmap = "bitmap.bin";
    std::filesystem::path data = "data.bin";
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

    void SetUp() override
    {
        std::filesystem::remove(base_path / bitmap);
        std::filesystem::remove(base_path / data);
        storage_manager = new StorageManager(base_path, PAGE_SIZE);
        buffer_manager = new BufferManager(storage_manager, buffer_size, PAGE_SIZE);
        radix_tree = new RadixTree<PAGE_SIZE>(1000000, buffer_manager);
        bplus_tree = new BPlusTree<PAGE_SIZE>(buffer_manager, radix_tree);
    }

    void TearDown() override
    {
    }

    RHeader *get_root()
    {
        return radix_tree->root;
    }

    int get_current_size()
    {
        return radix_tree->current_size;
    }

    uint8_t get_key_test(uint64_t key, int depth)
    {
        return radix_tree->get_key(key, depth);
    }

    int longest_common_prefix_test(int64_t key_a, int64_t key_b)
    {
        return radix_tree->longest_common_prefix(key_a, key_b);
    }

    bool is_compressed()
    {
        return radix_tree->is_compressed(radix_tree->root);
    }

    bool leaf_depth_correct()
    {
        return radix_tree->leaf_depth_correct(radix_tree->root);
    }

    bool key_matches()
    {
        return radix_tree->key_matches(radix_tree->root);
    }

    BHeader *get_page(int64_t key)
    {
        if (radix_tree->root)
        {
            radix_tree->root->fix_node();
            return radix_tree->get_page_recursive(radix_tree->root, radix_tree->transform(key));
        }
        return nullptr;
    }
};

TEST_F(RadixTreeTest, GetKey)
{
    uint64_t highest_bit = 9295429630892703744ULL;
    uint64_t lowest_bit = 9223372036854775809ULL;

    ASSERT_EQ(get_key_test(highest_bit, 8), 0);
    ASSERT_EQ(get_key_test(lowest_bit, 8), 1);

    ASSERT_EQ(get_key_test(highest_bit, 1), 129);
    ASSERT_EQ(get_key_test(lowest_bit, 1), 128);
}

TEST_F(RadixTreeTest, LongestCommonPrefix)
{

    ASSERT_EQ(longest_common_prefix_test(-9151314442816847872, -9223372036854775807 - 1), 0);

    // not the same because otherwise not added to the same node
    ASSERT_EQ(longest_common_prefix_test(1, 1), 7);

    ASSERT_EQ(longest_common_prefix_test(-9223372032559808512, -9223372036854775807 - 1), 3);
}

TEST_F(RadixTreeTest, IsCompressed)
{
    radix_tree->insert(-9223372036854775807 - 1, 0, 0);
    radix_tree->insert(-9223372036854775552, 0, 0);
    radix_tree->insert(-9223372036854710272, 0, 0);
    radix_tree->insert(-9223372036837998592, 0, 0);
    radix_tree->insert(-9223372032559808512, 0, 0);
    radix_tree->insert(-9223370937343148032, 0, 0);
    radix_tree->insert(-9223090561878065152, 0, 0);
    radix_tree->insert(-9151314442816847872, 0, 0);

    ASSERT_TRUE(is_compressed());

    get_root()->current_size = 1;
    ASSERT_FALSE(is_compressed());

    get_root()->current_size = 2;
    ASSERT_TRUE(is_compressed());

    RNode4 *node = (RNode4 *)((RNode4 *)get_root())->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];
    node->header.current_size = 1;
    ASSERT_FALSE(is_compressed());
}

TEST_F(RadixTreeTest, LeafDepthCorrect)
{
    radix_tree->insert(-9223372036854775807 - 1, 0, 0);
    radix_tree->insert(-9223372036854775552, 0, 0);
    radix_tree->insert(-9223372036854710272, 0, 0);
    radix_tree->insert(-9223372036837998592, 0, 0);
    radix_tree->insert(-9223372032559808512, 0, 0);
    radix_tree->insert(-9223370937343148032, 0, 0);
    radix_tree->insert(-9223090561878065152, 0, 0);
    radix_tree->insert(-9151314442816847872, 0, 0);

    ASSERT_TRUE(leaf_depth_correct());

    RNode4 *node = (RNode4 *)((RNode4 *)get_root())->children[1];
    node->header.depth = 7;
    ASSERT_FALSE(leaf_depth_correct());

    node->header.depth = 8;
    ASSERT_TRUE(leaf_depth_correct());

    node = (RNode4 *)((RNode4 *)get_root())->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];

    node->header.leaf = true;
    ASSERT_FALSE(leaf_depth_correct());

    node->header.leaf = false;
    ASSERT_TRUE(leaf_depth_correct());

    node->header.depth = 8;
    ASSERT_FALSE(leaf_depth_correct());
}

TEST_F(RadixTreeTest, KeyMatches)
{
    radix_tree->insert(-9223372036854775807 - 1, 0, 0);
    radix_tree->insert(-9223372036854775552, 0, 0);
    radix_tree->insert(-9223372036854710272, 0, 0);
    radix_tree->insert(-9223372036837998592, 0, 0);
    radix_tree->insert(-9223372032559808512, 0, 0);
    radix_tree->insert(-9223370937343148032, 0, 0);
    radix_tree->insert(-9223090561878065152, 0, 0);
    radix_tree->insert(-9151314442816847872, 0, 0);

    ASSERT_TRUE(key_matches());

    RNode4 *node = (RNode4 *)((RNode4 *)get_root())->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];
    node = (RNode4 *)node->children[0];
    node->header.key = -9151314442816847872;
    ASSERT_FALSE(key_matches());
}

// Isolated testing of a few use cases that I identified as correct during manual testing
TEST_F(RadixTreeTest, InsertLazyLeaf)
{
    radix_tree->insert(-9223372036854775807 - 1, 0, 0);
    radix_tree->insert(-9223372036854775552, 0, 0);
    radix_tree->insert(-9223372036854710272, 0, 0);
    radix_tree->insert(-9223372036837998592, 0, 0);
    radix_tree->insert(-9223372032559808512, 0, 0);
    radix_tree->insert(-9223370937343148032, 0, 0);
    radix_tree->insert(-9223090561878065152, 0, 0);
    radix_tree->insert(-9151314442816847872, 0, 0);
    radix_tree->insert(-9151314442800070656, 0, 0);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, InsertResizeNonLazyLeaf)
{
    radix_tree->insert(-9223372036854775807, 0, 0);
    radix_tree->insert(-9223372036854775552, 0, 0);
    radix_tree->insert(-9223372036854710272, 0, 0);
    radix_tree->insert(-9223372036837998592, 0, 0);
    radix_tree->insert(-9223372032559808512, 0, 0);
    radix_tree->insert(-9223370937343148032, 0, 0);
    radix_tree->insert(-9223090561878065152, 0, 0);
    radix_tree->insert(-9151314442816847872, 0, 0);
    radix_tree->insert(-9223372036854775806, 0, 0);
    radix_tree->insert(-9223372036854775805, 0, 0);
    radix_tree->insert(-9223372036854775804, 0, 0);
    radix_tree->insert(-9223372036854775803, 0, 0);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, InsertResizeLazyLeaf)
{
    radix_tree->insert(-9223372036854775807, 0, 0);
    radix_tree->insert(-9223372036854775552, 0, 0);
    radix_tree->insert(-9223372036854710272, 0, 0);
    radix_tree->insert(-9223372036837998592, 0, 0);
    radix_tree->insert(-9223372032559808512, 0, 0);
    radix_tree->insert(-9223370937343148032, 0, 0);
    radix_tree->insert(-9223090561878065152, 0, 0);
    radix_tree->insert(-9151314442816847872, 0, 0);
    radix_tree->insert(-9151314442816847871, 0, 0);
    radix_tree->insert(-9151314442816847870, 0, 0);
    radix_tree->insert(-9151314442816847869, 0, 0);
    radix_tree->insert(-9151314442816847868, 0, 0);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, InsertResizeInnerNode)
{
    radix_tree->insert(-9223372036854775807, 0, 0);
    radix_tree->insert(-9223372036854775552, 0, 0);
    radix_tree->insert(-9223372036854710272, 0, 0);
    radix_tree->insert(-9223372036837998592, 0, 0);
    radix_tree->insert(-9223372032559808512, 0, 0);
    radix_tree->insert(-9223370937343148032, 0, 0);
    radix_tree->insert(-9223090561878065152, 0, 0);
    radix_tree->insert(-9151314442816847872, 0, 0);
    radix_tree->insert(-9223372028264841216, 0, 0);
    radix_tree->insert(-9223372023969873920, 0, 0);
    radix_tree->insert(-9223372019674906624, 0, 0);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, InsertResizeRoot)
{
    radix_tree->insert(-9223372036854775807, 0, 0);
    radix_tree->insert(-9223372036854775552, 0, 0);
    radix_tree->insert(-9223372036854710272, 0, 0);
    radix_tree->insert(-9223372036837998592, 0, 0);
    radix_tree->insert(-9223372032559808512, 0, 0);
    radix_tree->insert(-9223370937343148032, 0, 0);
    radix_tree->insert(-9223090561878065152, 0, 0);
    radix_tree->insert(-9151314442816847872, 0, 0);
    radix_tree->insert(-9079256848778919936, 0, 0);
    radix_tree->insert(-9007199254740992000, 0, 0);
    radix_tree->insert(-8935141660703064064, 0, 0);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

// Testing get value with use cases identified above
TEST_F(RadixTreeTest, InsertLazyLeafBPlusTree)
{
    logger->debug("InsertLazyLeafBPlusTree starts");
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);
    bplus_tree->insert(-9151314442800070656, -9151314442800070656);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);
    ASSERT_EQ(radix_tree->get_value(-9151314442800070656), -9151314442800070656);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
    logger->debug("InsertLazyLeafBPlusTree ends");
}

TEST_F(RadixTreeTest, InsertResizeNonLazyLeafBPlusTree)
{
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);
    bplus_tree->insert(-9223372036854775806, -9223372036854775806);
    bplus_tree->insert(-9223372036854775805, -9223372036854775805);
    bplus_tree->insert(-9223372036854775804, -9223372036854775804);
    bplus_tree->insert(-9223372036854775803, -9223372036854775803);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775806), -9223372036854775806);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775805), -9223372036854775805);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775804), -9223372036854775804);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775803), -9223372036854775803);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, InsertResizeLazyLeafBPlusTree)
{
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);
    bplus_tree->insert(-9151314442816847871, -9151314442816847871);
    bplus_tree->insert(-9151314442816847870, -9151314442816847870);
    bplus_tree->insert(-9151314442816847869, -9151314442816847869);
    bplus_tree->insert(-9151314442816847868, -9151314442816847868);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847871), -9151314442816847871);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847870), -9151314442816847870);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847869), -9151314442816847869);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847868), -9151314442816847868);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, InsertResizeInnerNodeBPlusTree)
{
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);
    bplus_tree->insert(-9223372028264841216, -9223372028264841216);
    bplus_tree->insert(-9223372023969873920, -9223372023969873920);
    bplus_tree->insert(-9223372019674906624, -9223372019674906624);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);
    ASSERT_EQ(radix_tree->get_value(-9223372028264841216), -9223372028264841216);
    ASSERT_EQ(radix_tree->get_value(-9223372023969873920), -9223372023969873920);
    ASSERT_EQ(radix_tree->get_value(-9223372019674906624), -9223372019674906624);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, InsertResizeRootBPlusTree)
{
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);
    bplus_tree->insert(-9079256848778919936, -9079256848778919936);
    bplus_tree->insert(-9007199254740992000, -9007199254740992000);
    bplus_tree->insert(-8935141660703064064, -8935141660703064064);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);
    ASSERT_EQ(radix_tree->get_value(-9079256848778919936), -9079256848778919936);
    ASSERT_EQ(radix_tree->get_value(-9007199254740992000), -9007199254740992000);
    ASSERT_EQ(radix_tree->get_value(-8935141660703064064), -8935141660703064064);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
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
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        logger->debug("i: {}, value: {}", i, values[i]);
        ASSERT_EQ(radix_tree->get_value(values[i]), values[i]);
        ASSERT_TRUE(is_compressed());
        ASSERT_TRUE(leaf_depth_correct());
        ASSERT_TRUE(key_matches());
    }

    // testing if values are not in there
    for (int i = 0; i < 1000; i++)
    {
        int64_t value = dist(generator); // generate a random number

        while (unique_values.count(value))
        {
            value = dist(generator);
        }
        ASSERT_EQ(radix_tree->get_value(value), INT64_MIN);
        ASSERT_TRUE(is_compressed());
        ASSERT_TRUE(leaf_depth_correct());
        ASSERT_TRUE(key_matches());
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
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        logger->debug("i: {}, value: {}", i, values[i]);
        ASSERT_EQ(radix_tree->get_value(values[i]), values[i]);
        ASSERT_TRUE(is_compressed());
        ASSERT_TRUE(leaf_depth_correct());
        ASSERT_TRUE(key_matches());
    }

    // testing if values are not in there
    for (int i = 0; i < 1000; i++)
    {
        int64_t value = dist(rd); // generate a random number

        while (unique_values.count(value))
        {
            value = dist(rd);
        }
        ASSERT_EQ(radix_tree->get_value(value), INT64_MIN);
        ASSERT_TRUE(is_compressed());
        ASSERT_TRUE(leaf_depth_correct());
        ASSERT_TRUE(key_matches());
    }
}

// Isolated testing of a few use cases that I identified as correct during manual testing
TEST_F(RadixTreeTest, DeleteWithoutResizing)
{
    bplus_tree->insert(-9223372036854775807 - 1, -9223372036854775807 - 1);
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), -9223372036854775807 - 1);

    bplus_tree->delete_value(-9223372036854775807 - 1);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, DeleteWithResizing)
{
    bplus_tree->insert(-9223372036854775807 - 1, -9223372036854775807 - 1);
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775806, -9223372036854775806);
    bplus_tree->insert(-9223372036854775805, -9223372036854775805);
    bplus_tree->insert(-9223372036854775804, -9223372036854775804);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775806), -9223372036854775806);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775805), -9223372036854775805);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775804), -9223372036854775804);

    bplus_tree->delete_value(-9223372036854775806);
    bplus_tree->delete_value(-9223372036854775805);
    bplus_tree->delete_value(-9223372036854775804);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), -9223372036854775807 - 1);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775806), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775805), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775804), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, DeleteWithRemovalOfNode)
{
    bplus_tree->insert(-9223372036854775807 - 1, -9223372036854775807 - 1);
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775806, -9223372036854775806);
    bplus_tree->insert(-9223372036854775805, -9223372036854775805);
    bplus_tree->insert(-9223372036854775804, -9223372036854775804);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), -9223372036854775807 - 1);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775806), -9223372036854775806);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775805), -9223372036854775805);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775804), -9223372036854775804);

    bplus_tree->delete_value(-9223372036854775807 - 1);
    bplus_tree->delete_value(-9223372036854775807);
    bplus_tree->delete_value(-9223372036854775806);
    bplus_tree->delete_value(-9223372036854775805);
    bplus_tree->delete_value(-9223372036854775804);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775806), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775805), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775804), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, DeleteWithRemovalOfNodeFurtherUpTheTree)
{
    bplus_tree->insert(-9223372036854775807 - 1, -9223372036854775807 - 1);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223372032559808511, -9223372032559808511);
    bplus_tree->insert(-9223372032559808510, -9223372032559808510);
    bplus_tree->insert(-9223372032559808509, -9223372032559808509);
    bplus_tree->insert(-9223372032559808508, -9223372032559808508);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);

    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808511), -9223372032559808511);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808510), -9223372032559808510);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808509), -9223372032559808509);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808508), -9223372032559808508);

    bplus_tree->delete_value(-9223372032559808512);
    bplus_tree->delete_value(-9223372032559808511);
    bplus_tree->delete_value(-9223372032559808510);
    bplus_tree->delete_value(-9223372032559808509);
    bplus_tree->delete_value(-9223372032559808508);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), -9223372036854775807 - 1);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808511), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808510), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808509), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808508), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, DeleteWithDecreaseAndDeleteOfInnerNode)
{
    bplus_tree->insert(-9223372036854775807 - 1, -9223372036854775807 - 1);
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854710272, -9223372036854710272);
    bplus_tree->insert(-9223372036837998592, -9223372036837998592);
    bplus_tree->insert(-9223372032559808512, -9223372032559808512);
    bplus_tree->insert(-9223372028264841216, -9223372028264841216);
    bplus_tree->insert(-9223372023969873920, -9223372023969873920);
    bplus_tree->insert(-9223372019674906624, -9223372019674906624);
    bplus_tree->insert(-9223372015379939328, -9223372015379939328);
    bplus_tree->insert(-9223370937343148032, -9223370937343148032);
    bplus_tree->insert(-9223090561878065152, -9223090561878065152);
    bplus_tree->insert(-9151314442816847872, -9151314442816847872);

    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), -9223372032559808512);
    ASSERT_EQ(radix_tree->get_value(-9223372028264841216), -9223372028264841216);
    ASSERT_EQ(radix_tree->get_value(-9223372023969873920), -9223372023969873920);
    ASSERT_EQ(radix_tree->get_value(-9223372019674906624), -9223372019674906624);
    ASSERT_EQ(radix_tree->get_value(-9223372015379939328), -9223372015379939328);

    bplus_tree->delete_value(-9223372032559808512);
    bplus_tree->delete_value(-9223372028264841216);
    bplus_tree->delete_value(-9223372023969873920);
    bplus_tree->delete_value(-9223372019674906624);
    bplus_tree->delete_value(-9223372015379939328);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), -9223372036854775807 - 1);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775552), -9223372036854775552);
    ASSERT_EQ(radix_tree->get_value(-9223372036854710272), -9223372036854710272);
    ASSERT_EQ(radix_tree->get_value(-9223372036837998592), -9223372036837998592);
    ASSERT_EQ(radix_tree->get_value(-9223372032559808512), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372028264841216), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372023969873920), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372019674906624), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372015379939328), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223370937343148032), -9223370937343148032);
    ASSERT_EQ(radix_tree->get_value(-9223090561878065152), -9223090561878065152);
    ASSERT_EQ(radix_tree->get_value(-9151314442816847872), -9151314442816847872);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, DeleteFromLeafRoot)
{
    bplus_tree->insert(-9223372036854775807 - 1, -9223372036854775807 - 1);
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775806, -9223372036854775806);
    bplus_tree->insert(-9223372036854775805, -9223372036854775805);
    bplus_tree->insert(-9223372036854775804, -9223372036854775804);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), -9223372036854775807 - 1);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775806), -9223372036854775806);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775805), -9223372036854775805);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775804), -9223372036854775804);

    bplus_tree->delete_value(-9223372036854775807 - 1);
    bplus_tree->delete_value(-9223372036854775807);
    bplus_tree->delete_value(-9223372036854775806);
    bplus_tree->delete_value(-9223372036854775805);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775806), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775805), INT64_MIN);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());

    bplus_tree->delete_value(-9223372036854775804);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775804), INT64_MIN);
}

TEST_F(RadixTreeTest, DeleteWithDepthTwo)
{
    bplus_tree->insert(-9223372036854775552, -9223372036854775552);
    bplus_tree->insert(-9223372036854775807 - 1, -9223372036854775807 - 1);
    bplus_tree->insert(-9223372036854775807, -9223372036854775807);
    bplus_tree->insert(-9223372036854775806, -9223372036854775806);
    bplus_tree->insert(-9223372036854775805, -9223372036854775805);
    bplus_tree->insert(-9223372036854775804, -9223372036854775804);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), -9223372036854775807 - 1);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), -9223372036854775807);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775806), -9223372036854775806);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775805), -9223372036854775805);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775804), -9223372036854775804);

    bplus_tree->delete_value(-9223372036854775807 - 1);
    bplus_tree->delete_value(-9223372036854775807);
    bplus_tree->delete_value(-9223372036854775806);
    bplus_tree->delete_value(-9223372036854775805);

    ASSERT_EQ(radix_tree->get_value(-9223372036854775807 - 1), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775807), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775806), INT64_MIN);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775805), INT64_MIN);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());

    bplus_tree->delete_value(-9223372036854775804);
    ASSERT_EQ(radix_tree->get_value(-9223372036854775804), INT64_MIN);
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
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        ASSERT_EQ(radix_tree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());

    // delete 500 elements
    for (int i = 0; i < 500; i++)
    {
        int64_t value = values[i];
        unique_values.erase(value);
        bplus_tree->delete_value(value);
    }

    for (int i = 0; i < 500; i++)
    {
        ASSERT_EQ(radix_tree->get_value(values[i]), INT64_MIN);
    }
    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());

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
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        ASSERT_EQ(radix_tree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());

    // delete all
    for (int i = 0; i < 1000; i++)
    {
        int64_t value = values[i];
        unique_values.erase(value);
        bplus_tree->delete_value(value);

        ASSERT_TRUE(is_compressed());
        ASSERT_TRUE(leaf_depth_correct());
        ASSERT_TRUE(key_matches());
    }

    ASSERT_FALSE(get_root());
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
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        ASSERT_EQ(radix_tree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());

    // delete 500 elements
    for (int i = 0; i < 500; i++)
    {
        int64_t value = values[i];
        unique_values.erase(value);
        bplus_tree->delete_value(value);
    }

    for (int i = 0; i < 500; i++)
    {
        ASSERT_EQ(radix_tree->get_value(values[i]), INT64_MIN);
    }
    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());

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
        bplus_tree->insert(value, value);
    }

    for (int i = 0; i < 1000; i++)
    {
        ASSERT_EQ(radix_tree->get_value(values[i]), values[i]);
    }
    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());

    // delete all
    for (int i = 0; i < 1000; i++)
    {
        int64_t value = values[i];
        unique_values.erase(value);
        bplus_tree->delete_value(value);

        ASSERT_TRUE(is_compressed());
        ASSERT_TRUE(leaf_depth_correct());
        ASSERT_TRUE(key_matches());
    }

    ASSERT_FALSE(get_root());
}

TEST_F(RadixTreeTest, GetPage)
{
    BHeader *header = (BHeader *)malloc(96);
    header->page_id = 0;
    radix_tree->insert(-9223372036854775807 - 1, 0, header);
    radix_tree->insert(-9223372036854775552, 0, header);
    radix_tree->insert(-9223372036854710272, 0, header);
    radix_tree->insert(-9223372036837998592, 0, header);
    radix_tree->insert(-9223372032559808512, 0, header);
    radix_tree->insert(-9223370937343148032, 0, header);
    radix_tree->insert(-9223090561878065152, 0, header);
    radix_tree->insert(-9151314442816847872, 0, header);

    ASSERT_EQ(get_page(-9223372036854775807 - 1), header);
    ASSERT_EQ(get_page(-9223372036854775552), header);
    ASSERT_EQ(get_page(-9223372036854710272), header);
    ASSERT_EQ(get_page(-9223372036837998592), header);
    ASSERT_EQ(get_page(-9223372032559808512), header);
    ASSERT_EQ(get_page(-9223370937343148032), header);
    ASSERT_EQ(get_page(-9223090561878065152), header);
    ASSERT_EQ(get_page(-9151314442816847872), header);

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, UpdateRangeWithSeed42)
{
    std::mt19937 generator(42); // 42 is the seed value
    std::uniform_int_distribution<int64_t> dist(INT64_MIN + 1, INT64_MAX);
    std::unordered_set<int64_t> unique_values;
    int64_t values[100];
    BHeader *header = (BHeader *)malloc(96);
    header->page_id = 0;

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
        radix_tree->insert(value, 0, header);
    }

    std::sort(values, values + 100);
    BHeader *new_header = (BHeader *)malloc(96);
    new_header->page_id = 1;

    radix_tree->update_range(values[20], values[80], 1, new_header);

    for (int i = 0; i < 100; i++)
    {
        if (i >= 20 && i <= 80)
            ASSERT_EQ(get_page(values[i]), new_header);
        else
            ASSERT_EQ(get_page(values[i]), header);
    }
    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, Size)
{
    BHeader *header = (BHeader *)malloc(96);
    radix_tree->insert(-9223372036854775807 - 1, 0, header);
    radix_tree->insert(-9223372036854775552, 0, header);
    radix_tree->insert(-9223372036854710272, 0, header);
    radix_tree->insert(-9223372036837998592, 0, header);
    radix_tree->insert(-9223372032559808512, 0, header);
    radix_tree->insert(-9223370937343148032, 0, header);
    radix_tree->insert(-9223090561878065152, 0, header);
    radix_tree->insert(-9151314442816847872, 0, header);
    ASSERT_EQ(get_current_size(), 1088);

    radix_tree->insert(-9223372036854775807, 0, header);
    ASSERT_EQ(get_current_size(), 1104);

    radix_tree->delete_reference(-9223372036854710272);
    ASSERT_EQ(get_current_size(), 960);

    radix_tree->delete_reference(-9151314442816847872);
    ASSERT_EQ(get_current_size(), 816);

    radix_tree->insert(-9223372028264841216, 0, header);
    radix_tree->insert(-9223372023969873920, 0, header);
    radix_tree->insert(-9223372019674906624, 0, header);
    ASSERT_EQ(get_current_size(), 1160);

    radix_tree->delete_reference(-9223372028264841216);
    ASSERT_EQ(get_current_size(), 976);

    radix_tree->delete_reference(-9223372036854775807);
    radix_tree->delete_reference(-9223372036854775807 - 1);
    radix_tree->delete_reference(-9223372036854775552);
    radix_tree->delete_reference(-9223372036837998592);
    radix_tree->delete_reference(-9223372032559808512);
    radix_tree->delete_reference(-9223372023969873920);
    radix_tree->delete_reference(-9223372019674906624);
    radix_tree->delete_reference(-9223370937343148032);
    radix_tree->delete_reference(-9151314442816847872);
    ASSERT_EQ(get_current_size(), 0);
}

TEST_F(RadixTreeTest, UpdateWithSeed42)
{
    DataManager<PAGE_SIZE> data_manager = DataManager<PAGE_SIZE>(storage_manager, buffer_manager, bplus_tree, radix_tree);
    std::mt19937 generator(42); // 42 is the seed value
    std::uniform_int_distribution<int64_t> dist(INT64_MIN + 1, INT64_MAX);
    std::unordered_set<int64_t> unique_values;
    int64_t values[100];
    BHeader *header = (BHeader *)malloc(96);
    header->page_id = 0;

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
        data_manager.insert(value, value);
    }

    for (int i = 0; i < 100; i++)
    {
        radix_tree->update(values[i], values[i] + 1);
    }

    for (int i = 0; i < 100; i++)
    {
        ASSERT_EQ(bplus_tree->get_value(values[i]), values[i] + 1);
    }
    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}

TEST_F(RadixTreeTest, ScanWithSeed42)
{
    DataManager<PAGE_SIZE> data_manager = DataManager<PAGE_SIZE>(storage_manager, buffer_manager, bplus_tree, radix_tree);
    std::mt19937 generator(42); // 42 is the seed value
    std::uniform_int_distribution<int64_t> dist(INT64_MIN + 1, INT64_MAX);
    std::unordered_set<int64_t> unique_values;
    int64_t values[100];
    BHeader *header = (BHeader *)malloc(96);
    header->page_id = 0;

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
        data_manager.insert(value, value);
    }

    std::sort(values, values + 100);
    BHeader *new_header = (BHeader *)malloc(96);
    new_header->page_id = 1;

    ASSERT_EQ(bplus_tree->scan(values[20], 40), radix_tree->scan(values[20], 40));

    ASSERT_TRUE(is_compressed());
    ASSERT_TRUE(leaf_depth_correct());
    ASSERT_TRUE(key_matches());
}