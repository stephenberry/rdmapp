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

size_t device_list::size() { return nr_devices_; }

struct ibv_device *device_list::at(size_t i) {
  if (i >= nr_devices_) {
    throw std::out_of_range("out of range");
  }
  return devices_[i];
}

device::device(struct ibv_device *target, uint16_t port_num) {
  assert(target != nullptr);
  open_device(target, port_num);
}

device::device(std::string const &device_name, uint16_t port_num)
    : device_(nullptr), port_num_(0) {
  auto devices = device_list();
  for (auto target : devices) {
    if (::ibv_get_device_name(target) == device_name) {
      open_device(target, port_num);
      return;
    }
  }
  throw_with("no device named %s found", device_name.c_str());
}

device::device(uint16_t device_num, uint16_t port_num)
    : device_(nullptr), port_num_(0) {
  auto devices = device_list();
  if (device_num >= devices.size()) {
    char buffer[kErrorStringBufferSize] = {0};
    ::snprintf(buffer, sizeof(buffer),
               "requested device number %d out of range, %lu devices available",
               device_num, devices.size());
    throw std::invalid_argument(buffer);
  }
  open_device(devices.at(device_num), port_num);
}

uint16_t device::port_num() const { return port_num_; }

uint16_t device::lid() const { return port_attr_.lid; }

bool device::is_compare_and_swap_supported() const {
  return device_attr_ex_.orig_attr.atomic_cap != IBV_ATOMIC_NONE;
}

bool device::is_fetch_and_add_supported() const {
  return device_attr_ex_.orig_attr.atomic_cap != IBV_ATOMIC_NONE;
}

} // namespace rdmapp
