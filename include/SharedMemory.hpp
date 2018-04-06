#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "Locks.hpp"

#define DEBUG if (0) std::cout << getpid() << " "

using namespace boost::interprocess;

template <class T>
class SharedMemory {
  private:
    const std::string name;
    bool owner;
    shared_memory_object* shared_memory;
    mapped_region* memory_region;
  public:
  public:
    SharedMemory(const std::string& name, boost::interprocess::mode_t type, bool owner) :
        name(name), owner(owner) {
      DEBUG << "SharedMemory region " << name << " at address " << this <<
          " created with type " << type << " owned by me " << owner << std::endl;
      switch (type) {
        case read_only:
          {
            int retry_count = 0;
            while (true) {
              try {
                shared_memory = new shared_memory_object(open_only, name.c_str(), read_only);
              } catch (const boost::interprocess::interprocess_exception& e) {
                DEBUG << "Open shared memory - Retry count " << retry_count++ << std::endl;
                continue;
              }
              break;
            }
            memory_region = new mapped_region(*shared_memory, read_only);
            break;
          }
        case read_write:
          if (owner) {
            shared_memory = new shared_memory_object(create_only, name.c_str(), read_write);
            shared_memory->truncate(sizeof(T)); // set memory region size
          } else {
            int retry_count = 0;
            while (true) {
              try {
                shared_memory = new shared_memory_object(open_only, name.c_str(), read_write);
              } catch (const boost::interprocess::interprocess_exception& e) {
                DEBUG << "Open shared memory - Retry count " << retry_count++ << std::endl;
                continue;
              }
              break;
            }
          }
          memory_region = new mapped_region(*shared_memory, read_write);
          break;
        default:
          // TODO error
          break;
      }
    }

    ~SharedMemory(void) {
      DEBUG << "SharedMemory destructor" << std::endl;
      if (owner)
        shared_memory_object::remove(name.c_str());
      delete memory_region;
      delete shared_memory;
    }

    T& operator*() {
      return *static_cast<T*>(memory_region->get_address());
    }

    T* operator->() {
      return static_cast<T*>(memory_region->get_address());
    }

    T* operator&() {
      return static_cast<T*>(memory_region->get_address());
    }
};

template <class T>
class Reader {
  public:
    virtual bool read(T& data) = 0;
};

template <class T>
class LockedSharedMemory {
  private:
    interprocess_condition no_data;
    SharedMemory<ProducerConsumerLocks> locks;
    SharedMemory<T> data;
  public:
    LockedSharedMemory(const std::string& name, boost::interprocess::mode_t type) :
        locks(name + "locks", read_write, type == read_write),
        data(name + "data", type, type == read_write) {
      new (&locks) ProducerConsumerLocks();
    }

    ~LockedSharedMemory(void) {
      DEBUG << "LockedSharedMemory destructor" << std::endl;
      // TODO
    }

    template <class ... Args>
    void construct_data(Args&& ... args) {
      scoped_lock<interprocess_mutex> lock(locks->mutex);
      new (&data) T(std::forward<Args...>(args...));
    }

    T* get_data(void) {
      locks->mutex.lock();
      return &data;
    }

    void release_data(void) {
      locks->mutex.unlock();
    }

    /*void write(Writer& writer) {
      scoped_lock<interprocess_mutex> lock(locks->mutex);
      writer.write();
      //void* ret = memcpy(&data, &t, sizeof(T));
      //assert(ret == &data);
    }*/

    bool read(Reader<T>& reader) {
      scoped_lock<interprocess_mutex> lock(locks->mutex);
      return reader.read(*data);
    }

};

#endif

