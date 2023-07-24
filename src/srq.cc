#include "rdmapp/srq.h"

#include <cstring>
#include <memory>

#include <infiniband/verbs.h>

#include "rdmapp/device.h"
#include "rdmapp/error.h"

#include "rdmapp/detail/debug.h"

namespace rdmapp {

srq::~srq() {
  if (srq_ == nullptr) [[unlikely]] {
    return;
  }

  if (auto rc = ::ibv_destroy_srq(srq_); rc != 0) [[unlikely]] {
    RDMAPP_LOG_ERROR("failed to destroy srq %p: %s (rc=%d)",
                     reinterpret_cast<void *>(srq_), strerror(rc), rc);
  } else {
    RDMAPP_LOG_DEBUG("destroyed srq %p", reinterpret_cast<void *>(srq_));
  }
}

} // namespace rdmapp