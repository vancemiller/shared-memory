#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Debug.hpp"

#define RETRY_COUNT 100
#define US_SLEEP_CONNECT 10000

template <class T>
class SharedMemory {
  private:
    const std::string name;
    bool owner;
    int fd = -1;
    T* memory_region = NULL;
  public:
    SharedMemory(void) {}

    SharedMemory(const std::string& name, int flags, bool owner, const pid_t replacing) : name(
#ifdef UNIQUE_NAME
          std::string(UNIQUE_NAME) + name
#else
          name
#endif
          ), owner(owner) {

      mode_t mode = owner ? 0666 : 0;
      if (owner && !replacing) flags |= O_CREAT;
      int count;
      DEBUG << this->name << " " << flags << " " << mode << " " << replacing << std::endl;
      while ((fd = shm_open(this->name.c_str(), flags, mode)) == -1) {
        DEBUG << "Open shared memory failure: " << std::strerror(errno) << std::endl;
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
      DEBUG << "SharedMemory destructor" << std::endl;
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

    T& operator*() {
      return *memory_region;
    }

    T* operator->() {
      return memory_region;
    }

    T* operator&() {
      return memory_region;
    }
};

#endif

