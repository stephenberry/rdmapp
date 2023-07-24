#include "rdmapp/mr.h"

#include <cstdint>
#include <utility>
#include <vector>

#include <infiniband/verbs.h>

#include "rdmapp/detail/debug.h"
#include "rdmapp/detail/serdes.h"

namespace rdmapp {


void *local_mr::addr() const { return mr_->addr; }

size_t local_mr::length() const { return mr_->length; }

uint32_t local_mr::rkey() const { return mr_->rkey; }

uint32_t local_mr::lkey() const { return mr_->lkey; }



void *remote_mr::addr() { return addr_; }

uint32_t remote_mr::length() { return length_; }

uint32_t remote_mr::rkey() { return rkey_; }

} // namespace rdmapp