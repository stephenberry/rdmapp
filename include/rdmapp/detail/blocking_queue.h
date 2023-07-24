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
      struct queue_closed_error
      {};

      void push(const T& item)
      {
         std::lock_guard lock(mtx);
         if (closed) {
            throw queue_closed_error();
         }
         queue.push(item);
         cv.notify_one();
      }
      T pop()
      {
         std::unique_lock lock(mtx);
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
         std::lock_guard lock(mtx);
         closed = true;
         cv.notify_all();
      }

      private:
      std::mutex mtx{};
      std::condition_variable cv{};
      std::queue<T> queue{};
      bool closed{};
   };
} // namespace rdmapp