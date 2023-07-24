#pragma once

#include <infiniband/verbs.h>

#include <array>
#include <cassert>
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

   /**
    * @brief This class is an abstraction of a Completion Queue.
    *
    */
   struct cq : public noncopyable
   {
     private:
      std::shared_ptr<device> device_;
      struct ibv_cq* cq_;
      friend struct qp;

     public:
      /**
       * @brief Construct a new cq object.
       *
       * @param device The device to use.
       * @param num_cqe The number of completion entries to allocate.
       */
      cq(std::shared_ptr<device> device, size_t num_cqe = 128) : device_(device)
      {
         cq_ = ::ibv_create_cq(device->ctx_, num_cqe, this, nullptr, 0);
         check_ptr(cq_, "failed to create cq");
         RDMAPP_LOG_TRACE("created cq: %p", reinterpret_cast<void*>(cq_));
      }

      /**
       * @brief Poll the completion queue.
       *
       * @param wc If any, this will be filled with a completion entry.
       * @return true If there is a completion entry.
       * @return false If there is no completion entry.
       * @exception std::runtime_exception Error occured while polling the
       * completion queue.
       */
      bool poll(struct ibv_wc& wc)
      {
         if (auto rc = ::ibv_poll_cq(cq_, 1, &wc); rc < 0) [[unlikely]] {
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
       * @return size_t The number of completion entries. 0 means no completion
       * entry.
       * @exception std::runtime_exception Error occured while polling the
       * completion queue.
       */
      size_t poll(std::vector<struct ibv_wc>& wc_vec) { return poll(&wc_vec[0], wc_vec.size()); }

      template <class It>
      size_t poll(It wc, int count)
      {
         int rc = ::ibv_poll_cq(cq_, count, wc);
         if (rc < 0) {
            throw_with("failed to poll cq: %s (rc=%d)", strerror(rc), rc);
         }
         return rc;
      }
      template <int N>
      size_t poll(std::array<struct ibv_wc, N>& wc_array)
      {
         return poll(&wc_array[0], N);
      }
      
      ~cq()
      {
         if (cq_ == nullptr) [[unlikely]] {
            return;
         }

         if (auto rc = ::ibv_destroy_cq(cq_); rc != 0) [[unlikely]] {
            RDMAPP_LOG_ERROR("failed to destroy cq %p: %s", reinterpret_cast<void*>(cq_), strerror(errno));
         }
         else {
            RDMAPP_LOG_TRACE("destroyed cq: %p", reinterpret_cast<void*>(cq_));
         }
      }
   };

} // namespace rdmapp