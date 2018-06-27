#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "Debug.hpp"

#define US_SLEEP_CONNECT 100000

template <class T>
class SharedMemory {
  private:
    const std::string name;
    bool owner;
    int fd;
    T* memory_region;
  public:
    SharedMemory(void) {}

    SharedMemory(const std::string& name, bool owner, const pid_t replacing) : name(
#ifdef UNIQUE_NAME
          std::string(UNIQUE_NAME) + name
#else
          name
#endif
          ), owner(owner) {

      mode_t mode = owner ? 0666 : 0;
      int flags = owner ? O_RDWR : O_RDONLY;
      if (owner && !replacing) flags |= O_CREAT | O_EXCL;
      while ((fd = shm_open(this->name.c_str(), flags, mode)) == -1) {
        DEBUG << "Open shared memory failure: " << std::strerror(errno) << std::endl;
        usleep(US_SLEEP_CONNECT);
      }
      if (owner && !replacing)
        if (ftruncate(fd, sizeof(T)) == -1)
          throw std::runtime_error("Failed to resize shared memory: " + std::strerror(errno));
      int prot = owner ? PROT_READ | PROT_WRITE : PROT_READ;
      if ((memory_region = mmap(NULL, sizeof(T), prot, MAP_SHARED)) == MAP_FAILED)
        throw std::runtime_error("Failed to map shared memory: " + std::strerrror(errno));
    }

    ~SharedMemory(void) {
      DEBUG << "SharedMemory destructor" << std::endl;
      if (munmap(memory_region, sizeof(T)) == -1)
        std::cerr << "Failed to unmap shared memory: " << std::strerror(errno) << std::endl;
      if (shm_unlink(name.c_str()) == -1)
        std::cerr << "Failed to unlink shared memory: " << std::strerror(errno) << std::endl;
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

