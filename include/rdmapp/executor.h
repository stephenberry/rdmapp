#pragma once

#include <infiniband/verbs.h>

#include <functional>
#include <thread>
#include <vector>

#include "rdmapp/detail/blocking_queue.h"
#include "rdmapp/detail/debug.h"

namespace rdmapp
{
   /**
    * @brief This class is used to execute callbacks of completion entries.
    */
   struct executor
   {
     private:
      using work_queue = detail::blocking_queue<struct ibv_wc>;
      std::vector<std::thread> workers_;
      work_queue work_queue_;
      void worker_fn(size_t worker_id);

     public:
      using queue_closed_error = work_queue::queue_closed_error;
      using callback_fn = std::function<void(const ibv_wc& wc)>;
      using callback_ptr = callback_fn*;

      /**
       * @brief Construct a new executor object
       *
       * @param nr_worker The number of worker threads to use.
       */
      executor(size_t nr_worker = 4)
      {
         for (size_t i = 0; i < nr_worker; ++i) {
            workers_.emplace_back(&executor::worker_fn, this, i);
         }
      }

      /**
       * @brief Process a completion entry.
       *
       * @param wc The completion entry to process.
       */
      void process_wc(const ibv_wc& wc);

      /**
       * @brief Shutdown the executor.
       *
       */
      void shutdown();

      ~executor();

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

      /**
       * @brief Destroy a callback function.
       *
       * @param cb The callback function pointer.
       */
      static void destroy_callback(callback_ptr cb);
   };

} // namespace rdmapp