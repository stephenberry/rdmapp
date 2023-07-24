#pragma once

#include <infiniband/verbs.h>

#include <functional>
#include <thread>
#include <vector>

#include "rdmapp/detail/blocking_queue.h"
#include "rdmapp/detail/debug.h"

namespace rdmapp
{
   // This class is used to execute callbacks of completion entries.
   struct executor
   {
     private:
      using work_queue_t = detail::blocking_queue<ibv_wc>;
      std::vector<std::thread> workers;
      work_queue_t work_queue;

      void worker_fn(size_t worker_id)
      {
         try {
            while (true) {
               auto wc = work_queue.pop();
               auto cb = reinterpret_cast<callback_ptr>(wc.wr_id);
               (*cb)(wc);
               destroy_callback(cb);
            }
         }
         catch (work_queue_t::queue_closed_error&) {
            RDMAPP_LOG_TRACE("executor worker %lu exited", worker_id);
         }
      }

     public:
      using queue_closed_error = work_queue_t::queue_closed_error;
      using callback_fn = std::function<void(const ibv_wc& wc)>;
      using callback_ptr = callback_fn*;

      executor(size_t n_worker_threads = 4)
      {
         for (size_t i = 0; i < n_worker_threads; ++i) {
            workers.emplace_back(&executor::worker_fn, this, i);
         }
      }

      /**
       * @brief Process a completion entry.
       *
       * @param wc The completion entry to process.
       */
      void process_wc(const ibv_wc& wc) { work_queue.push(wc); }

      void shutdown() { work_queue.close(); }

      ~executor()
      {
         shutdown();
         for (auto& worker : workers) {
            worker.join();
         }
      }

      /**
       * @brief Make a callback function that will be called when a completion entry
       * is processed. The callback function will be called in the executor's
       * thread. The lifetime of this pointer is controlled by the executor.
       * @tparam T The type of the callback function.
       * @param cb The callback function.
       * @return callback_ptr The callback function pointer.
       */
      template <class T>
      static callback_ptr make_callback(const T& cb)
      {
         return new executor::callback_fn(cb);
      }

      static void destroy_callback(callback_ptr cb) { delete cb; }
   };

} // namespace rdmapp