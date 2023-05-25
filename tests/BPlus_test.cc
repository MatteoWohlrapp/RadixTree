#include "gtest/gtest.h"
#include "../src/btree/BPlus.h"
#include "../src/data/BufferManager.h"
#include "../src/Configuration.h"

class BPlusTest : public ::testing::Test
{
    friend class BPlus;

protected:
    std::unique_ptr<BPlus> bplus;
    std::filesystem::path base_path = "../tests/temp/";
    int page_size = 16;
    int buffer_size = 2;
    std::shared_ptr<spdlog::logger> logger = spdlog::get("logger");

    void SetUp() override
    {
        bplus = std::make_unique<BPlus>(std::make_shared<BufferManager>(std::make_shared<StorageManager>(base_path, page_size), buffer_size));
    }

    void TearDown() override
    {
    }
};

TEST_F(BPlusTest, Test)
{
}