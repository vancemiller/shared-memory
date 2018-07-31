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
    bool create;
    int fd;
  protected:
    T* memory_region = NULL;
  public:
    /**
      * Create a shared memory region of size nmemb * size identified by name.
      * flags are POSIX shm (man shm_open) flags specifying permissions (O_RDONLY by default).
      * Writable regions should set flags to O_RDWR.
      * If create is true, the constructor will overwrite existing shared memory regions with the
      * same name.
      */
    SharedMemory(const std::string& name, size_t nmemb, size_t size, int flags = O_RDONLY,
        bool create = false) : name(std::string("/") + name), nmemb(nmemb), size(size),
        create(create), fd(-1) {
      if (sizeof(T) > size)
        throw std::runtime_error("size must be at least " + std::to_string(sizeof(T)));
      mode_t mode = create ? 0666 : 0;
      if (create) flags |= O_CREAT;
      int retry_count = 0;
      while ((fd = shm_open(this->name.c_str(), flags, mode)) == -1) {
        retry_count++;
        usleep(US_SLEEP_CONNECT);
        if (retry_count >= RETRY_COUNT)
          throw std::system_error(errno, std::generic_category(), "shm_open failed");
      }
      if (create)
        if (ftruncate(fd, nmemb * size) == -1)
          throw std::system_error(errno, std::generic_category(), "ftruncate failed");
      int prot = PROT_READ;
      if (flags & O_RDWR) prot |= PROT_WRITE;
      if ((memory_region = (T*) mmap(NULL, nmemb * size, prot, MAP_SHARED, fd, 0)) == MAP_FAILED)
        throw std::system_error(errno, std::generic_category(), "mmap failed");
   }

    SharedMemory(const std::string& name, size_t size, int flags = O_RDONLY, bool create = false) :
        SharedMemory(name, 1, size, flags, create) {}

    ~SharedMemory(void) {
      if (memory_region)
        if (munmap(memory_region, nmemb * size) == -1)
          std::cerr << "munmap failed: " << std::strerror(errno) << std::endl;
      if (fd != -1) {
        if (close(fd) == -1)
          std::cerr << "close failed: " << std::strerror(errno) << std::endl;
        if (create)
          if (shm_unlink(name.c_str()) == -1)
            std::cerr << "shm_unlink failed: " << std::strerror(errno) << std::endl;
      }
    }

    const T& operator*(void) const { return *memory_region; }
    const T* operator&(void) const { return memory_region; }
    const T* operator->(void) const { return memory_region; }
    const T& operator[](size_t i) const {
      if (i > nmemb) throw std::out_of_range(std::to_string(i));
      return ((T*) this->memory_region)[i];
    }
    T& operator*(void) { return *memory_region; }
    T* operator&(void) { return memory_region; }
    T* operator->(void) { return memory_region; }
    T& operator[](size_t i) {
      if (i > nmemb) throw std::out_of_range(std::to_string(i));
      return ((T*) this->memory_region)[i];
    }
};

#endif

