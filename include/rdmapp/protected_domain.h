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

   struct pd_deleter {
      void operator()(ibv_pd* pd_) const {
         if (pd_) [[likely]] {
            if (auto rc = ::ibv_dealloc_pd(pd_); rc != 0) [[unlikely]] {
               RDMAPP_LOG_ERROR("failed to dealloc pd %p: %s", reinterpret_cast<void*>(pd_), strerror(errno));
            }
            else {
               RDMAPP_LOG_TRACE("dealloc pd %p", reinterpret_cast<void*>(pd_));
            }
         }
      }
   };

   // This class is an abstraction of a Protection Domain.
   struct protected_domain : public noncopyable, public std::enable_shared_from_this<protected_domain>
   {
      std::shared_ptr<rdmapp::device> device{};
      std::unique_ptr<ibv_pd, pd_deleter> pd_{};

      protected_domain(std::shared_ptr<rdmapp::device> device) : device(device)
      {
         pd_.reset(::ibv_alloc_pd(device->ctx_));
         if (!pd_) {
            throw std::runtime_error("failed to alloc pd");
         }
         RDMAPP_LOG_TRACE("alloc pd %p", reinterpret_cast<void*>(pd_.get()));
      }

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
         auto mr = ::ibv_reg_mr(pd_.get(), buffer, length, flags);
         check_ptr(mr, "failed to reg mr");
         return local_mr(this->shared_from_this(), mr);
      }
   };

} // namespace rdmapp