#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <system_error>
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
    const size_t nmemb;
    const size_t size;
    int flags;
    int fd;
  protected:
    T* memory_region = NULL;
  public:
    /**
      * Create a shared memory region of size nmemb * size identified by name.
      * flags are POSIX shm (man shm_open) flags specifying permissions (O_RDONLY by default).
      * Writable regions should set flags to O_RDWR.
      * To create a new shared memory region, OR O_CREAT in to flags.
      * To make sure a newly created memory region doesn't overwrite an existing one, OR O_EXCL
      * in to flags.
      */
    SharedMemory(const std::string& name, size_t nmemb, size_t size, int flags = O_RDONLY) :
        name(std::string("/") + name), nmemb(nmemb), size(size), flags(flags), fd(-1) {
      if (sizeof(T) > size)
        throw std::runtime_error("size must be at least " + std::to_string(sizeof(T)));
      mode_t mode = (flags & O_CREAT) ? 0666 : 0;
      int retry_count = 0;
      while ((fd = shm_open(this->name.c_str(), flags, mode)) == -1) {
        retry_count++;
        usleep(US_SLEEP_CONNECT);
        if (retry_count >= RETRY_COUNT)
          throw std::system_error(errno, std::generic_category(), "shm_open failed");
      }
      if (flags & O_CREAT)
        if (ftruncate(fd, nmemb * size) == -1)
          throw std::system_error(errno, std::generic_category(), "ftruncate failed");
      int prot = PROT_READ;
      if (flags & O_RDWR) prot |= PROT_WRITE;
      if ((memory_region = (T*) mmap(NULL, nmemb * size, prot, MAP_SHARED, fd, 0)) == MAP_FAILED)
        throw std::system_error(errno, std::generic_category(), "mmap failed");
   }

    SharedMemory(const std::string& name, size_t size, int flags = O_RDONLY) :
        SharedMemory(name, 1, size, flags) {}

    ~SharedMemory(void) {
      if (memory_region)
        if (munmap(memory_region, nmemb * size) == -1)
          std::cerr << "munmap failed: " << std::strerror(errno) << std::endl;
      if (fd != -1) {
        if (close(fd) == -1)
          std::cerr << "close failed: " << std::strerror(errno) << std::endl;
        if (flags & O_CREAT)
          if (shm_unlink(name.c_str()) == -1)
            std::cerr << "shm_unlink failed: " << std::strerror(errno) << std::endl;
      }
    }

    // Delete copy constructors
    SharedMemory(SharedMemory& o) = delete;
    SharedMemory(const SharedMemory& o) = delete;
    // Define move constructor
    SharedMemory(SharedMemory&& o) : name(o.name), nmemb(o.nmemb), size(o.size), flags(o.flags),
        fd(o.fd), memory_region(o.memory_region) {
      o.flags = O_RDONLY;
      o.memory_region = NULL;
      o.fd = -1;
    }

    const T& operator*(void) const {
      if (!memory_region) throw std::runtime_error("nullptr");
      return *memory_region;
    }
    const T* operator&(void) const {
      return memory_region;
    }
    const T* operator->(void) const {
      return memory_region;
    }
    const T& operator[](size_t i) const {
      if (!memory_region) throw std::runtime_error("nullptr");
      if (i > nmemb) throw std::out_of_range(std::to_string(i));
      return ((T*) this->memory_region)[i];
    }
    T& operator*(void) {
      if (!memory_region) throw std::runtime_error("nullptr");
      return *memory_region;
    }
    T* operator&(void) {
      return memory_region;
    }
    T* operator->(void) {
      return memory_region;
    }
    T& operator[](size_t i) {
      if (!memory_region) throw std::runtime_error("nullptr");
      if (i > nmemb) throw std::out_of_range(std::to_string(i));
      return ((T*) this->memory_region)[i];
    }
};

#endif
