#pragma once

#include <endian.h>
#include <netinet/in.h>

#include <algorithm>
#include <cstdint>
#include <type_traits>

namespace rdmapp::detail
{

   static inline uint16_t ntoh(const uint16_t& value) { return ::be16toh(value); }

   static inline uint32_t ntoh(const uint32_t& value) { return ::be32toh(value); }

   static inline uint64_t ntoh(const uint64_t& value) { return ::be64toh(value); }

   static inline uint16_t hton(const uint16_t& value) { return ::htobe16(value); }

   static inline uint32_t hton(const uint32_t& value) { return ::htobe32(value); }

   static inline uint64_t hton(const uint64_t& value) { return ::htobe64(value); }

   template <std::integral T, class It>
   void serialize(const T& value, It& it)
   {
      T nvalue = hton(value);
      std::copy_n(reinterpret_cast<uint8_t*>(&nvalue), sizeof(T), it);
   }

   template <std::integral T, class It>
   void deserialize(It& it, T& value)
   {
      std::copy_n(it, sizeof(T), reinterpret_cast<uint8_t*>(&value));
      it += sizeof(T);
      value = ntoh(value);
   }

   template <class T, class It> requires(std::same_as<T, void*>)
   void deserialize(It& it, T& value)
   {
      std::copy_n(it, sizeof(T), reinterpret_cast<uint8_t*>(&value));
      it += sizeof(T);
      value = reinterpret_cast<void*>(ntoh(reinterpret_cast<uint64_t>(value)));
   }

} // namespace rdmapp