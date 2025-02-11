#include "orderbook.h"

#include <cstring>  // For std::memset

namespace {

/**
 * @brief Bitmask used to ensure positions remain within the 128-bit limit.
 */
constexpr unsigned int kMaxBitPosition = 128U;

}  // namespace

// =============================================================================
// Bitset128 Method Definitions
// =============================================================================

/**
 * @brief Sets the bit in the 128-bit set at the given position.
 *
 * @param pos The bit position to set (0-indexed from the least significant bit).
 */
void OrderBook::Bitset128::SetBit(unsigned int pos) {
  // If pos >= 128, ignore (out of range for our fixed bitset).
  if (pos >= kMaxBitPosition) return;

  if (pos < 64U) {
    low |= (static_cast<uint64_t>(1) << pos);
  } else {
    high |= (static_cast<uint64_t>(1) << (pos - 64U));
  }
}

/**
 * @brief Clears (unsets) the bit in the 128-bit set at the given position.
 *
 * @param pos The bit position to clear (0-indexed from the least significant bit).
 */
void OrderBook::Bitset128::ClearBit(unsigned int pos) {
  if (pos >= kMaxBitPosition) return;

  if (pos < 64U) {
    low &= ~(static_cast<uint64_t>(1) << pos);
  } else {
    high &= ~(static_cast<uint64_t>(1) << (pos - 64U));
  }
}

/**
 * @brief Checks if the bit at the given position is set.
 *
 * @param pos The bit position to test.
 * @return True if the bit is set, false otherwise.
 */
bool OrderBook::Bitset128::TestBit(unsigned int pos) const {
  if (pos >= kMaxBitPosition) return false;

  if (pos < 64U) {
    return (low & (static_cast<uint64_t>(1) << pos)) != 0U;
  } else {
    return (high & (static_cast<uint64_t>(1) << (pos - 64U))) != 0U;
  }
}

/**
 * @brief Finds the highest set bit (most significant bit) in the 128-bit set.
 *
 * @return The index of the highest set bit, or -1 if none is set.
 */
int OrderBook::Bitset128::HighestSetBit() const {
  // Check the high 64 bits first.
  if (high != 0U) {
#if defined(__GNUC__) || defined(__clang__)
    // __builtin_clzll returns the number of leading zeros in a 64-bit value.
    // The highest set bit index in 'x' is 63 - __builtin_clzll(x).
    const int lead = __builtin_clzll(high);
    return 64 + (63 - lead);
#else
    // Fallback: naive loop from high bit to low bit.
    for (int i = 63; i >= 0; --i) {
      if ((high >> i) & 1ULL) {
        return 64 + i;
      }
    }
    return -1;
#endif
  } else if (low != 0U) {
#if defined(__GNUC__) || defined(__clang__)
    const int lead = __builtin_clzll(low);
    return (63 - lead);
#else
    for (int i = 63; i >= 0; --i) {
      if ((low >> i) & 1ULL) {
        return i;
      }
    }
    return -1;
#endif
  }
  return -1;  // No bits set.
}

/**
 * @brief Finds the lowest set bit (least significant bit) in the 128-bit set.
 *
 * @return The index of the lowest set bit, or -1 if none is set.
 */
int OrderBook::Bitset128::LowestSetBit() const {
  // Check the low 64 bits first.
  if (low != 0U) {
#if defined(__GNUC__) || defined(__clang__)
    // __builtin_ctzll returns the count of trailing zeros in a 64-bit value.
    return __builtin_ctzll(low);
#else
    for (int i = 0; i < 64; ++i) {
      if ((low >> i) & 1ULL) {
        return i;
      }
    }
    return -1;
#endif
  } else if (high != 0U) {
#if defined(__GNUC__) || defined(__clang__)
    return 64 + __builtin_ctzll(high);
#else
    for (int i = 0; i < 64; ++i) {
      if ((high >> i) & 1ULL) {
        return 64 + i;
      }
    }
    return -1;
#endif
  }
  return -1;  // No bits set.
}

// =============================================================================
// OrderBook Method Definitions
// =============================================================================

/**
 * @brief Default constructor that initializes the order book with zeroed data.
 */
OrderBook::OrderBook() {
  std::memset(bids_, 0, sizeof(bids_));
  std::memset(asks_, 0, sizeof(asks_));
  bids_bitset_.low = 0U;
  bids_bitset_.high = 0U;
  asks_bitset_.low = 0U;
  asks_bitset_.high = 0U;
}

/**
 * @brief Applies a snapshot to the order book, clearing previous data and loading new bids/asks.
 *
 * @param snap Pointer to a SnapshotMessage containing bid and ask data.
 */
void OrderBook::ApplySnapshot(const SnapshotMessage* snap) {
  // Clear everything first.
  std::memset(bids_, 0, sizeof(bids_));
  std::memset(asks_, 0, sizeof(asks_));
  bids_bitset_.low = 0U;
  bids_bitset_.high = 0U;
  asks_bitset_.low = 0U;
  asks_bitset_.high = 0U;

  // Load bids ("yes" side).
  for (int i = 0; i < snap->yes_count; ++i) {
    unsigned int price_value = snap->yes_price[i];
    unsigned int quantity_value = snap->yes_qty[i];
    if (price_value < kArraySize) {
      bids_[price_value] = quantity_value;
      if (quantity_value > 0U) {
        bids_bitset_.SetBit(price_value);
      }
    }
  }

  // Load asks ("no" side).
  for (int i = 0; i < snap->no_count; ++i) {
    unsigned int price_value = snap->no_price[i];
    unsigned int quantity_value = snap->no_qty[i];
    if (price_value < kArraySize) {
      asks_[price_value] = quantity_value;
      if (quantity_value > 0U) {
        asks_bitset_.SetBit(price_value);
      }
    }
  }
}

/**
 * @brief Applies a delta update to the order book, adjusting the specified side's quantity.
 *
 * @param msg Pointer to a DeltaMessage indicating side, price, and delta quantity.
 */
void OrderBook::ApplyDelta(const DeltaMessage* msg) {
  const unsigned int price_value = msg->price;
  const int delta_value = msg->delta;

  if (price_value >= kArraySize) {
    // Out of range -- ignore or handle error (no functionality changes here).
    return;
  }

  if (msg->side == Side::kYes) {
    // Bids
    bids_[price_value] += delta_value;
    if (bids_[price_value] == 0) {
      bids_bitset_.ClearBit(price_value);
    } else {
      bids_bitset_.SetBit(price_value);
    }
  } else if (msg->side == Side::kNo) {
    // Asks
    asks_[price_value] += delta_value;
    if (asks_[price_value] == 0) {
      asks_bitset_.ClearBit(price_value);
    } else {
      asks_bitset_.SetBit(price_value);
    }
  }
  // If side is undefined, do nothing.
}

/**
 * @brief Applies a trade message, removing executed quantity from the matched side of the book.
 *
 * @param trade Pointer to a TradeMessage containing trade details.
 */
void OrderBook::ApplyTrade(const TradeMessage* trade) {
  // The quantity to remove from the side that was "hit."
  const int remove_qty = trade->count;
  if (remove_qty <= 0) {
    return;  // No removal.
  }

  if (trade->taker_side == Side::kYes) {
    // A taker on the "yes" side consumes "no" orders from the ask side.
    unsigned int price_value = trade->no_price;
    if (price_value < kArraySize) {
      asks_[price_value] -= remove_qty;
      if (asks_[price_value] == 0) {
        asks_bitset_.ClearBit(price_value);
      }
    }
  } else if (trade->taker_side == Side::kNo) {
    // A taker on the "no" side consumes "yes" orders from the bid side.
    unsigned int price_value = trade->yes_price;
    if (price_value < kArraySize) {
      bids_[price_value] -= remove_qty;
      if (bids_[price_value] == 0) {
        bids_bitset_.ClearBit(price_value);
      }
    }
  }
}

/**
 * @brief Retrieves the best bid (highest price) and its quantity.
 *
 * @return A pair of (price, quantity), or (0, 0) if no valid bid exists.
 */
std::pair<unsigned int, unsigned int> OrderBook::BestBid() const {
  int idx = bids_bitset_.HighestSetBit();
  if (idx >= 0 && static_cast<unsigned int>(idx) < kArraySize) {
    return {static_cast<unsigned int>(idx), bids_[idx]};
  }
  return {0U, 0U};  // No valid bid.
}

/**
 * @brief Retrieves the best ask (lowest price) and its quantity.
 *
 * @return A pair of (price, quantity), or (0, 0) if no valid ask exists.
 */
std::pair<unsigned int, unsigned int> OrderBook::BestAsk() const {
  int idx = asks_bitset_.LowestSetBit();
  if (idx >= 0 && static_cast<unsigned int>(idx) < kArraySize) {
    return {static_cast<unsigned int>(idx), asks_[idx]};
  }
  return {0U, 0U};  // No valid ask.
}

/**
 * @brief Retrieves the top N bids in descending order of price.
 *
 * @param n The number of bids to retrieve.
 * @return A vector of (price, quantity) pairs, from highest to lowest.
 */
std::vector<std::pair<unsigned int, unsigned int>> OrderBook::GetTopNBids(int n) const {
  std::vector<std::pair<unsigned int, unsigned int>> result;
  result.reserve(n);

  Bitset128 mask = bids_bitset_;

  while (n > 0) {
    int idx = mask.HighestSetBit();
    if (idx < 0 || static_cast<unsigned int>(idx) >= kArraySize) {
      break;
    }
    result.emplace_back(static_cast<unsigned int>(idx), bids_[idx]);
    mask.ClearBit(idx);
    --n;
  }
  return result;
}

/**
 * @brief Retrieves the top N asks in ascending order of price.
 *
 * @param n The number of asks to retrieve.
 * @return A vector of (price, quantity) pairs, from lowest to highest.
 */
std::vector<std::pair<unsigned int, unsigned int>> OrderBook::GetTopNAsks(int n) const {
  std::vector<std::pair<unsigned int, unsigned int>> result;
  result.reserve(n);

  Bitset128 mask = asks_bitset_;

  while (n > 0) {
    int idx = mask.LowestSetBit();
    if (idx < 0 || static_cast<unsigned int>(idx) >= kArraySize) {
      break;
    }
    result.emplace_back(static_cast<unsigned int>(idx), asks_[idx]);
    mask.ClearBit(idx);
    --n;
  }
  return result;
}
