#pragma once

#include <infiniband/verbs.h>

#include <atomic>
#include <functional>
#include <thread>

#include "rdmapp/completion_queue.h"
#include "rdmapp/detail/debug.h"
#include "rdmapp/detail/util.h"
#include "rdmapp/executor.h"

namespace rdmapp
{
   // This class is used to poll a completion queue.
   struct cq_poller
   {
      std::shared_ptr<completion_queue> cq{}; // The completion queue to poll.
      std::shared_ptr<executor> exec{}; // The executor to use to process the completion entries.
      size_t batch_size = 16; // The number of completion entries to poll at a time.
      std::atomic<bool> stopped{};
      std::thread poller_thread{&cq_poller::worker, this};
      std::vector<ibv_wc> wc_vec = std::vector<ibv_wc>(batch_size);

      ~cq_poller()
      {
         stopped = true;
         poller_thread.join();
      }

      void worker()
      {
         while (!stopped) {
            try {
               auto nr_wc = cq->poll(wc_vec);
               for (int i = 0; i < nr_wc; ++i) {
                  auto& wc = wc_vec[i];
                  RDMAPP_LOG_TRACE("polled cqe wr_id=%p status=%d", reinterpret_cast<void*>(wc.wr_id), wc.status);
                  exec->process_wc(wc);
               }
            }
            catch (const std::runtime_error& e) {
               RDMAPP_LOG_ERROR("%s", e.what());
               stopped = true;
               return;
            }
            catch (const executor::queue_closed_error&) {
               stopped = true;
               return;
            }
         }
      }
   };
} // namespace rdmapp