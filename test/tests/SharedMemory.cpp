#include "gtest/gtest.h"
#include "SharedMemory.hpp"

#define ITERATIONS 6666

TEST(SharedMemory, CreateDestroy) {
  SharedMemory<int> sharedInt;
}

TEST(SharedMemory, ReadWrite) {
  SharedMemory<int> my_int("/intTest0", O_RDWR, true, 0);
  int i = 0;
  for (*my_int = 0; *my_int < ITERATIONS; i++, (*my_int)++)
    EXPECT_EQ(*my_int, i);
}

TEST(SharedMemory, ReadOnly) {
  SharedMemory<int> my_int("/intTest1", O_RDWR, true, 0);
  SharedMemory<int> your_int("/intTest1", O_RDONLY, false, 0);
  *my_int = 666;
  EXPECT_EQ(*my_int, *your_int);
}

TEST(SharedMemory, ReadWrite2) {
  SharedMemory<int> my_int("/intTest2", O_RDWR, true, 0);
  SharedMemory<int> our_int("/intTest2", O_RDWR, false, 0);
  *my_int = 666;
  EXPECT_EQ(*my_int, *our_int);
  (*our_int)++;
  EXPECT_EQ(*my_int, *our_int);
  EXPECT_EQ(*my_int, 667);
}

TEST(SharedMemory, Remap) {
  for (int i = 0; i < ITERATIONS; i++) {
    SharedMemory<int> my_int("/intTest3", O_RDWR, true, 0);
    SharedMemory<int> your_int("/intTest3", O_RDONLY, false, 0);
    *my_int = i;
    EXPECT_EQ(*my_int, i);
    EXPECT_EQ(*my_int, *your_int);
  }
}

