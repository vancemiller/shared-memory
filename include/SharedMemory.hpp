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
          if (owner) {
            new(memory_region->get_address()) T();
            // call T's default constructor and place the object in shared memory
            // using the new(place) constructor
          }
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
};

template <class T>
class LockedSharedMemory {
  private:
    SharedMemory<ProducerConsumerLocks> locks;
    SharedMemory<T> data;
  public:
    LockedSharedMemory(const std::string& name, boost::interprocess::mode_t type) :
        locks(name + "locks", read_write, type == read_write),
        data(name + "data", type, type == read_write) {
      if (type == read_only) {
        DEBUG << "LockedSharedMemory adding consumer " << &locks << std::endl;
        locks->add_consumer();
      }
    }
    ~LockedSharedMemory(void) {
      DEBUG << "LockedSharedMemory destructor" << std::endl;
      // TODO
    }

    void write(T& t) {
      DEBUG << "LockedSharedMemory accessed locks " << &locks << " at " << &*locks << std::endl;
      for (size_t i = 0; i < locks->n_consumers; i++)
        locks->data_processed.wait();
      locks->mutex.lock();
      *(data) = t; // copy
      DEBUG << "LockedSharedMemory wrote data " << &t << " to " << locks->n_consumers << " consumers." << std::endl;
      locks->mutex.unlock();
      for (size_t i = 0; i < locks->n_consumers; i++)
        locks->data_available.post();
    }

    T& read(void) {
      DEBUG << "LockedSharedMemory accessed locks " << &locks << " at " << &*locks << std::endl;
      locks->data_available.wait();
      locks->mutex.lock();
      return *data;
    }// must call done to release the locks on the item

    void done(void) {
      locks->mutex.unlock();
      locks->data_processed.post();
    }
};

#endif

