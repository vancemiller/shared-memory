#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <thread>

#include "Debug.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

#define MS_SLEEP_CONNECT 100
using namespace boost::interprocess;

template <class T>
class SharedMemory {
  private:
    const std::string name;
    bool owner;
    shared_memory_object* shared_memory;
    mapped_region* memory_region;
  private:
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
              std::chrono::milliseconds(MS_SLEEP_CONNECT));
        }
      }
    }
  public:
    SharedMemory(void) {}

    SharedMemory(const std::string& name, boost::interprocess::mode_t type, bool owner,
        const pid_t replacing) : name(
#ifdef UNIQUE_NAME
          std::string(UNIQUE_NAME) + name
#else
          name
#endif
          ), owner(owner) {
      DEBUG << "SharedMemory region " << this->name << " at address " << this <<
          " created with type " << type << " owned by me " << owner << std::endl;
      if (owner && !replacing) {
        shared_memory = new shared_memory_object(open_or_create, name.c_str(), type);
        shared_memory->truncate(sizeof(T)); // set memory region size
      } else {
        shared_memory = open_only_retry(name, type);
      }
      memory_region = new mapped_region(*shared_memory, type);
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

#endif

