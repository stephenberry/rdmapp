#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>

namespace rdmapp::detail
{
   template <class T>
   struct blocking_queue
   {
     private:
      std::mutex mutex{};
      std::condition_variable cv{};
      std::queue<T> queue{};
      bool closed{};

     public:
      struct queue_closed_error
      {};

      void push(const T& item)
      {
         std::lock_guard lock(mutex);
         if (closed) {
            throw queue_closed_error();
         }
         queue.push(item);
         cv.notify_one();
      }
      T pop()
      {
         std::unique_lock lock(mutex);
         cv.wait(lock, [this]() { return !queue.empty() || closed; });
         if (queue.empty()) {
            throw queue_closed_error();
         }
         T item = queue.front();
         queue.pop();
         return item;
      }
      void close()
      {
         std::lock_guard lock(mutex);
         closed = true;
         cv.notify_all();
      }
   };
} // namespace rdmapp