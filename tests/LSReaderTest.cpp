#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include "LSReader.h"

class LSReaderTest : public ::testing::Test {
protected:
    string validFile = "test_data/Logo_p.e5";

    void SetUp() override {
        spdlog::set_level(spdlog::level::off);
        createFile("empty_file.txt", "");
        createFile("wrong_format.txt", "this is not a valid format");
        createFile("truncated_file.txt", "Ls11 this file is truncated");
    }

    void TearDown() override {
        remove("empty_file.txt");
        remove("wrong_format.txt");
        remove("truncated_file.txt");
        spdlog::set_level(spdlog::level::info);
    }

    void createFile(const std::string &filename, const std::string &content) {
        std::ofstream file(filename);
        file << content;
        file.close();
    }
};

TEST_F(LSReaderTest, ReadFileNotExist) {
    LSReader reader("a_file_not_exist");
    EXPECT_TRUE(reader.isFail());
}

TEST_F(LSReaderTest, ReadEmptyFile) {
    LSReader reader("empty_file.txt");
    EXPECT_TRUE(reader.isFail());
}

TEST_F(LSReaderTest, ReadWrongFormat) {
    LSReader reader("wrong_format.txt");
    EXPECT_TRUE(reader.isFail());
}

TEST_F(LSReaderTest, ReadTruncated) {
    LSReader reader("truncated_file.tx");
    EXPECT_TRUE(reader.isFail());
}

TEST_F(LSReaderTest, ReadValidFile) {
    LSReader reader(validFile);
    EXPECT_FALSE(reader.isFail());
    EXPECT_EQ(reader.getNumOfFiles(), 4);

    // decode beyond valid file
    char *buffer;
    EXPECT_EQ(reader.getFile(5, &buffer), -1);
    EXPECT_EQ(buffer, nullptr);

    // decode a valid file
    EXPECT_EQ(reader.getFile(1, &buffer), 768);
    EXPECT_NE(buffer, nullptr);
    delete[] buffer;

    // decode another valid file
    EXPECT_EQ(reader.getFile(2, &buffer), 738);
    EXPECT_NE(buffer, nullptr);
    delete[] buffer;

    // decode last file
    EXPECT_EQ(reader.getFile(3, &buffer), 690);
    EXPECT_NE(buffer, nullptr);
    delete[] buffer;
}