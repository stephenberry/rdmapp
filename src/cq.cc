#include "rdmapp/cq.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <infiniband/verbs.h>

#include "rdmapp/error.h"

#include "rdmapp/detail/debug.h"

namespace rdmapp {

bool cq::poll(struct ibv_wc &wc) {
  if (auto rc = ::ibv_poll_cq(cq_, 1, &wc); rc < 0) [[unlikely]] {
    check_rc(-rc, "failed to poll cq");
  } else if (rc == 0) {
    return false;
  } else {
    return true;
  }
  return false;
}

size_t cq::poll(std::vector<struct ibv_wc> &wc_vec) {
  return poll(&wc_vec[0], wc_vec.size());
}

cq::~cq() {
  if (cq_ == nullptr) [[unlikely]] {
    return;
  }

  if (auto rc = ::ibv_destroy_cq(cq_); rc != 0) [[unlikely]] {
    RDMAPP_LOG_ERROR("failed to destroy cq %p: %s",
                     reinterpret_cast<void *>(cq_), strerror(errno));
  } else {
    RDMAPP_LOG_TRACE("destroyed cq: %p", reinterpret_cast<void *>(cq_));
  }
}

} // namespace rdmapp