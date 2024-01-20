#include "wiki_page_table.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace net_zelcon::wikidice;

class WikiPageTableTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a temporary directory for the test database
        temp_dir =
            std::filesystem::temp_directory_path() / "wiki_page_table_test";
        std::filesystem::create_directory(temp_dir);

        // Create a WikiPageTable instance
        wiki_page_table = std::make_unique<WikiPageTable>(temp_dir);
    }

    void TearDown() override {
        // Clean up the temporary directory
        std::filesystem::remove_all(temp_dir);
    }

    std::filesystem::path temp_dir;
    std::unique_ptr<WikiPageTable> wiki_page_table;
};

TEST_F(WikiPageTableTest, AddPage) {
    // Create a sample PageTableRow
    entities::PageTableRow page_row;
    page_row.page_id = 1ULL;
    page_row.page_title = "Test Page";
    page_row.is_redirect = false;

    // Add the page to the WikiPageTable
    wiki_page_table->add_page(page_row);

    // Retrieve the page from the WikiPageTable
    auto retrieved_page = wiki_page_table->find(page_row.page_id);

    // Verify that the retrieved page matches the original page
    ASSERT_TRUE(retrieved_page.has_value());
    EXPECT_EQ(retrieved_page.value().page_id, page_row.page_id);
    EXPECT_EQ(retrieved_page.value().page_title, page_row.page_title);
    EXPECT_EQ(retrieved_page.value().is_redirect, page_row.is_redirect);
}

TEST_F(WikiPageTableTest, FindNonExistentPage) {
    // Try to find a non-existent page
    auto retrieved_page = wiki_page_table->find(999);

    // Verify that the retrieved page is empty
    ASSERT_FALSE(retrieved_page.has_value());
}