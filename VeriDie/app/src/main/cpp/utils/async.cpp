#include "utils/callback.hpp"
#include "utils/canceller.hpp"

/**
 * Flag layout:
 * 0
 * 0 1 2 3 4 5 6 7 8
 * +-+-+-+-+-+-+-+-+
 * |    ID=6   |C|A|
 * +-+-+-+-+-+-+-+-+
 *
 *
 * CallbackId layout:
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    ID=6   |                   INDEX=26                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * A - alive bit. 1 = callback object exists, 0 = callback has gone out of scope.
 *
 * C - cancelled bit. Set when callback is being cancelled explicitly by its creator
 *     (i.e. via Canceller::CancelCallback()).
 *
 * ID - unique operation id in order to be able to reuse a flag once its previous operation has
 *      finished. Incremented each time the flag is used for a new operation.
 *      NB! Will wrap around eventually which might cause CallbackId clashes. To mitigate this
 *      slightly, a CallbackId is set to nullopt once we know that the operation is inactive or
 *      cancelled.
 *
 * INDEX - position of flag in the array storing all flags (i.e. m_activeFlags in Canceller).
 *
 */

namespace {
constexpr uint8_t MASK_ALIVE = 1 << 7;
constexpr uint8_t MASK_CANCELLED = 1 << 6;
constexpr int ID_LENGTH = 6;
constexpr uint8_t MASK_ID = (1 << ID_LENGTH) - 1;
} // namespace

async::internal::AtomicFlagRef::AtomicFlagRef(std::atomic<uint8_t> * block) noexcept
   : m_block(block)
{}
bool async::internal::AtomicFlagRef::IsEmpty() const noexcept
{
   return m_block == nullptr;
}
bool async::internal::AtomicFlagRef::IsAlive() const noexcept
{
   assert(!IsEmpty());
   return (m_block->load() & MASK_ALIVE) != 0;
}
bool async::internal::AtomicFlagRef::IsCancelled() const noexcept
{
   assert(!IsEmpty());
   return (m_block->load() & MASK_CANCELLED) != 0;
}
uint32_t async::internal::AtomicFlagRef::GetId() const noexcept
{
   assert(!IsEmpty());
   return m_block->load() & MASK_ID;
}
void async::internal::AtomicFlagRef::Activate() noexcept
{
   assert(!IsEmpty());
   m_block->fetch_add(1);
   m_block->fetch_or(MASK_ALIVE);
   m_block->fetch_and(static_cast<uint8_t>(~MASK_CANCELLED));
}
void async::internal::AtomicFlagRef::Deactivate() noexcept
{
   assert(!IsEmpty());
   m_block->fetch_and(static_cast<uint8_t>(~MASK_ALIVE));
}
void async::internal::AtomicFlagRef::Cancel() noexcept
{
   assert(!IsEmpty());
   m_block->fetch_or(MASK_CANCELLED);
}

bool async::internal::operator==(AtomicFlagRef lhs, AtomicFlagRef rhs) noexcept
{
   return lhs.m_block == rhs.m_block;
}
bool async::internal::operator!=(AtomicFlagRef lhs, AtomicFlagRef rhs) noexcept
{
   return !(lhs == rhs);
}

const std::shared_ptr<async::internal::CancellerToken> & async::internal::GlobalCancellerToken()
{
   static auto s_token = std::make_shared<CancellerToken>();
   return s_token;
}

uint32_t async::internal::MakeCallbackId(AtomicFlagRef flagRef, size_t index) noexcept
{
   uint32_t ret = index << ID_LENGTH;
   ret |= flagRef.GetId();
   return ret;
}
size_t async::internal::GetFlagIndex(uint32_t callbackId) noexcept
{
   return callbackId >> ID_LENGTH;
}
uint32_t async::internal::GetOperationId(uint32_t callbackId) noexcept
{
   return callbackId & MASK_ID;
}

void async::OnAllCompleted::Detach()
{
   if (m_state) {
      assert(m_state->trackedCount >= 10'000U);

      m_state->trackedCount -= 10'000U;
      if (m_state->firedCount == m_state->trackedCount) {
         m_state->listener();
         delete m_state;
         m_state = nullptr;
      }
   }
}

void async::OnAnyCompleted::Detach()
{
   if (m_state) {
      assert(m_state->trackedCount >= 10'000U);

      m_state->trackedCount -= 10'000U;
      if (m_state->firedCount > 0U) {
         m_state->listener();
      }
      if (m_state->firedCount == m_state->trackedCount) {
         delete m_state;
         m_state = nullptr;
      }
   }
}