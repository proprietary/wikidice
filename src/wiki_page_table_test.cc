#include "wiki_page_table.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace net_zelcon::wikidice;

class WikiPageTableTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a temporary directory for the test database
        tempDir =
            std::filesystem::temp_directory_path() / "wiki_page_table_test";
        std::filesystem::create_directory(tempDir);

        // Create a WikiPageTable instance
        wikiPageTable = std::make_unique<WikiPageTable>(tempDir);
    }

    void TearDown() override {
        // Clean up the temporary directory
        std::filesystem::remove_all(tempDir);
    }

    std::filesystem::path tempDir;
    std::unique_ptr<WikiPageTable> wikiPageTable;
};

TEST_F(WikiPageTableTest, AddPage) {
    // Create a sample PageTableRow
    PageTableRow pageRow;
    pageRow.page_id = 1ULL;
    pageRow.page_title = "Test Page";
    pageRow.is_redirect = false;

    // Add the page to the WikiPageTable
    wikiPageTable->add_page(pageRow);

    // Retrieve the page from the WikiPageTable
    auto retrievedPage = wikiPageTable->find(pageRow.page_id);

    // Verify that the retrieved page matches the original page
    ASSERT_TRUE(retrievedPage.has_value());
    EXPECT_EQ(retrievedPage.value().page_id, pageRow.page_id);
    EXPECT_EQ(retrievedPage.value().page_title, pageRow.page_title);
    EXPECT_EQ(retrievedPage.value().is_redirect, pageRow.is_redirect);
}

TEST_F(WikiPageTableTest, FindNonExistentPage) {
    // Try to find a non-existent page
    auto retrievedPage = wikiPageTable->find(999);

    // Verify that the retrieved page is empty
    ASSERT_FALSE(retrievedPage.has_value());
}