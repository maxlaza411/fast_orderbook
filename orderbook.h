#ifndef PROJECT_ORDERBOOK_H_
#define PROJECT_ORDERBOOK_H_

#include <cstdint> // for uint64_t
#include <utility> // for std::pair
#include <vector>  // for std::vector
#include <cstring> // for std::memset
#include "message_types.h"

/**
 * @class OrderBook
 *
 * @brief Maintains bids and asks in direct arrays indexed by price, plus
 *        two 128-bit bitsets (one for bids, one for asks) to indicate which
 *        prices have nonzero quantity. BSR/BSF instructions yield O(1)
 *        best-bid/ask lookups. We also provide top-N methods by repeatedly
 *        scanning local copies of these bitsets.
 *
 * NOTE: This implementation assumes kMaxBookLevels <= 128, because
 * we use a fixed 128-bit bitset. If kMaxBookLevels > 128, you must
 * expand the bitset size or choose another data structure.
 */
class OrderBook
{
public:
  /// We store [0..kMaxBookLevels-1] inclusive.
  static constexpr unsigned int kArraySize = kMaxBookLevels;

  /**
   * @brief 128-bit bitset split into two 64-bit parts
   */
  struct Bitset128
  {
    uint64_t low{0};
    uint64_t high{0};

    void SetBit(unsigned int pos);
    void ClearBit(unsigned int pos);
    bool TestBit(unsigned int pos) const;
    int HighestSetBit() const;
    int LowestSetBit() const;
  };

  OrderBook();

  void ApplySnapshot(const SnapshotMessage *snap);
  void ApplyDelta(const DeltaMessage *msg);
  void ApplyTrade(const TradeMessage *trade);

  std::pair<unsigned int, unsigned int> BestBid() const;
  std::pair<unsigned int, unsigned int> BestAsk() const;

  std::vector<std::pair<unsigned int, unsigned int>> GetTopNBids(int n) const;
  std::vector<std::pair<unsigned int, unsigned int>> GetTopNAsks(int n) const;

private:
  /// Bids array (indexed by price 0..kMaxBookLevels-1).
  alignas(64) unsigned int bids_[kArraySize];
  /// Asks array (indexed by price 0..kMaxBookLevels-1).
  alignas(64) unsigned int asks_[kArraySize];

  /// Which bid prices have nonzero qty
  Bitset128 bids_bitset_;
  /// Which ask prices have nonzero qty
  Bitset128 asks_bitset_;
};

#endif // PROJECT_ORDERBOOK_H_
