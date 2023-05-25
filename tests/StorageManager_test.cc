#include "gtest/gtest.h"
#include "../src/data/StorageManager.h"
#include "../src/Configuration.h"

class StorageManagerTest : public ::testing::Test
{
    friend class StorageManager;

protected:
    std::unique_ptr<StorageManager> storage_manager;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::path bitmap = "bitmap.bin";
    std::filesystem::path data = "data.bin";
    int page_size = 32;
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

    void SetUp() override
    {
        std::filesystem::remove(base_path / bitmap);
        std::filesystem::remove(base_path / data);
        storage_manager = std::make_unique<StorageManager>(base_path, page_size);
    }

    boost::dynamic_bitset<> get_free_space_map()
    {
        return storage_manager->free_space_map;
    }

    int get_current_page_count()
    {
        return storage_manager->current_page_count;
    }

    int get_next_free_space()
    {
        return storage_manager->next_free_space;
    }

    void overwrite_storage_manager()
    {
        storage_manager = std::make_unique<StorageManager>(base_path, page_size);
    }
};

TEST_F(StorageManagerTest, FolderInitialization)
{
    ASSERT_TRUE(std::filesystem::exists(base_path));
    ASSERT_TRUE(std::filesystem::exists(base_path / bitmap));
    ASSERT_TRUE(std::filesystem::exists(base_path / data));
}

TEST_F(StorageManagerTest, BitmapSavingAndLoading)
{

    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;

    storage_manager->save_page(header);

    header->page_id = 2;
    storage_manager->save_page(header);

    header->page_id = 3;
    storage_manager->save_page(header);

    storage_manager->destroy();

    overwrite_storage_manager();

    for (int i = 0; i < 4; i++)
    {
        ASSERT_EQ(get_free_space_map()[i], 0);
    }
}

TEST_F(StorageManagerTest, SaveAndLoadNewPage)
{
    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;
    header->inner = true;

    // cast to char array and fill latest two bytes with values to check later
    char *arr = (char *)header;
    arr[page_size - 2] = 1;
    arr[page_size - 1] = 2;

    // save the page to the file
    storage_manager->save_page(header);

    // Re-load the page and check if it was saved correctly
    Header *loaded_header = (Header *)malloc(page_size);
    storage_manager->load_page(loaded_header, 1);

    // test for header properties
    ASSERT_EQ(loaded_header->page_id, 1);
    ASSERT_EQ(loaded_header->inner, true);
    // test for data properties
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[page_size - 2], 1);
    ASSERT_EQ(arr[page_size - 1], 2);
}

TEST_F(StorageManagerTest, SaveAndLoadNewPageFromMemory)
{
    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;
    header->inner = true;

    // cast to char array and fill latest two bytes with values to check later
    char *arr = (char *)header;
    arr[page_size - 2] = 1;
    arr[page_size - 1] = 2;

    // save the page to the file
    storage_manager->save_page(header);
    storage_manager->destroy();

    overwrite_storage_manager();

    // Re-load the page and check if it was saved correctly
    Header *loaded_header = (Header *)malloc(page_size);
    storage_manager->load_page(loaded_header, 1);

    // test for header properties
    ASSERT_EQ(loaded_header->page_id, 1);
    ASSERT_EQ(loaded_header->inner, true);
    // test for data properties
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[page_size - 2], 1);
    ASSERT_EQ(arr[page_size - 1], 2);
}

TEST_F(StorageManagerTest, OverwritePageThenInsertNewThenOverwritePage)
{
    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;
    header->inner = true;

    // cast to char array and fill latest two bytes with values to check later
    char *arr = (char *)header;
    arr[page_size - 2] = 1;
    arr[page_size - 1] = 2;

    // save the page to the file
    storage_manager->save_page(header);

    // rewrite the properties
    header->inner = false;
    arr[page_size - 2] = 3;
    arr[page_size - 1] = 4;

    // save the changes to the page
    storage_manager->save_page(header);

    // Re-load the page and check if the changes were overwritten correctly
    // If saving new property works was shown before
    Header *loaded_header = (Header *)malloc(page_size);
    storage_manager->load_page(loaded_header, 1);

    // test for page id
    ASSERT_EQ(loaded_header->page_id, 1);
    ASSERT_EQ(loaded_header->inner, false);
    // test for filled byte array in the end
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[page_size - 2], 3);
    ASSERT_EQ(arr[page_size - 1], 4);

    // second part, add new page and overwrite it as well
    // rewrite the properties
    header->page_id = 2;
    header->inner = true;
    arr = (char *)header;
    arr[page_size - 2] = 5;
    arr[page_size - 1] = 6;

    // save the changes to the page
    storage_manager->save_page(header);

    // load new page
    storage_manager->load_page(loaded_header, 2);

    ASSERT_EQ(loaded_header->page_id, 2);
    ASSERT_EQ(loaded_header->inner, true);
    // test for filled byte array in the end
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[page_size - 2], 5);
    ASSERT_EQ(arr[page_size - 1], 6);

    header->inner = false;
    arr = (char *)header;
    arr[page_size - 2] = 7;
    arr[page_size - 1] = 8;

    // save the changes to the page
    storage_manager->save_page(header);

    // load new page
    storage_manager->load_page(loaded_header, 2);

    ASSERT_EQ(loaded_header->page_id, 2);
    ASSERT_EQ(loaded_header->inner, false);
    // test for filled byte array in the end
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[page_size - 2], 7);
    ASSERT_EQ(arr[page_size - 1], 8);
}

TEST_F(StorageManagerTest, WritingBoundaries)
{
    Header *header = (Header *)malloc(page_size);
    header->page_id = 2;

    // cast to char array and fill latest two bytes with values to check later
    char *arr = (char *)header;
    arr[page_size - 2] = 1;
    arr[page_size - 1] = 2;
    storage_manager->save_page(header);

    header->page_id = 1;
    storage_manager->save_page(header);

    header->page_id = 3;
    storage_manager->save_page(header);

    // Re-load the page and check if the changes were overwritten correctly
    // If saving new property works was shown before
    Header *loaded_header = (Header *)malloc(page_size);
    storage_manager->load_page(loaded_header, 2);

    // test for page id
    ASSERT_EQ(loaded_header->page_id, 2);
    // test for filled byte array in the end
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[page_size - 2], 1);
    ASSERT_EQ(arr[page_size - 1], 2);
}

TEST_F(StorageManagerTest, InsertHigherPagesThanInitialBitmapSize)
{
    Header *header = (Header *)malloc(page_size);
    header->page_id = page_size + 10;
    char *arr = (char *)header;
    arr[page_size - 2] = 1;
    arr[page_size - 1] = 2;
    storage_manager->save_page(header);
    storage_manager->destroy();

    overwrite_storage_manager();

    // Re-load the page and check if it was saved correctly
    Header *loaded_header = (Header *)malloc(page_size);
    storage_manager->load_page(loaded_header, page_size + 10);

    // test for header properties
    ASSERT_EQ(loaded_header->page_id, page_size + 10);
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[page_size - 2], 1);
    ASSERT_EQ(arr[page_size - 1], 2);
    // test if the bitmap at that position is set correctly 
    ASSERT_EQ(get_free_space_map()[loaded_header->page_id], 0);  
    ASSERT_EQ(get_next_free_space(), 1); 
}

TEST_F(StorageManagerTest, CurrentPageCount)
{

    ASSERT_EQ(get_current_page_count(), 0);

    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;

    // save the page to the file
    storage_manager->save_page(header);

    // check if offset increased
    ASSERT_EQ(get_current_page_count(), 2);

    // save the page to the file again, triggering overwrite
    storage_manager->save_page(header);

    // already in file so page count should not increase
    ASSERT_EQ(get_current_page_count(), 2);

    header->page_id = 2;
    storage_manager->save_page(header);

    ASSERT_EQ(get_current_page_count(), 3);
}

TEST_F(StorageManagerTest, UniqueIdAfterSaving)
{
    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;
    storage_manager->save_page(header);

    header->page_id = 3;
    storage_manager->save_page(header);

    // internally makes sure that free space is also saved correctly
    ASSERT_EQ(storage_manager->get_unused_page_id(), 2);
    ASSERT_EQ(storage_manager->get_unused_page_id(), 4);
}

TEST_F(StorageManagerTest, FirstFreeAfterSaving)
{
    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;
    storage_manager->save_page(header);

    header->page_id = 3;
    storage_manager->save_page(header);

    ASSERT_EQ(storage_manager->get_unused_page_id(), 2);
    ASSERT_EQ(storage_manager->get_unused_page_id(), 4);
}