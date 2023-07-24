#pragma once

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
} // namespace rdmapp