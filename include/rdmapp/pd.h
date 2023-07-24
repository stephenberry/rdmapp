#pragma once

#include <infiniband/verbs.h>

#include <cstdio>
#include <cstring>
#include <memory>

#include "rdmapp/detail/debug.h"
#include "rdmapp/detail/noncopyable.h"
#include "rdmapp/device.h"
#include "rdmapp/error.h"
#include "rdmapp/mr.h"

namespace rdmapp
{

   struct qp;

   /**
    * @brief This class is an abstraction of a Protection Domain.
    *
    */
   struct pd : public noncopyable, public std::enable_shared_from_this<pd>
   {
     private:
      std::shared_ptr<device> device_;
      struct ibv_pd* pd_;
      friend struct qp;
      friend class srq;

     public:
      pd(std::shared_ptr<rdmapp::device> device) : device_(device)
      {
         pd_ = ::ibv_alloc_pd(device->ctx_);
         check_ptr(pd_, "failed to alloc pd");
         RDMAPP_LOG_TRACE("alloc pd %p", reinterpret_cast<void*>(pd_));
      }

      /**
       * @brief Get the device object pointer.
       *
       * @return std::shared_ptr<device> The device object pointer.
       */
      std::shared_ptr<device> device_ptr() const { return device_; }

      /**
       * @brief Register a local memory region.
       *
       * @param addr The address of the memory region.
       * @param length The length of the memory region.
       * @param flags The access flags to use.
       * @return local_mr The local memory region handle.
       */
      local_mr reg_mr(void* buffer, size_t length,
                      int flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ |
                                  IBV_ACCESS_REMOTE_ATOMIC)
      {
         auto mr = ::ibv_reg_mr(pd_, buffer, length, flags);
         check_ptr(mr, "failed to reg mr");
         return rdmapp::local_mr(this->shared_from_this(), mr);
      }

      /**
       * @brief Destroy the pd object and the associated protection domain.
       *
       */
      ~pd()
      {
         if (pd_ == nullptr) [[unlikely]] {
            return;
         }
         if (auto rc = ::ibv_dealloc_pd(pd_); rc != 0) [[unlikely]] {
            RDMAPP_LOG_ERROR("failed to dealloc pd %p: %s", reinterpret_cast<void*>(pd_), strerror(errno));
         }
         else {
            RDMAPP_LOG_TRACE("dealloc pd %p", reinterpret_cast<void*>(pd_));
         }
      }
   };

} // namespace rdmapp