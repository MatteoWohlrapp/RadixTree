#include "gtest/gtest.h"
#include "../src/data/buffer_manager.h"
#include "../src/configuration.h"

class BufferManagerTest : public ::testing::Test
{
    friend class BufferManager;

protected:
    BufferManager *buffer_manager;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::path bitmap = "bitmap.bin";
    std::filesystem::path data = "data.bin";
    int page_size = 32;
    int buffer_size = 2;
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

    void SetUp() override
    {
        std::filesystem::remove(base_path / bitmap);
        std::filesystem::remove(base_path / data);
        buffer_manager = new BufferManager(new StorageManager(base_path, page_size), buffer_size, page_size);
    }

    void overwrite_buffer_manager()
    {
        buffer_manager = new BufferManager(new StorageManager(base_path, page_size), buffer_size, page_size);
    }

    int get_current_buffer_size()
    {
        return buffer_manager->current_buffer_size;
    }

    std::map<uint64_t, BFrame *> get_page_id_map()
    {
        return buffer_manager->page_id_map;
    }
};

TEST_F(BufferManagerTest, PageCreation)
{
    BHeader *header = buffer_manager->create_new_page();
    ASSERT_EQ(header->page_id, 1);
    ASSERT_EQ(get_current_buffer_size(), 1);
    auto page_id_map = get_page_id_map();
    std::map<uint64_t, BFrame *>::iterator it = page_id_map.find(header->page_id);
    ASSERT_EQ(it->second->dirty, true);
    ASSERT_EQ(it->second->marked, true);

    header = buffer_manager->create_new_page();
    ASSERT_EQ(get_current_buffer_size(), 2);
}

TEST_F(BufferManagerTest, PageRequest)
{
    buffer_manager->create_new_page();
    buffer_manager->create_new_page();

    BHeader *header = buffer_manager->request_page(1);
    ASSERT_EQ(header->page_id, 1);
    ASSERT_EQ(get_current_buffer_size(), 2);
}

TEST_F(BufferManagerTest, FixAndUnfixPage)
{
    BHeader *header = buffer_manager->create_new_page();
    auto page_id_map = get_page_id_map();
    std::map<uint64_t, BFrame *>::iterator it = page_id_map.find(header->page_id);
    ASSERT_EQ(it->second->fix_count, 1);
    buffer_manager->unfix_page(1, true);
    ASSERT_EQ(it->second->fix_count, 0);
}

TEST_F(BufferManagerTest, BufferFullWhenCreatingPage)
{
    BHeader *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(header->page_id, false);
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(header->page_id, false);
    header = buffer_manager->create_new_page();
    ASSERT_EQ(get_current_buffer_size(), 2);
    ASSERT_EQ(header->page_id, 3);
    auto page_id_map = get_page_id_map();
    ASSERT_TRUE((page_id_map.find(1) != page_id_map.end() && page_id_map.find(2) == page_id_map.end()) || (page_id_map.find(1) == page_id_map.end() && page_id_map.find(2) != page_id_map.end()));
}

TEST_F(BufferManagerTest, BufferFullWhenRequestingPage)
{
    BHeader *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(header->page_id, false);
    header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(header->page_id, false);
    header = buffer_manager->create_new_page();
    auto page_id_map = get_page_id_map();
    int page_id = 1;
    if (page_id_map.find(1) != page_id_map.end())
    {
        page_id = 2;
    }
    header = buffer_manager->request_page(page_id);
    ASSERT_EQ(get_current_buffer_size(), 2);
    ASSERT_EQ(header->page_id, page_id);
    page_id_map = get_page_id_map();
    ASSERT_TRUE((page_id_map.find(3) != page_id_map.end()) && (page_id_map.find(page_id) != page_id_map.end()));
}

TEST_F(BufferManagerTest, DeletingPage)
{
    BHeader *header = buffer_manager->create_new_page();
    buffer_manager->unfix_page(1, false);

    auto page_id_map = get_page_id_map();
    ASSERT_FALSE(page_id_map.find(1) == page_id_map.end());
    buffer_manager->delete_page(1);
    page_id_map = get_page_id_map();
    ASSERT_TRUE(page_id_map.find(1) == page_id_map.end());
}

TEST_F(BufferManagerTest, DestroyPage)
{
    BHeader *header = buffer_manager->create_new_page();

    buffer_manager->destroy();

    overwrite_buffer_manager();

    BHeader *loaded_header = buffer_manager->request_page(1);
    ASSERT_EQ(header->page_id, 1);
}