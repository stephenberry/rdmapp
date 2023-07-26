#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace rdmapp
{
    struct noncopyable
   {
      noncopyable() = default;
      noncopyable(noncopyable&&) = default;
      noncopyable(const noncopyable&) = delete;
      noncopyable& operator=(const noncopyable&) = delete;
      noncopyable& operator=(noncopyable&&) = default;
   };
}