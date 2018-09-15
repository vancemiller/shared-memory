#include "gtest/gtest.h"
#include "shared_memory.hpp"

#define ITERATIONS 6666
namespace wrapper {

TEST(SharedMemory, ConstructDestruct) {
  SharedMemory<int> sharedInt("test", sizeof(int), O_RDWR | O_CREAT | O_EXCL);
}

TEST(SharedMemory, ReadWrite) {
  SharedMemory<int> my_int("test", sizeof(int), O_RDWR | O_CREAT | O_EXCL);
  int i = 0;
  for (*my_int = 0; *my_int < ITERATIONS; i++, (*my_int)++)
    EXPECT_EQ(*my_int, i);
}

TEST(SharedMemory, ReadOnly) {
  SharedMemory<int> my_int("test", sizeof(int), O_RDWR | O_CREAT | O_EXCL);
  SharedMemory<int> your_int("test", sizeof(int));
  *my_int = 666;
  EXPECT_EQ(*my_int, *your_int);
}

TEST(SharedMemory, ReadWrite2) {
  SharedMemory<int> my_int("test", sizeof(int), O_RDWR | O_CREAT | O_EXCL);
  SharedMemory<int> our_int("test", sizeof(int), O_RDWR);
  *my_int = 666;
  EXPECT_EQ(*my_int, *our_int);
  (*our_int)++;
  EXPECT_EQ(*my_int, *our_int);
  EXPECT_EQ(*my_int, 667);
}

TEST(SharedMemory, Remap) {
  for (int i = 0; i < ITERATIONS; i++) {
    SharedMemory<int> my_int("test", sizeof(int), O_RDWR | O_CREAT | O_EXCL);
    SharedMemory<int> your_int("test", sizeof(int), O_RDONLY);
    *my_int = i;
    EXPECT_EQ(*my_int, i);
    EXPECT_EQ(*my_int, *your_int);
  }
}

TEST(SharedMemory, TransferOwnership) {
  std::unique_ptr<SharedMemory<int>> my_int(std::make_unique<SharedMemory<int>>("test",
      sizeof(int), O_RDWR | O_CREAT | O_EXCL));
  **my_int = 666;
  {
    SharedMemory<int> your_int("test", sizeof(int));
    my_int.reset();
    EXPECT_EQ(666, *your_int);
  }
  EXPECT_THROW(SharedMemory<int> your_int("test", sizeof(int)), std::system_error);
}

const int nmemb = 8;

TEST(SharedMemory, ConstructDestructArray) {
  SharedMemory<int> sharedArray("test", nmemb, sizeof(int), O_RDWR | O_CREAT | O_EXCL);
}

TEST(SharedMemory, ReadWriteArray) {
  SharedMemory<int> my_int("test", nmemb, sizeof(int), O_RDWR | O_CREAT | O_EXCL);
  for (int i = 0; i < nmemb; i++) {
    my_int[i] = i;
    EXPECT_EQ(i, my_int[i]);
  }
}

TEST(SharedMemory, ReadOnlyArray) {
  SharedMemory<int> my_int("test", nmemb, sizeof(int), O_RDWR | O_CREAT | O_EXCL);
  SharedMemory<int> your_int("test", nmemb, sizeof(int));
  for (int i = 0; i < nmemb; i++) {
    my_int[i] = i;
    EXPECT_EQ(i, my_int[i]);
  }
  for (int i = 0; i < nmemb; i++) {
    EXPECT_EQ(*my_int, *your_int);
  }
}

TEST(SharedMemory, ReadWrite2Array) {
  SharedMemory<int> my_int("test", nmemb, sizeof(int), O_RDWR | O_CREAT | O_EXCL);
  SharedMemory<int> our_int("test", nmemb, sizeof(int), O_RDWR);

  for (int i = 0; i < nmemb; i++) {
    my_int[i] = i;
  }
  for (int i = 0; i < nmemb; i++) {
    EXPECT_EQ(*my_int, *our_int);
  }
  for (int i = 0; i < nmemb; i++) {
    our_int[i] = i * 2;
  }
  for (int i = 0; i < nmemb; i++) {
    EXPECT_EQ(my_int[i], our_int[i]);
  }
}

TEST(SharedMemory, NotBigEnough) {
  EXPECT_THROW(new SharedMemory<int>("test", 1, (size_t) 1, O_RDWR | O_CREAT | O_EXCL),
      std::runtime_error);
}

TEST(SharedMemory, MoveConstructor) {
  SharedMemory<int> m("test", 1, sizeof(int), O_RDWR | O_CREAT | O_EXCL);
  *m = 12345;
  SharedMemory<int> m2(std::move(m));
  EXPECT_EQ(12345, *m2);
  EXPECT_THROW(*m, std::runtime_error);
}

class DestructSetsInt {
  int& to_set;
  int value;
  public:
    DestructSetsInt(int& to_set, int value) : to_set(to_set), value(value) {}
    ~DestructSetsInt(void) { to_set = value; }
};

TEST(SharedMemory, DestructDoesNotAffectContents) {
  int i = 0;
  {
    SharedMemory<DestructSetsInt> shared("test", sizeof(DestructSetsInt), O_RDWR | O_CREAT |
        O_EXCL);
    new (&shared) DestructSetsInt(i, 2);
    EXPECT_EQ(0, i);
  }
  EXPECT_EQ(0, i);
}

TEST(SharedMemory, ExplicitDestructDoesDestruct) {
  int i = 0;
  {
    SharedMemory<DestructSetsInt> shared("test", sizeof(DestructSetsInt), O_RDWR | O_CREAT |
        O_EXCL);
    new (&shared) DestructSetsInt(i, 2);
    EXPECT_EQ(0, i);
    shared->~DestructSetsInt();
  }
  EXPECT_EQ(2, i);
}

TEST(SharedMemory, NotExist) {
  EXPECT_THROW(SharedMemory<int> m("test", 1, sizeof(int), O_RDWR), std::runtime_error);
  EXPECT_THROW(SharedMemory<int> m("test", 1, sizeof(int)), std::runtime_error);
}
} // namespace wrapper
