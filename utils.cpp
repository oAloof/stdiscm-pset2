#include "utils.h"

#include <utility>

// Return a random integer in [lo, hi] inclusive range
auto random_int(int lo, int hi) -> int
{
  static thread_local std::mt19937 rng{std::random_device{}()};   // thread-local RNG for concurrency
  std::uniform_int_distribution<int> dist(lo, hi);
  return dist(rng);
}

// Padding utility to align strings
auto pad(const std::string &str, int width) -> std::string
{
  if (std::cmp_greater_equal(str.size(), width)) return str;
  return str + std::string(width - str.size(), ' ');
}
