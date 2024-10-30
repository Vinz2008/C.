#include <gtest/gtest.h>

#include "../../src/utils.h"

TEST(stringDistance, TestStringDistance) {
  EXPECT_EQ(stringDistance("aaa", "aab"), 1);
  EXPECT_EQ(stringDistance("aaaaaaa", "aabaaab"), 2);
}

TEST(getFileExtension, TestgetFileExtension) {
  EXPECT_EQ(getFileExtension("test.cpoint"), "cpoint");
  EXPECT_EQ(getFileExtension("test."), "");
  EXPECT_EQ(getFileExtension("test"), "");
}

TEST(is_char_one_of_these, Test_is_char_one_of_these) {
  EXPECT_EQ(is_char_one_of_these('a', "a"), true);
  EXPECT_EQ(is_char_one_of_these('a', "kldkdkladkdjdj"), true);
  EXPECT_EQ(is_char_one_of_these('c',"jklmnop"), false);
}



int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}