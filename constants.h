#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stddef.h>
#include <stdint.h>

constexpr char kInputLineDelimiter = ' ';
constexpr size_t kPlaceOrderArgSize = 5;
constexpr size_t kCancelOrderArgSize = 1;
constexpr char kBuySideCode[2] = "B";
constexpr char kSellSideCode[2] = "S";

enum class ActionCode : char {
  kPlaceOrder = 'O',
  kCancelOrder = 'X',
  kPrintBook = 'P',
};

enum class OrderSide {
  kBuySide = 0,
  kSellSide = 1,
};

struct Order {
  double px;
  int64_t time;
  int qty = 0;
  int oid = 0;
  OrderSide side;

  auto operator<=>(const Order&) const = default;
};

#endif  // CONSTANTS_H