#include "simple_cross.h"

#include <assert.h>

#include <fstream>
#include <iostream>
#include <string>

bool Matcher(const std::string& fn, const results_t& expectation) {
  std::cout << "Testing file " << fn << std::endl;
  SimpleCross scross;
  std::string line;
  std::ifstream actions(fn, std::ios::in);
  auto expectation_itr = expectation.begin();
  int i = 0;
  while (std::getline(actions, line)) {
    results_t results = scross.action(line);
    results_t::const_iterator it = results.begin();
    for (; it != results.end() && expectation_itr != expectation.end();
         ++it, ++i, ++expectation_itr) {
      if (*it != *expectation_itr) {
        std::cout << "FAILED: Expected and actual results differed at line "
                  << i << std::endl;
        std::cout << "Expected: " << *expectation_itr << std::endl;
        std::cout << "Actual: " << *it << std::endl;
        return false;
      }
    }
    if (expectation_itr == expectation.end() && it != results.end()) {
      std::cout << "FAILED: Expected had fewer results than actual."
                << std::endl;
      std::cout << "Next result in actual: " << *it << std::endl;
      return false;
    }
  }
  if (expectation_itr != expectation.end()) {
    std::cout << "FAILED: Expected had more results than actual." << std::endl;
    std::cout << "Next result in expected: " << *expectation_itr << std::endl;
    return false;
  }
  std::cout << "PASSED: Expected matched actual." << std::endl;;
  return true;
}

int main() {
  SimpleCross scross;
  std::string line;
  std::ifstream actions("actions.txt", std::ios::in);
  while (std::getline(actions, line)) {
    results_t results = scross.action(line);
    results_t::const_iterator it = results.begin();
    for (; it != results.end(); ++it) {
      std::cout << *it << std::endl;
    }
  }
  assert(Matcher("./tests/actions.txt",
                 {"F 10003 IBM 5 100.000000", "F 10000 IBM 5 100.000000",
                  "F 10004 IBM 5 100.000000", "F 10000 IBM 5 100.000000",
                  "X 10002", "E 10008 Duplicate order id",
                  "P 10009 IBM S 10 102.000000", "P 10008 IBM S 10 102.000000",
                  "P 10007 IBM S 10 101.000000", "P 10006 IBM B 10 100.000000",
                  "P 10005 IBM B 10 99.000000", "P 10001 IBM B 10 99.000000",
                  "F 10010 IBM 10 101.000000", "F 10007 IBM 10 101.000000",
                  "F 10010 IBM 3 102.000000", "F 10008 IBM 3 102.000000"}));
  assert(Matcher("./tests/multiple_symbols.txt",
                 {"F 10003 APPL 5 100.000000", "F 10001 APPL 5 100.000000",
                  "P 10002 IBM B 10 100.000000", "P 10000 IBM B 10 100.000000",
                  "P 10001 APPL B 5 100.000000", "X 10002", "X 10001",
                  "P 10000 IBM B 10 100.000000"}));
  return 0;
}