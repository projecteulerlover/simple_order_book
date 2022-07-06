#ifndef SIMPLE_CROSS_H
#define SIMPLE_CROSS_H

#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "constants.h"

typedef std::list<std::string> results_t;

class OrderBook {
 public:
  OrderBook(const std::string& symbol) : symbol_(symbol){};

  // Handles placing the order `current`.
  // 1. Checks if the current order is buy/sell side.
  // 2. Iterates through sell/buy side (respectively) for matching orders,
  // sorted by best price then earliest time.
  // 3. If sell/buy side orders are exhausted, push remaining qty in to buy/sell
  // side.
  // 4. While doing above, updates sell_orders_by_oid_/buy_orders_by_oid_ by
  // removing orders that have been completely filled and adding leftover
  // `current` order if not completely filled.
  // 5. Adds (partially) filled orders on both buy/sell sides to `results` and
  // returns this value (e.g., "F 10003 IBM 5 100.000000"_.
  results_t PlaceOrder(Order& current);

  // Handles cancelling the order corresponding to `oid`.
  // Arriving to this function means an order with the previous `oid` was seen.
  // If oid does not exist in `buy_orders_by_oid_` nor `sell_orders_by_oid_`,
  // adds a result of type error (e.g., "E 10010 Cancellation failed: order id
  // in cancel request has been filled completely").
  // If it is found, logs a cancel success result (e.g., X 10000) and removes it
  // from buy/sell_side_ and buy/sell_orders_by_oid_.
  results_t CancelOrder(int oid);

  // By construction, buy and sell order collections cannot have overlapping
  // prices (otherwise those entries would be executed). So, this requires
  // iterating through sell collection in order of decreasing prices, then buy
  // collection in order of decreasing prices.
  results_t PrintBook() const;

 private:
  // Creates a formatted string for successful order fills.
  std::string GetResultForOrderFill(int oid, int delta, double px) const;

  // Creates a formatted string for printing given order `o`.
  std::string GetResultForPrintOrder(const Order& o) const;

  // Symbol corresponding to this order book.
  std::string symbol_;
  // All placed but unfilled buy/sell orders.
  // `buy_side_` is decreasing because cheaper and older orders should be filled
  // first.
  std::set<Order, std::greater<Order>> buy_side_;
  std::set<Order> sell_side_;
  // Map from oid to its corresponding iterator in buy/sell_side_. Used for
  // cancellation.
  std::unordered_map<int, decltype(buy_side_.begin())> buy_orders_by_oid_;
  std::unordered_map<int, decltype(sell_side_.begin())> sell_orders_by_oid_;
};

class SimpleCross {
 public:
  SimpleCross() { std::cout.precision(5); }

  // Processes the given line into tokens and calls the corresponding action
  // below.
  results_t action(const std::string& line);
  
 private:
  // Checks for duplicate orders and returns an error result if oid previously
  // exists. Calls OrderBook::PlaceOrder for the order book for the given
  // symbol.
  results_t PlaceOrder(const std::vector<std::string>& tokens);

  // Checks the given oid was previously placed. Calls OrderBook::CancelOrder
  // for the order book for the given symbol.
  results_t CancelOrder(const std::vector<std::string>& tokens);

  // Iterates over all order books in `orders_by_sid_`, and calls
  // OrderBook::PrintBook.
  results_t PrintBook() const;

  // Map from symbol to a created `symbol_id`. Incremented in order symbols are
  // seen.
  std::unordered_map<std::string, int> symbol_to_sid_map_;
  // Indices of `orders_by_sid_` is the corresponding sid for that order book.
  std::vector<OrderBook> orders_by_sid_;
  // Creates a map between oid to sid. Used for cancellations, for which only an
  // oid is provided. This gives a link between an oid and its corresponding
  // order book.
  std::unordered_map<int, int> sids_by_oid_map_;
  int64_t counter_ = 0;
};

#endif  // SIMPLE_CROSS_H