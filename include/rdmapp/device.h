#pragma once

#include <infiniband/verbs.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>

#include "rdmapp/detail/debug.h"
#include "rdmapp/detail/noncopyable.h"
#include "rdmapp/error.h"

namespace rdmapp
{
   // This class holds a list of devices available on the system.
   struct device_list : public noncopyable
   {
     private:
      struct ibv_device** devices_{};
      size_t nr_devices_{};

     public:
      class iterator : public std::iterator<std::forward_iterator_tag, struct ibv_device*>
      {
         friend struct device_list;
         size_t i_;
         struct ibv_device** devices_;
         iterator(struct ibv_device** devices, size_t i);

        public:
         struct ibv_device*& operator*();
         bool operator==(const device_list::iterator& other) const;
         bool operator!=(const device_list::iterator& other) const;
         device_list::iterator& operator++();
         device_list::iterator& operator++(int);
      };

      device_list()
      {
         int32_t nr_devices = -1;
         devices_ = ::ibv_get_device_list(&nr_devices);
         if (nr_devices == 0) {
            ::ibv_free_device_list(devices_);
            throw std::runtime_error("no Infiniband devices found");
         }
         check_ptr(devices_, "failed to get Infiniband devices");
         nr_devices_ = nr_devices;
      }

      size_t size();
      struct ibv_device* at(size_t i);
      iterator begin();
      iterator end();

      ~device_list()
      {
         if (devices_ != nullptr) {
            ::ibv_free_device_list(devices_);
         }
      }
   };

   // This class is an abstraction of an Infiniband device.
   struct device : public noncopyable
   {
      ibv_device* device_{};
      ibv_context* ctx_{};
      ibv_port_attr port_attr_{};
      ibv_device_attr_ex device_attr_ex_{};

      uint16_t port_num_;
      void open_device(ibv_device* target, uint16_t port_num);

      /**
       * @brief Construct a new device object.
       *
       * @param target The target device.
       * @param port_num The port number of the target device.
       */
      device(ibv_device* target, uint16_t port_num = 1);

      /**
       * @brief Construct a new device object.
       *
       * @param device_name The name of the target device.
       * @param port_num The port number of the target device.
       */
      device(const std::string& device_name, uint16_t port_num = 1);

      /**
       * @brief Construct a new device object.
       *
       * @param device_num The index of the target device.
       * @param port_num The port number of the target device.
       */
      device(uint16_t device_num = 0, uint16_t port_num = 1);

      /**
       * @brief Get the device port number.
       *
       * @return uint16_t The port number.
       */
      uint16_t port_num() const;

      /**
       * @brief Get the lid of the device.
       *
       * @return uint16_t The lid.
       */
      uint16_t lid() const;

      /**
       * @brief Checks if the device supports fetch and add.
       *
       * @return true Supported.
       * @return false Not supported.
       */
      bool is_fetch_and_add_supported() const;

      /**
       * @brief Checks if the device supports compare and swap.
       *
       * @return true Supported.
       * @return false Not supported.
       */
      bool is_compare_and_swap_supported() const;

      ~device()
      {
         if (ctx_) [[likely]] {
            if (auto rc = ::ibv_close_device(ctx_); rc != 0) [[unlikely]] {
               RDMAPP_LOG_ERROR("failed to close device lid=%d: %s", port_attr_.lid, strerror(rc));
            }
            else {
               RDMAPP_LOG_DEBUG("closed device lid=%d", port_attr_.lid);
            }
         }
      }
   };

} // namespace rdmapp