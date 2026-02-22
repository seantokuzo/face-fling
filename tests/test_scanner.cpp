/**
 * Scanner unit tests.
 */

#include <gtest/gtest.h>
#include "core/Scanner.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class ScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir = fs::temp_directory_path() / "facefling_test";
        fs::create_directories(test_dir);
    }
    
    void TearDown() override {
        // Clean up
        fs::remove_all(test_dir);
    }
    
    void create_file(const std::string& relative_path) {
        auto path = test_dir / relative_path;
        fs::create_directories(path.parent_path());
        std::ofstream(path) << "test";
    }
    
    fs::path test_dir;
};

TEST_F(ScannerTest, EmptyDirectory) {
    facefling::Scanner scanner;
    auto results = scanner.scan(test_dir.string());
    EXPECT_TRUE(results.empty());
}

TEST_F(ScannerTest, FindsImages) {
    create_file("photo1.jpg");
    create_file("photo2.png");
    create_file("document.pdf");
    
    facefling::Scanner scanner;
    auto results = scanner.scan(test_dir.string());
    
    EXPECT_EQ(results.size(), 2);
}

TEST_F(ScannerTest, RecursiveSearch) {
    create_file("photo1.jpg");
    create_file("subdir/photo2.jpg");
    create_file("subdir/deeper/photo3.jpg");
    
    facefling::Scanner scanner;
    auto results = scanner.scan(test_dir.string());
    
    EXPECT_EQ(results.size(), 3);
}

TEST_F(ScannerTest, SkipsHiddenFiles) {
    create_file("photo.jpg");
    create_file(".hidden.jpg");
    create_file(".hidden_dir/photo.jpg");
    
    facefling::Scanner scanner;
    auto results = scanner.scan(test_dir.string());
    
    EXPECT_EQ(results.size(), 1);
}

TEST_F(ScannerTest, CaseInsensitiveExtensions) {
    create_file("photo1.JPG");
    create_file("photo2.Png");
    create_file("photo3.JPEG");
    
    facefling::Scanner scanner;
    auto results = scanner.scan(test_dir.string());
    
    EXPECT_EQ(results.size(), 3);
}

TEST_F(ScannerTest, Cancellation) {
    // Create many files
    for (int i = 0; i < 100; ++i) {
        create_file("photo" + std::to_string(i) + ".jpg");
    }
    
    facefling::Scanner scanner;
    
    // Cancel after finding some files
    int found = 0;
    auto results = scanner.scan(test_dir.string(), 
        [&](size_t count, const std::string&, const std::string&) {
            found = count;
            if (count >= 10) {
                scanner.cancel();
            }
        });
    
    // Should have stopped early
    EXPECT_LT(results.size(), 100);
}
