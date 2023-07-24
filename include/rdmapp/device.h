#pragma once

#include <infiniband/verbs.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <memory>

#include "rdmapp/detail/debug.h"
#include "rdmapp/detail/noncopyable.h"
#include "rdmapp/error.h"

namespace rdmapp
{
   struct device_list_deleter {
      void operator()(ibv_device** ptr) const {
         if (ptr) {
            ::ibv_free_device_list(ptr);
         }
      }
   };

   // This class holds a list of devices available on the system.
   struct device_list final
   {
      std::span<ibv_device*> devices{};
      std::unique_ptr<ibv_device*, device_list_deleter> device_list_ptr{};

      device_list()
      {
         int32_t n_devices = -1;
         device_list_ptr.reset(::ibv_get_device_list(&n_devices));
         if (n_devices == 0) {
            throw std::runtime_error("no Infiniband devices found");
         }
         if (!device_list_ptr) {
            throw std::runtime_error("failed to get Infiniband devices");
         }
         devices = { device_list_ptr.get(), size_t(n_devices) };
      }
   };

   // This class is an abstraction of an Infiniband device.
   struct device : public noncopyable
   {
      ibv_device* device_{};
      uint16_t port_num{}; // port number
      ibv_context* ctx_{};
      ibv_port_attr port_attr_{};
      ibv_device_attr_ex device_attr_ex_{};

      void open_device(ibv_device* target, uint16_t port_num_in)
      {
         device_ = target;
         port_num = port_num_in;
         ctx_ = ::ibv_open_device(device_);
         check_ptr(ctx_, "failed to open device");
         check_rc(::ibv_query_port(ctx_, port_num, &port_attr_), "failed to query port");
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
       * @param device_name The name of the target device.
       * @param port_num The port number of the target device.
       */
      device(const std::string& device_name, uint16_t port_num = 1)
      {
         auto list = device_list();
         auto devices = list.devices;
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
      device(uint16_t device_num, uint16_t port_num = 1)
      {
         auto list = device_list();
         auto devices = list.devices;
         if (device_num >= devices.size()) {
            char buffer[kErrorStringBufferSize]{0};
            ::snprintf(buffer, sizeof(buffer), "requested device number %d out of range, %lu devices available",
                       device_num, devices.size());
            throw std::invalid_argument(buffer);
         }
         open_device(devices[device_num], port_num);
      }

      // Get the lid of the device.
      uint16_t lid() const { return port_attr_.lid; }

      bool is_fetch_and_add_supported() const { return device_attr_ex_.orig_attr.atomic_cap != IBV_ATOMIC_NONE; }

      bool is_compare_and_swap_supported() const { return device_attr_ex_.orig_attr.atomic_cap != IBV_ATOMIC_NONE; }

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