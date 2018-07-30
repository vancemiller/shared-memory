#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define RETRY_COUNT 100
#define US_SLEEP_CONNECT 10000

template <class T>
class SharedMemory {
  public:
    const std::string name;
  private:
    bool owner;
    int fd = -1;
  protected:
    T* memory_region = NULL;
  public:
    SharedMemory(void) {}
    SharedMemory(const SharedMemory& m) { assert(false); }
    SharedMemory(const std::string& name, int flags, bool owner, bool replacing) :
        name(std::string("/") + name), owner(owner) {
      mode_t mode = owner ? 0666 : 0;
      if (owner && !replacing) flags |= O_CREAT;
      int count = 0;
      while ((fd = shm_open(this->name.c_str(), flags, mode)) == -1) {
        count++;
        usleep(US_SLEEP_CONNECT);
        if (count >= RETRY_COUNT)
          throw std::runtime_error(std::string("Failed to open shared memory: ") +
              std::strerror(errno));
      }
      if (owner && !replacing)
        if (ftruncate(fd, sizeof(T)) == -1)
          throw std::runtime_error(std::string("Failed to resize shared memory: ") +
              std::strerror(errno));
      int prot = PROT_READ;
      if (flags & O_RDWR) prot |= PROT_WRITE;
      if ((memory_region = (T*) mmap(NULL, sizeof(T), prot, MAP_SHARED, fd, 0)) == MAP_FAILED)
        throw std::runtime_error(std::string("Failed to map shared memory: ") +
            std::strerror(errno));
    }

    ~SharedMemory(void) {
      if (memory_region)
        if (munmap(memory_region, sizeof(T)) == -1)
          std::cerr << "Failed to unmap shared memory: " << std::strerror(errno) << std::endl;
      if (fd != -1) {
        if (close(fd) == -1)
          std::cerr << "Failed to close shared memory: " << std::strerror(errno) << std::endl;
        if (owner)
          if (shm_unlink(name.c_str()) == -1)
            std::cerr << "Failed to unlink shared memory: " << std::strerror(errno) << std::endl;
      }
    }

    const T& operator*(void) const { return *memory_region; }
    T& operator*(void) { return *memory_region; }
    const T* operator->(void) const { return memory_region; }
    T* operator->(void) { return memory_region; }
    const T* operator&(void) const { return memory_region; }
    T* operator&(void) { return memory_region; }
};

#endif

