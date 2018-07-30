#include "gtest/gtest.h"
#include "shared_array.hpp"

const int size = 8;

TEST(SharedArray, CreateDestroyNamed) {
  SharedArray<int, size> sharedIntArray("intTest-1", O_RDWR, true, false);
}

TEST(SharedArray, ReadWrite) {
  SharedArray<int, size> my_int("intTest0", O_RDWR, true, false);
  int i = 0;
  for (int i = 0; i < size; i++) {
    my_int[i] = i;
    EXPECT_EQ(i, my_int[i]);
  }
}

TEST(SharedArray, ReadOnly) {
  SharedArray<int, size> my_int("intTest1", O_RDWR, true, false);
  SharedArray<int, size> your_int("intTest1", O_RDONLY, false, false);
  for (int i = 0; i < size; i++) {
    my_int[i] = i;
    EXPECT_EQ(i, my_int[i]);
  }
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(*my_int, *your_int);
  }
}

TEST(SharedArray, ReadWrite2) {
  SharedArray<int, size> my_int("intTest2", O_RDWR, true, false);
  SharedArray<int, size> our_int("intTest2", O_RDWR, false, false);

  for (int i = 0; i < size; i++) {
    my_int[i] = i;
  }
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(*my_int, *our_int);
  }
  for (int i = 0; i < size; i++) {
    our_int[i] = i * 2;
  }
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(my_int[i], our_int[i]);
  }
}

