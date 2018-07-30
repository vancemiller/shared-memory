#include "gtest/gtest.h"
#include "shared_memory.hpp"

#define ITERATIONS 6666

TEST(SharedMemory, CreateDestroyAnonymous) {
  SharedMemory<int> sharedInt;
}

TEST(SharedMemory, CreateDestroyNamed) {
  SharedMemory<int> sharedInt("intTest-1", O_RDWR, true, false);
}

TEST(SharedMemory, ReadWrite) {
  SharedMemory<int> my_int("intTest0", O_RDWR, true, false);
  int i = 0;
  for (*my_int = 0; *my_int < ITERATIONS; i++, (*my_int)++)
    EXPECT_EQ(*my_int, i);
}

TEST(SharedMemory, ReadOnly) {
  SharedMemory<int> my_int("intTest1", O_RDWR, true, false);
  SharedMemory<int> your_int("intTest1", O_RDONLY, false, false);
  *my_int = 666;
  EXPECT_EQ(*my_int, *your_int);
}

TEST(SharedMemory, ReadWrite2) {
  SharedMemory<int> my_int("intTest2", O_RDWR, true, false);
  SharedMemory<int> our_int("intTest2", O_RDWR, false, false);
  *my_int = 666;
  EXPECT_EQ(*my_int, *our_int);
  (*our_int)++;
  EXPECT_EQ(*my_int, *our_int);
  EXPECT_EQ(*my_int, 667);
}

TEST(SharedMemory, Remap) {
  for (int i = 0; i < ITERATIONS; i++) {
    SharedMemory<int> my_int("intTest3", O_RDWR, true, false);
    SharedMemory<int> your_int("intTest3", O_RDONLY, false, false);
    *my_int = i;
    EXPECT_EQ(*my_int, i);
    EXPECT_EQ(*my_int, *your_int);
  }
}

TEST(SharedMemory, Reattach) {
  SharedMemory<int>* my_int = new SharedMemory<int>("intTest4", O_RDWR, true, false);
  // leak memory so destructor isn't called

  SharedMemory<int> re_int("intTest4", O_RDWR, true, true);
  *re_int = 666;

  EXPECT_EQ(**my_int, *re_int);
}

