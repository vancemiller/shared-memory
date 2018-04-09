#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "Locks.hpp"
#include "Reader.hpp"

#define DEBUG if (1) std::cout << getpid() << " "

using namespace boost::interprocess;

static shared_memory_object* open_only_retry(const std::string& name,
    boost::interprocess::mode_t type) {
  int retry_count = 0;
  while (true) {
    try {
      DEBUG << "Open shared memory - Attempt " << ++retry_count << std::endl;
      return new shared_memory_object(open_only, name.c_str(), type);
    } catch (const boost::interprocess::interprocess_exception& e) {
      DEBUG << "Failed to connect to shared memory." << std::endl;
      std::this_thread::sleep_until(std::chrono::steady_clock::now() +
          std::chrono::milliseconds(100));
    }
  }
}

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
          shared_memory = open_only_retry(name, read_only);
          memory_region = new mapped_region(*shared_memory, read_only);
          break;
        case read_write:
          if (owner) {
            shared_memory = new shared_memory_object(create_only, name.c_str(), read_write);
            shared_memory->truncate(sizeof(T)); // set memory region size
          } else {
            shared_memory = open_only_retry(name, read_write);
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
class LockedSharedMemory {
  private:
    SharedMemory<ProducerConsumerLocks> locks;
    SharedMemory<T> data;
  public:
    LockedSharedMemory(const std::string& name, boost::interprocess::mode_t type) :
        locks(name + "locks", read_write, type == read_write),
        data(name + "data", type, type == read_write) {
      if (type == read_write) {
        new (&locks) ProducerConsumerLocks();
      }
    }

    ~LockedSharedMemory(void) {
      DEBUG << "LockedSharedMemory destructor" << std::endl;
      // TODO
    }

    void add_connection(bool synchronous, pid_t replacing) {
      locks->add_consumer(synchronous, replacing);
    }

    void remove_connection(void) {
      locks->remove_consumer();
    }

    T* get_data(void) {
      locks->data_processed_wait();
      return &data;
    }

    void release_data(void) {
      locks->data_available_post();
    }

    bool read(Reader<T>& reader) {
      return locks->data_read(reader, *data);
    }
};

#undef DEBUG
#endif

