// Stub implementation and example driver for SimpleCross.
// Your crossing logic should be accesible from the SimpleCross class.
// Other than the signature of SimpleCross::action() you are free to modify as
// needed.
#include "simple_cross.h"

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

results_t OrderBook::PlaceOrder(Order& current) {
  results_t results;
  switch (current.side) {
    case OrderSide::kBuySide:
      for (auto itr = sell_side_.begin(); itr != sell_side_.end() &&
                                          itr->px <= current.px &&
                                          current.qty > 0;) {
        auto itr_copy = itr++;
        auto other = sell_side_.extract(itr_copy);
        int delta = std::min(current.qty, other.value().qty);
        current.qty -= delta;
        other.value().qty -= delta;
        results.push_back(
            GetResultForOrderFill(current.oid, delta, other.value().px));
        results.push_back(
            GetResultForOrderFill(other.value().oid, delta, other.value().px));
        if (other.value().qty > 0) {
          sell_side_.insert(std::move(other));
        } else {
          sell_orders_by_oid_.erase(other.value().oid);
        }
      }
      if (current.qty > 0) {
        auto [itr, inserted] = buy_side_.insert(current);
        buy_orders_by_oid_[current.oid] = itr;
      }
      break;
    case OrderSide::kSellSide:
      for (auto itr = buy_side_.begin(); itr != buy_side_.end() &&
                                         itr->px >= current.px &&
                                         current.qty > 0;) {
        auto itr_copy = itr++;
        auto other = buy_side_.extract(itr_copy);
        int delta = std::min(current.qty, other.value().qty);
        current.qty -= delta;
        other.value().qty -= delta;
        results.push_back(
            GetResultForOrderFill(current.oid, delta, other.value().px));
        results.push_back(
            GetResultForOrderFill(other.value().oid, delta, other.value().px));
        if (other.value().qty > 0) {
          buy_side_.insert(std::move(other));
        } else {
          buy_orders_by_oid_.erase(other.value().oid);
        }
      }
      if (current.qty > 0) {
        auto [itr, inserted] = sell_side_.insert(current);
        sell_orders_by_oid_[current.oid] = itr;
      }
      break;
  }
  return results;
}

results_t OrderBook::CancelOrder(int oid) {
  results_t results;
  auto buy_itr = buy_orders_by_oid_.find(oid);
  if (buy_itr != buy_orders_by_oid_.end()) {
    buy_side_.erase(buy_itr->second);
    buy_orders_by_oid_.erase(buy_itr);
    results.push_back("X " + std::to_string(oid));
    return results;
  }
  auto sell_itr = sell_orders_by_oid_.find(oid);
  if (sell_itr != sell_orders_by_oid_.end()) {
    sell_side_.erase(sell_itr->second);
    sell_orders_by_oid_.erase(sell_itr);
    results.push_back("X " + std::to_string(oid));
    return results;
  }
  results.push_back("E " + std::to_string(oid) +
                    " Cancellation failed: order id in cancel request has "
                    "been filled completely");
  return results;
}

results_t OrderBook::PrintBook() const {
  results_t results;
  for (auto itr = sell_side_.rbegin(); itr != sell_side_.rend(); ++itr) {
    results.push_back(GetResultForPrintOrder(*itr));
  }
  for (auto itr = buy_side_.begin(); itr != buy_side_.end(); ++itr) {
    results.push_back(GetResultForPrintOrder(*itr));
  }
  return results;
}

std::string OrderBook::GetResultForOrderFill(int oid, int delta,
                                             double px) const {
  return "F " + std::to_string(oid) + " " + symbol_ + " " +
         std::to_string(delta) + " " + std::to_string(px);
}

std::string OrderBook::GetResultForPrintOrder(const Order& o) const {
  return "P " + std::to_string(o.oid) + " " + symbol_ + " " +
         (o.side == OrderSide::kBuySide ? kBuySideCode : kSellSideCode) + " " +
         std::to_string(o.qty) + " " + std::to_string(o.px);
}

results_t SimpleCross::action(const std::string& line) {
  std::stringstream stream(line);
  std::string token;
  results_t results;
  if (!std::getline(stream, token, kInputLineDelimiter)) {
    return results_t();
  }
  ActionCode code = static_cast<ActionCode>(token[0]);
  std::vector<std::string> tokens;
  while (std::getline(stream, token, kInputLineDelimiter)) {
    tokens.push_back(std::move(token));
  }
  switch (code) {
    case (ActionCode::kPlaceOrder):
      return PlaceOrder(tokens);
      break;
    case (ActionCode::kCancelOrder):
      return CancelOrder(tokens);
      break;
    case (ActionCode::kPrintBook):
      return PrintBook();
      break;
  }

  return results_t();
}

results_t SimpleCross::PlaceOrder(const std::vector<std::string>& tokens) {
  results_t results;
  if (tokens.empty()) {
    return results;
  }
  int oid = stoi(tokens[0]);
  const std::string& symbol = tokens[1];
  OrderSide side =
      tokens[2] == kBuySideCode ? OrderSide::kBuySide : OrderSide::kSellSide;
  auto [sid_itr, symbol_inserted] =
      symbol_to_sid_map_.try_emplace(symbol, symbol_to_sid_map_.size());
  size_t sid = sid_itr->second;
  if (sid == orders_by_sid_.size()) {
    orders_by_sid_.push_back(OrderBook(symbol));
  }
  auto [seen_itr, sid_inserted] = sids_by_oid_map_.try_emplace(oid, sid);
  if (!sid_inserted) {
    results.push_back("E " + tokens[0] + " Duplicate order id");
    return results;
  }
  Order current_order{.px = stod(tokens[4]),
                      .time = counter_++,
                      .qty = stoi(tokens[3]),
                      .oid = oid,
                      .side = side};
  return orders_by_sid_[sid].PlaceOrder(current_order);
}

results_t SimpleCross::CancelOrder(const std::vector<std::string>& tokens) {
  int oid = stoi(tokens[0]);
  const auto& itr = sids_by_oid_map_.find(oid);
  if (itr == sids_by_oid_map_.end()) {
    results_t results;
    results.push_back(
        "E " + tokens[0] +
        " Cancellation failed: order id in cancel request was never placed");
    return results;
  }

  return orders_by_sid_[itr->second].CancelOrder(oid);
}

results_t SimpleCross::PrintBook() const {
  results_t results;
  for (const auto& book : orders_by_sid_) {
    results_t book_results = book.PrintBook();
    results.insert(results.end(), std::make_move_iterator(book_results.begin()),
                   std::make_move_iterator(book_results.end()));
  }
  return results;
}