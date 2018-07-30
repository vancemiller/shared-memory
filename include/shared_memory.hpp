#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <cerrno>
#include <cstring>
#include <fcntl.h>
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
    const size_t size;
    bool owner;
    int fd = -1;
  protected:
    T* memory_region = NULL;
  public:
    /**
      * Create a shared memory region of size sizeof(T) * count identified by name.
      * flags are POSIX shm (man shm_open) flags specifying permissions. By default this is
      * O_RDONLY. Writable regions require flags to be O_RDWR.
      * The owner of the shared memory region has permission to overwrite existing shared memory
      * regions with the same name.
      * Owners can reattach to an existing shared memory region without overwriting it by setting
      * reattach to true.
      */
    SharedMemory(size_t count, const std::string& name, int flags = O_RDONLY, bool owner = false,
        bool reattach = false) : name(std::string("/") + name), size(sizeof(T) * count),
        owner(owner) {
      mode_t mode = owner ? 0666 : 0;
      if (owner && !reattach) flags |= O_CREAT;
      int retry_count = 0;
      while ((fd = shm_open(this->name.c_str(), flags, mode)) == -1) {
        retry_count++;
        usleep(US_SLEEP_CONNECT);
        if (retry_count >= RETRY_COUNT)
          throw std::system_error(errno, std::generic_category(), "shm_open failed after " +
              std::to_string(RETRY_COUNT) + " retries");
      }
      if (owner && !reattach)
        if (ftruncate(fd, size) == -1)
          throw std::system_error(errno, std::generic_category(), "ftruncate failed");
      int prot = PROT_READ;
      if (flags & O_RDWR) prot |= PROT_WRITE;
      if ((memory_region = (T*) mmap(NULL, size, prot, MAP_SHARED, fd, 0)) == MAP_FAILED)
        throw std::system_error(errno, std::generic_category(), "mmap failed");
    }

    ~SharedMemory(void) {
      if (memory_region)
        if (munmap(memory_region, size) == -1)
          std::cerr << "munmap failed: " << std::strerror(errno) << std::endl;
      if (fd != -1) {
        if (close(fd) == -1)
          std::cerr << "close failed: " << std::strerror(errno) << std::endl;
        if (owner)
          if (shm_unlink(name.c_str()) == -1)
            std::cerr << "shm_unlink failed: " << std::strerror(errno) << std::endl;
      }
    }


    const T& operator*(void) const { return *memory_region; }
    T& operator*(void) { return *memory_region; }
    const T* operator->(void) const { return memory_region; }
    T* operator->(void) { return memory_region; }
    const T* operator&(void) const { return memory_region; }
    T* operator&(void) { return memory_region; }
    // TODO should we do bounds checking?
    const T& operator[](size_t i) const { return ((T*) this->memory_region)[i]; }
    T& operator[](size_t i) { return ((T*) this->memory_region)[i]; }
};

#endif

