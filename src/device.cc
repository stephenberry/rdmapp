#include "rdmapp/device.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <infiniband/verbs.h>

#include "rdmapp/error.h"

#include "rdmapp/detail/debug.h"

namespace rdmapp {

device_list::iterator::iterator(struct ibv_device **devices, size_t i)
    : i_(i), devices_(devices) {}

struct ibv_device *&device_list::iterator::operator*() { return devices_[i_]; }

bool device_list::iterator::operator==(
    device_list::iterator const &other) const {
  return i_ == other.i_;
}

bool device_list::iterator::operator!=(
    device_list::iterator const &other) const {
  return i_ != other.i_;
}

device_list::iterator &device_list::iterator::operator++() {
  i_++;
  return *this;
}

device_list::iterator &device_list::iterator::operator++(int) {
  i_++;
  return *this;
}

device_list::iterator device_list::begin() { return iterator(devices_, 0); }

device_list::iterator device_list::end() {
  return iterator(devices_, nr_devices_);
}

struct ibv_device *device_list::at(size_t i) {
  if (i >= nr_devices_) {
    throw std::out_of_range("out of range");
  }
  return devices_[i];
}

} // namespace rdmapp
