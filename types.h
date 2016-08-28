#pragma once

namespace order_book
{
  using order_id_t = uint32_t;
  using qty_t = uint32_t;
  using price_t = double;

  enum class MessageType : char
  {
    unknown,
    add = 'A',
    mod = 'M',
    del = 'X',
    trade = 'T'
  };

  enum class SideType : uint8_t
  {
    bid = 0,
    ask,
    cardinality,
    unknown
  };

  inline SideType ToSide(char c)
  {
    switch(c)
    {
      case 'B':
        return SideType::bid;
      case 'S':
        return SideType::ask;
      default:
        return SideType::unknown;
    }
  }

  inline const char* ToStr(SideType side)
  {
    switch(side)
    {
      case SideType::bid:
        return "B";
      case SideType::ask:
        return "S";
      default:
        return "unknown";
    }
  }

  struct InvalidStats
  {
    uint64_t num_corrupted_msg = 0;
    uint64_t num_duplicate_order = 0;
    uint64_t num_unknown_trade = 0;
    uint64_t num_unknown_mod = 0;
    uint64_t num_crossed = 0;
    uint64_t num_invalid_neg = 0;
  };
}
