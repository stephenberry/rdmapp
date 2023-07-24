#pragma once

#include <infiniband/verbs.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "rdmapp/detail/debug.h"
#include "rdmapp/detail/noncopyable.h"
#include "rdmapp/detail/serdes.h"

namespace rdmapp
{
   namespace tags::mr
   {
      struct local
      {};
      struct remote
      {};
   }

   struct pd;

   // A remote or local memory region.
   template <class Tag>
   struct mr;

   // Represents a local memory region.
   template <>
   struct mr<tags::mr::local> : public noncopyable
   {
      ibv_mr* mr_;
      std::shared_ptr<pd> pd_;

     public:
      /**
       * @brief Construct a new mr object
       *
       * @param pd The protection domain to use.
       * @param mr The ibverbs memory region handle.
       */
      mr(std::shared_ptr<pd> pd, ibv_mr* mr) : mr_(mr), pd_(pd) {}

      /**
       * @brief Move construct a new mr object
       *
       * @param other The other mr object to move from.
       */
      mr(mr<tags::mr::local>&& other) : mr_(std::exchange(other.mr_, nullptr)), pd_(std::move(other.pd_)) {}

      /**
       * @brief Move assignment operator.
       *
       * @param other The other mr to move from.
       * @return mr<tags::mr::local>& This mr.
       */

      mr<tags::mr::local>& operator=(mr<tags::mr::local>&& other)
      {
         mr_ = other.mr_;
         pd_ = std::move(other.pd_);
         other.mr_ = nullptr;
         return *this;
      }

      // Destroy the mr object and deregister the memory region.
      ~mr()
      {
         if (mr_ == nullptr) [[unlikely]] {
            // This mr is moved.
            return;
         }
         auto addr = mr_->addr;
         if (auto rc = ::ibv_dereg_mr(mr_); rc != 0) [[unlikely]] {
            RDMAPP_LOG_ERROR("failed to dereg mr %p addr=%p", reinterpret_cast<void*>(mr_), addr);
         }
         else {
            RDMAPP_LOG_TRACE("dereg mr %p addr=%p", reinterpret_cast<void*>(mr_), addr);
         }
      }

      /**
       * @brief Serialize the memory region handle to be sent to a remote peer.
       *
       * @return std::vector<uint8_t> The serialized memory region handle.
       */
      std::vector<uint8_t> serialize() const
      {
         std::vector<uint8_t> buffer;
         auto it = std::back_inserter(buffer);
         detail::serialize(reinterpret_cast<uint64_t>(mr_->addr), it);
         detail::serialize(mr_->length, it);
         detail::serialize(mr_->rkey, it);
         return buffer;
      }

      /**
       * @brief Get the address of the memory region.
       *
       * @return void* The address of the memory region.
       */
      void* addr() const { return mr_->addr; }

      /**
       * @brief Get the length of the memory region.
       *
       * @return size_t The length of the memory region.
       */
      size_t length() const { return mr_->length; }

      /**
       * @brief Get the remote key of the memory region.
       *
       * @return uint32_t The remote key of the memory region.
       */
      uint32_t rkey() const { return mr_->rkey; }

      /**
       * @brief Get the local key of the memory region.
       *
       * @return uint32_t The local key of the memory region.
       */
      uint32_t lkey() const { return mr_->lkey; }
   };

   // Represents a remote memory region.
   template <>
   struct mr<tags::mr::remote>
   {
      void* addr{}; // The address of the remote memory region.
      uint32_t length{}; // The length of the remote memory region.
      uint32_t rkey{}; // The remote key of the memory region.

      // The size of a serialized remote memory region.
      static constexpr size_t kSerializedSize = sizeof(addr) + sizeof(length) + sizeof(rkey);

      /**
       * @brief Deserialize a remote memory region handle.
       *
       * @tparam It The iterator type.
       * @param it The iterator to deserialize from.
       * @return mr<tags::mr::remote> The deserialized remote memory region handle.
       */
      template <class It>
      static mr<tags::mr::remote> deserialize(It it)
      {
         mr<tags::mr::remote> remote_mr;
         detail::deserialize(it, remote_mr.addr);
         detail::deserialize(it, remote_mr.length);
         detail::deserialize(it, remote_mr.rkey);
         return remote_mr;
      }
   };

   using local_mr = mr<tags::mr::local>;
   using remote_mr = mr<tags::mr::remote>;

} // namespace rdmapp