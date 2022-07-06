#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "constants.h"

struct Order {
  double px;
  int time;
  int qty = 0;
  int oid = 0;
  OrderSide side;

  auto operator<=>(const Order&) const = default;
};

class OrderBook {
 public:
  results_t PlaceOrder(const std::string& symbol_, Order& current) {
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
          results.push_back("F " + std::to_string(current.oid) + " " + symbol_ +
                            " " + std::to_string(delta) + " " +
                            std::to_string(other.value().px));
          results.push_back("F " + std::to_string(other.value().oid) + " " +
                            symbol_ + " " + std::to_string(delta) + " " +
                            std::to_string(other.value().px));
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
          results.push_back("F " + std::to_string(current.oid) + " " + symbol_ +
                            " " + std::to_string(delta) + " " +
                            std::to_string(other.value().px));
          results.push_back("F " + std::to_string(other.value().oid) + " " +
                            symbol_ + " " + std::to_string(delta) + " " +
                            std::to_string(other.value().px));
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

  results_t CancelOrder(int oid) {
    results_t results;
    auto buy_itr = buy_orders_by_oid_.find(oid);
    if (buy_itr != buy_orders_by_oid_.end()) {
      buy_side_.erase(buy_itr->second);
      buy_orders_by_oid_.erase(buy_itr);
      results.push_back("X " + oid);
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

  results_t PrintBook() const {
    results_t results;
    for (auto itr = sell_side_.rbegin(); itr != sell_side_.rend(); ++itr) {
      results.push_back("P " + std::to_string(itr->oid) + " " + symbol_ +
                        " S " + std::to_string(itr->qty) + " " +
                        std::to_string(itr->px));
    }
    for (auto itr = buy_side_.begin(); itr != buy_side_.end(); ++itr) {
      results.push_back("P " + std::to_string(itr->oid) + " " + symbol_ +
                        " B " + std::to_string(itr->qty) + " " +
                        std::to_string(itr->px));
    }
    return results;
  }

 private:
  std::string symbol_;
  std::set<Order, std::greater<Order>> buy_side_;
  std::set<Order> sell_side_;
  std::unordered_map<int, decltype(buy_side_.begin())> buy_orders_by_oid_;
  std::unordered_map<int, decltype(sell_side_.begin())> sell_orders_by_oid_;
};

#endif  // ORDER_BOOK_H