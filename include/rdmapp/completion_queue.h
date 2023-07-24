#pragma once

#include <infiniband/verbs.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

#include "rdmapp/detail/debug.h"
#include "rdmapp/detail/noncopyable.h"
#include "rdmapp/device.h"
#include "rdmapp/error.h"

namespace rdmapp
{
   struct qp;

   struct cq_deleter
   {
      void operator()(ibv_cq* cq) const
      {
         if (cq) [[likely]] {
            if (auto rc = ::ibv_destroy_cq(cq); rc != 0) [[unlikely]] {
               RDMAPP_LOG_ERROR("failed to destroy cq %p: %s", reinterpret_cast<void*>(cq), strerror(errno));
            }
            else {
               RDMAPP_LOG_TRACE("destroyed cq: %p", reinterpret_cast<void*>(cq));
            }
         }
      }
   };

   // This class is an abstraction of a Completion Queue.
   struct completion_queue : public noncopyable
   {
      inline ibv_cq* make_cq(std::shared_ptr<device> device, size_t num_cqe = 128)
      {
         ibv_cq* cq = ::ibv_create_cq(device->ctx_, num_cqe, this, nullptr, 0);
         check_ptr(cq, "failed to create cq");
         RDMAPP_LOG_TRACE("created cq: %p", reinterpret_cast<void*>(cq));
         return cq;
      }

     private:
      std::shared_ptr<rdmapp::device> device_{};
      size_t num_cqe{128};
      std::unique_ptr<ibv_cq, cq_deleter> cq_{make_cq(device_, num_cqe)};
      friend struct qp;

     public:
      /**
       * @brief Construct a new cq object.
       *
       * @param device The device to use.
       * @param num_cqe The number of completion entries to allocate.
       */
      completion_queue(std::shared_ptr<device> device, size_t num_cqe = 128) : device_(device), num_cqe(num_cqe) {}

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
         if (auto rc = ::ibv_poll_cq(cq_.get(), 1, &wc); rc < 0) [[unlikely]] {
            check_rc(-rc, "failed to poll cq");
         }
         else if (rc == 0) {
            return false;
         }
         else {
            return true;
         }
         return false;
      }

      /**
       * @brief Poll the completion queue.
       *
       * @param wc_vec If any, this will be filled with completion entries up to the
       * size of the vector.
       * @return int The number of completion entries. 0 means no completion
       * entry.
       * @exception std::runtime_exception Error occured while polling the
       * completion queue.
       */
      int poll(std::vector<ibv_wc>& wc_vec) { return poll(wc_vec.data(), wc_vec.size()); }

      int poll(ibv_wc* wc, int count)
      {
         int rc = ::ibv_poll_cq(cq_.get(), count, wc);
         if (rc < 0) {
            throw_with("failed to poll cq: %s (rc=%d)", strerror(rc), rc);
         }
         return rc;
      }
   };
} // namespace rdmapp