#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include "file_descriptor.hpp"

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

namespace wrapper {

template <class T>
class SharedMemory {
public:
    std::string name;
    size_t nmemb;

private:
    int flags;
    FileDescriptor fd;

protected:
    void *memory_region = NULL;

public:
    /**
     * @brief Create a shared memory region
     * @param name the identifier the memory region
     * @param nmemb the number of entries in the shared memory region (if it is
     * an array)
     * @param flags are POSIX shm (man shm_open) flags specifying permissions
     * (O_RDONLY by default). Writable regions should set flags to O_RDWR. To
     * create a new shared memory region, OR O_CREAT in to flags. To make sure a
     * newly created memory region doesn't overwrite an existing one, OR O_EXCL
     * in to flags.
     */
    SharedMemory(const char *name, size_t nmemb, int flags = O_RDONLY)
        : name(std::string() + "/shared-memory-" + name), nmemb(nmemb),
          flags(flags), fd([this](void) -> int {
              mode_t mode = (this->flags & O_CREAT) ? 0666 : 0;
              return shm_open(this->name.c_str(), this->flags, mode);
          }()) {
        if (fd == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    std::string("shm_open ") + this->name +
                                        " failed");
        }
        if (flags & O_CREAT) {
            if (ftruncate(fd.get(), nmemb * sizeof(T)) == -1) {
                throw std::system_error(errno, std::generic_category(),
                                        "ftruncate failed");
            }
        }
        int prot = PROT_READ;
        if (flags & O_RDWR) {
            prot |= PROT_WRITE;
        }
        if ((memory_region = mmap(NULL, nmemb * sizeof(T), prot,
                                       MAP_SHARED, fd.get(), 0)) ==
            MAP_FAILED) {
            throw std::system_error(errno, std::generic_category(),
                                    "mmap failed");
        }
    }

    SharedMemory(const char *name, int flags = O_RDONLY)
        : SharedMemory(name, 1, flags) {}
    SharedMemory() : nmemb(0), fd(-1) {}
    ~SharedMemory() {
        if (memory_region) {
            if (munmap(memory_region, nmemb * sizeof(T)) == -1) {
                std::cerr << "munmap failed: " << std::strerror(errno)
                          << std::endl;
            }
            if (flags & O_CREAT) {
                if (shm_unlink(name.c_str()) == -1) {
                    std::cerr << "shm_unlink failed: " << std::strerror(errno)
                              << std::endl;
                }
            }
        }
    }

    // Delete copy constructors
    SharedMemory(SharedMemory &o) = delete;
    SharedMemory(const SharedMemory &o) = delete;
    // Define move constructor
    SharedMemory(SharedMemory &&o)
        : name(o.name), nmemb(o.nmemb), flags(o.flags), fd(std::move(o.fd)),
          memory_region(o.memory_region) {
        o.memory_region = NULL;
    }
    SharedMemory &operator=(SharedMemory &&o) {
        this->name = o.name;
        this->nmemb = o.nmemb;
        this->flags = o.flags;
        this->fd = std::move(o.fd);
        this->memory_region = o.memory_region;
        o.memory_region = NULL;
        return *this;
    }

    const T &operator*(void) const {
        if (!memory_region) {
            throw std::runtime_error("nullptr");
        }
        return *reinterpret_cast<T*>(memory_region);
    }
    const T *operator&(void) const { return reinterpret_cast<T*>(memory_region); }
    const T *operator->(void) const { return reinterpret_cast<T*>(memory_region); }
    const T &operator[](size_t i) const {
        if (!memory_region) {
            throw std::runtime_error("nullptr");
        }
        if (i > nmemb) {
            throw std::out_of_range(std::to_string(i));
        }
        return reinterpret_cast<T*>(this->memory_region)[i];
    }
    T &operator*(void) {
        if (!memory_region) {
            throw std::runtime_error("nullptr");
        }
        return *reinterpret_cast<T*>(memory_region);
    }
    T *operator&(void) { return reinterpret_cast<T*>(memory_region); }
    T *operator->(void) { return reinterpret_cast<T*>(memory_region); }
    T &operator[](size_t i) {
        if (!memory_region) {
            throw std::runtime_error("nullptr");
        }
        if (i > nmemb) {
            throw std::out_of_range(std::to_string(i));
        }
        return reinterpret_cast<T*>(this->memory_region)[i];
    }
};

}  // namespace wrapper
#endif
