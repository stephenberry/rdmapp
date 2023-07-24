#pragma once

#include <infiniband/verbs.h>

#include <memory>

#include "rdmapp/detail/debug.h"
#include "rdmapp/pd.h"

namespace rdmapp
{
   struct srq_deleter {
      void operator()(ibv_srq* srq_) const {
         if (srq_) [[likely]] {
            if (auto rc = ::ibv_destroy_srq(srq_); rc != 0) [[unlikely]] {
               RDMAPP_LOG_ERROR("failed to destroy srq %p: %s (rc=%d)", reinterpret_cast<void*>(srq_), strerror(rc),
                                rc);
            }
            else {
               RDMAPP_LOG_DEBUG("destroyed srq %p", reinterpret_cast<void*>(srq_));
            }
         }
      }
   };

   // This class represents a Shared Receive Queue.
   struct srq
   {
      std::shared_ptr<pd> pd_{};
      std::unique_ptr<ibv_srq> srq_{};

      /**
       * @brief Construct a new srq object
       *
       * @param pd The protection domain to use.
       * @param max_wr The maximum number of outstanding work requests.
       */
      srq(std::shared_ptr<pd> pd, uint32_t max_wr = 1024) : pd_(pd)
      {
         ibv_srq_init_attr srq_init_attr{};
         srq_init_attr.srq_context = this;
         srq_init_attr.attr.max_sge = 1;
         srq_init_attr.attr.max_wr = max_wr;
         srq_init_attr.attr.srq_limit = max_wr;

         srq_.reset(::ibv_create_srq(pd_->pd_.get(), &srq_init_attr));
         if (!srq_) {
            throw std::runtime_error("failed to create srq");
         }
         RDMAPP_LOG_DEBUG("created srq %p", reinterpret_cast<void*>(srq_.get()));
      }
   };

} // namespace rdmapp