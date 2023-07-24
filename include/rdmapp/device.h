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

      size_t size() { return nr_devices_; }
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
      void open_device(ibv_device* target, uint16_t port_num)
      {
         device_ = target;
         port_num_ = port_num;
         ctx_ = ::ibv_open_device(device_);
         check_ptr(ctx_, "failed to open device");
         check_rc(::ibv_query_port(ctx_, port_num_, &port_attr_), "failed to query port");
         struct ibv_query_device_ex_input query = {};
         check_rc(::ibv_query_device_ex(ctx_, &query, &device_attr_ex_), "failed to query extended attributes");

         auto link_layer = [&]() {
            switch (port_attr_.link_layer) {
            case IBV_LINK_LAYER_ETHERNET:
               return "ethernet";
            case IBV_LINK_LAYER_INFINIBAND:
               return "infiniband";
            }
            return "unspecified";
         }();
         RDMAPP_LOG_DEBUG("opened Infiniband device lid=%d link_layer=%s", port_attr_.lid, link_layer);
      }

      /**
       * @brief Construct a new device object.
       *
       * @param target The target device.
       * @param port_num The port number of the target device.
       */
      device(ibv_device* target, uint16_t port_num = 1)
      {
         assert(target != nullptr);
         open_device(target, port_num);
      }

      /**
       * @brief Construct a new device object.
       *
       * @param device_name The name of the target device.
       * @param port_num The port number of the target device.
       */
      device(const std::string& device_name, uint16_t port_num = 1) : device_(nullptr), port_num_(0)
      {
         auto devices = device_list();
         for (auto target : devices) {
            if (::ibv_get_device_name(target) == device_name) {
               open_device(target, port_num);
               return;
            }
         }
         throw_with("no device named %s found", device_name.c_str());
      }

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