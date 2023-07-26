#pragma once

#include <infiniband/verbs.h>

#include "rdmapp/device.h"

namespace rdmapp
{
   struct cq_deleter
   {
      void operator()(ibv_cq* cq) const
      {
         if (cq) {
            if (auto rc = ::ibv_destroy_cq(cq); rc != 0) {
               format_throw("failed to destroy cq {}: {}", reinterpret_cast<void*>(cq), std::strerror(errno));
            }
         }
      }
   };

   struct completion_queue
   {
      std::shared_ptr<rdmapp::device> device{}; // The device to use.
      size_t num_cqe{128}; // The number of completion entries to allocate.

      std::unique_ptr<ibv_cq, cq_deleter> cq{[&] {
         check_ptr(device, "device pointer null");
         ibv_cq* cq = ::ibv_create_cq(device->ctx, num_cqe, this, nullptr, 0);
         check_ptr(cq, "failed to create cq");
         return cq;
      }()};

      /**
       * @brief Poll the completion queue.
       *
       * @param wc If any, this will be filled with a completion entry.
       * @return true If there is a completion entry.
       * @return false If there is no completion entry.
       * @exception std::runtime_exception Error occured while polling the
       * completion queue.
       */
      bool poll(ibv_wc& wc)
      {
         auto rc = ::ibv_poll_cq(cq.get(), 1, &wc);
         if (rc < 0) [[unlikely]] {
            throw std::runtime_error("failed to poll cq");
         }
         return rc;
      }

      /**
       * @brief Poll the completion queue.
       *
       * @param wc_vec If any, this will be filled with completion entries up to the size of the vector.
       * @return int The number of completion entries. 0 means no completion entry.
       * @exception std::runtime_exception occured while polling the completion queue.
       */
      int poll(std::vector<ibv_wc>& wc_vec) { return poll(wc_vec.data(), wc_vec.size()); }

      int poll(ibv_wc* wc, int count)
      {
         int rc = ::ibv_poll_cq(cq.get(), count, wc);
         if (rc < 0) {
            format_throw("failed to poll cq: {} (rc={})", strerror(rc), rc);
         }
         return rc;
      }
   };
}