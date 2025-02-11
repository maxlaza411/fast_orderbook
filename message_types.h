#ifndef PROJECT_MESSAGE_TYPES_H_
#define PROJECT_MESSAGE_TYPES_H_

#include <cstddef>  // for size_t

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int kMaxBookLevels = 100;

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------
enum class MessageType {
  kUnknown = 0,
  kSnapshot,
  kDelta,
  kTrade,
};

enum class Side {
  kUndefined = 0,
  kYes,
  kNo,
};

// ---------------------------------------------------------------------------
// SnapshotMessage
// ---------------------------------------------------------------------------
/**
 * @brief Trivial aggregate for "orderbook_snapshot".
 */
struct SnapshotMessage {
  // Pointers & lengths for string fields
  const char* market_ticker_ptr;
  size_t market_ticker_len;

  const char* market_id_ptr;
  size_t market_id_len;

  int yes_count;
  unsigned int yes_price[kMaxBookLevels];
  unsigned int yes_qty[kMaxBookLevels];

  int no_count;
  unsigned int no_price[kMaxBookLevels];
  unsigned int no_qty[kMaxBookLevels];
};

// ---------------------------------------------------------------------------
// DeltaMessage
// ---------------------------------------------------------------------------
/**
 * @brief Trivial aggregate for "orderbook_delta".
 */
struct DeltaMessage {
  const char* market_ticker_ptr;
  size_t market_ticker_len;

  const char* market_id_ptr;
  size_t market_id_len;

  unsigned int price;
  int delta;
  Side side;
};

// ---------------------------------------------------------------------------
// TradeMessage
// ---------------------------------------------------------------------------
/**
 * @brief Trivial aggregate for "trade".
 */
struct TradeMessage {
  // e.g. trade_id
  const char* trade_id_ptr;
  size_t trade_id_len;

  const char* market_ticker_ptr;
  size_t market_ticker_len;

  unsigned int yes_price;
  unsigned int no_price;
  int count;
  Side taker_side;
  int ts;
};


#endif  // PROJECT_MESSAGE_TYPES_H_
