#include "gtest/gtest.h"
#include "../src/data/StorageManager.h"


TEST(StorageManager, FolderInitialization)
{
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::remove(base_path / "offsets.bin");
    std::filesystem::remove(base_path / "data.bin");

    StorageManager storage_manager(base_path);
    ASSERT_TRUE(std::filesystem::exists(base_path));
    ASSERT_TRUE(std::filesystem::exists(base_path / "offsets.bin"));
    ASSERT_TRUE(std::filesystem::exists(base_path / "data.bin"));
}

TEST(StorageManager, OffsetMapSavingAndLoading)
{
    int page_size = 10;
    std::filesystem::path base_path = "../tests/db_in_out/";
    std::filesystem::remove(base_path / "offsets.bin");
    std::filesystem::remove(base_path / "data.bin");

    StorageManager storage_manager(base_path, page_size);

    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;

    storage_manager.save_page(header);

    header->page_id = 2;
    storage_manager.save_page(header);

    header->page_id = 3;
    storage_manager.save_page(header);

    storage_manager.destroy();

    // it will access the db/offsets.bin and in the process

    StorageManager storage_manager_new(base_path, page_size);

    for (uint32_t i = 1; i <= 3; i++)
    {
        std::map<uint32_t, uint32_t>::iterator it = storage_manager_new.page_id_offset_map.find(i);

        ASSERT_TRUE(it != storage_manager_new.page_id_offset_map.end());
        ASSERT_EQ(it->first, i);
        ASSERT_EQ(it->second, (i - 1) * page_size);
    }
}

TEST(StorageManager, SaveAndLoadNewPage)
{
    int page_size = 10;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::remove(base_path / "offsets.bin");
    std::filesystem::remove(base_path / "data.bin");
    StorageManager storage_manager(base_path, page_size);

    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;
    header->inner = true;

    // cast to char array and fill latest two bytes with values to check later
    char *arr = (char *)header;
    arr[8] = 1;
    arr[9] = 2;

    // save the page to the file
    storage_manager.save_page(header);

    // Re-load the page and check if it was saved correctly
    Header *loaded_header = (Header *)malloc(page_size);
    storage_manager.load_page(loaded_header, 1);

    // test for header properties
    ASSERT_EQ(loaded_header->page_id, 1);
    ASSERT_EQ(loaded_header->inner, true);
    // test for data properties
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[8], 1);
    ASSERT_EQ(arr[9], 2);
}

TEST(StorageManager, OverwritePageThenInsertNewThenOverwritePage)
{
    int page_size = 10;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::remove(base_path / "offsets.bin");
    std::filesystem::remove(base_path / "data.bin");
    StorageManager storage_manager(base_path, page_size);

    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;
    header->inner = true;

    // cast to char array and fill latest two bytes with values to check later
    char *arr = (char *)header;
    arr[8] = 1;
    arr[9] = 2;

    // save the page to the file
    storage_manager.save_page(header);

    // rewrite the properties
    header->inner = false;
    arr[8] = 3;
    arr[9] = 4;

    // save the changes to the page
    storage_manager.save_page(header);

    // Re-load the page and check if the changes were overwritten correctly
    // If saving new property works was shown before
    Header *loaded_header = (Header *)malloc(page_size);
    storage_manager.load_page(loaded_header, 1);

    // test for page id
    ASSERT_EQ(loaded_header->page_id, 1);
    ASSERT_EQ(loaded_header->inner, false);
    // test for filled byte array in the end
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[8], 3);
    ASSERT_EQ(arr[9], 4);

    // second part, add new page and overwrite it as well
    // rewrite the properties
    header->page_id = 2;
    header->inner = true;
    arr = (char *)header;
    arr[8] = 5;
    arr[9] = 6;

    // save the changes to the page
    storage_manager.save_page(header);

    // load new page
    storage_manager.load_page(loaded_header, 2);

    ASSERT_EQ(loaded_header->page_id, 2);
    ASSERT_EQ(loaded_header->inner, true);
    // test for filled byte array in the end
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[8], 5);
    ASSERT_EQ(arr[9], 6);

    header->inner = false;
    arr = (char *)header;
    arr[8] = 7;
    arr[9] = 8;

    // save the changes to the page
    storage_manager.save_page(header);

    // load new page
    storage_manager.load_page(loaded_header, 2);

    ASSERT_EQ(loaded_header->page_id, 2);
    ASSERT_EQ(loaded_header->inner, false);
    // test for filled byte array in the end
    arr = (char *)loaded_header;
    ASSERT_EQ(arr[8], 7);
    ASSERT_EQ(arr[9], 8);
}


// current offset
TEST(StorageManager, CurrentOffset)
{
    int page_size = 10;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::remove(base_path / "offsets.bin");
    std::filesystem::remove(base_path / "data.bin");
    StorageManager storage_manager(base_path, page_size);

    ASSERT_EQ(storage_manager.current_offset, 0);

    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;

    // save the page to the file
    storage_manager.save_page(header);

    // check if offset increased
    ASSERT_EQ(storage_manager.current_offset, page_size);

    // save the page to the file
    storage_manager.save_page(header);
    // already in buffer so offset should not increase
    ASSERT_EQ(storage_manager.current_offset, page_size);
}

// test for highest id
TEST(StorageManager, HighestId)
{
    int page_size = 10;
    std::filesystem::path base_path = "../tests/temp/";
    std::filesystem::remove(base_path / "offsets.bin");
    std::filesystem::remove(base_path / "data.bin");
    StorageManager storage_manager(base_path, page_size);

    ASSERT_EQ(storage_manager.highest_page_id(), 0);

    Header *header = (Header *)malloc(page_size);
    header->page_id = 1;

    // save the page to the file
    storage_manager.save_page(header);

    header = (Header *)malloc(page_size);
    header->page_id = 5;

    // save the page to the file
    storage_manager.save_page(header);

    // check for highest saved id
    ASSERT_EQ(storage_manager.highest_page_id(), 5);
} 