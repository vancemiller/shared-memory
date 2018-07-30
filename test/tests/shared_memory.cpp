#include "gtest/gtest.h"
#include "shared_memory.hpp"

#define ITERATIONS 6666

TEST(SharedMemory, ConstructDestruct) {
  SharedMemory<int> sharedInt(1, "test", O_RDWR, true);
}

TEST(SharedMemory, ReadWrite) {
  SharedMemory<int> my_int(1, "test", O_RDWR, true);
  int i = 0;
  for (*my_int = 0; *my_int < ITERATIONS; i++, (*my_int)++)
    EXPECT_EQ(*my_int, i);
}

TEST(SharedMemory, ReadOnly) {
  SharedMemory<int> my_int(1, "test", O_RDWR, true);
  SharedMemory<int> your_int(1, "test");
  *my_int = 666;
  EXPECT_EQ(*my_int, *your_int);
}

TEST(SharedMemory, ReadWrite2) {
  SharedMemory<int> my_int(1, "test", O_RDWR, true);
  SharedMemory<int> our_int(1, "test", O_RDWR);
  *my_int = 666;
  EXPECT_EQ(*my_int, *our_int);
  (*our_int)++;
  EXPECT_EQ(*my_int, *our_int);
  EXPECT_EQ(*my_int, 667);
}

TEST(SharedMemory, Remap) {
  for (int i = 0; i < ITERATIONS; i++) {
    SharedMemory<int> my_int(1, "test", O_RDWR, true);
    SharedMemory<int> your_int(1, "test", O_RDONLY);
    *my_int = i;
    EXPECT_EQ(*my_int, i);
    EXPECT_EQ(*my_int, *your_int);
  }
}

TEST(SharedMemory, Reattach) {
  SharedMemory<int>* my_int = new SharedMemory<int>(1, "test", O_RDWR, true);
  // leak memory so destructor isn't called

  SharedMemory<int> re_int(1, "test", O_RDWR, true, true);
  *re_int = 666;

  EXPECT_EQ(**my_int, *re_int);
}

const int size = 8;

TEST(SharedMemory, ConstructDestructArray) {
  SharedMemory<int> sharedArray(size, "test", O_RDWR, true);
}

TEST(SharedMemory, ReadWriteArray) {
  SharedMemory<int> my_int(size, "test", O_RDWR, true);
  for (int i = 0; i < size; i++) {
    my_int[i] = i;
    EXPECT_EQ(i, my_int[i]);
  }
}

TEST(SharedMemory, ReadOnlyArray) {
  SharedMemory<int> my_int(size, "test", O_RDWR, true);
  SharedMemory<int> your_int(size, "test");
  for (int i = 0; i < size; i++) {
    my_int[i] = i;
    EXPECT_EQ(i, my_int[i]);
  }
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(*my_int, *your_int);
  }
}

TEST(SharedMemory, ReadWrite2Array) {
  SharedMemory<int> my_int(size, "test", O_RDWR, true);
  SharedMemory<int> our_int(size, "test", O_RDWR);

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

