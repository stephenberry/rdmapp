#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
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

   template <class... Args>
   inline constexpr void format_throw(std::format_string<Args...> fmt, Args&&... args) {
      throw std::runtime_error(std::format(fmt, std::forward<Args>(args)...));
   }

   inline void check_ptr(void* ptr, const std::string_view message)
   {
      if (ptr == nullptr) [[unlikely]] {
         format_throw("{}: {} (errno={})", message, std::strerror(errno), errno);
      }
   }

   template <class T>
   inline void check_ptr(const std::shared_ptr<T>& ptr, const char* message) {
      if (ptr == nullptr) [[unlikely]] {
         throw std::runtime_error(message);
      }
   }

   inline void check_errno(int rc, const char* message)
   {
      if (rc < 0) [[unlikely]] {
         format_throw("{}: {} (errno={})", message, std::strerror(errno), errno);
      }
   }

   static inline void check_rc(int rc, const char* message)
   {
      if (rc != 0) [[unlikely]] {
         format_throw("{}: {} (rc={})", message, std::strerror(rc), rc);
      }
   }
}