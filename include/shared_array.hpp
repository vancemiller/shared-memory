#ifndef SHARED_ARRAY_HPP
#define SHARED_ARRAY_HPP

#include <array>
#include <exception>

#include "shared_memory.hpp"

template <class T, size_t N_ENTRIES>
class SharedArray : public SharedMemory<std::array<T, N_ENTRIES>> {
  public:
    SharedArray(const std::string& name, int flags, bool owner, bool replacing) :
      SharedMemory<std::array<T, N_ENTRIES>>(name, flags, owner, replacing) {}
    T& operator[](size_t i) { return (*this->memory_region)[i]; }
    const T& operator[](size_t i) const { return (*this->memory_region)[i]; }
};

#endif
