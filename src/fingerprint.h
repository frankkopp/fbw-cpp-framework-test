//
// Created by frank on 04.03.2023.
//

#ifndef FBW_CPP_FRAMEWORK_TEST_FINGERPRINT_H
#define FBW_CPP_FRAMEWORK_TEST_FINGERPRINT_H

#include <cstdint>
#include <vector>

// Template function for fingerprinting vector data
template <typename T>
uint64_t fingerPrintFVN(const std::vector<T>& vec) {
  // Define some constants for FNV-1a hash
  const uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325;
  const uint64_t FNV_PRIME = 0x100000001b3;
  uint64_t fp = 0;
  for (const auto& elem : vec) {
    const T& value = elem;
    uint64_t hash = FNV_OFFSET_BASIS;
    const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
    for (size_t i = 0; i < sizeof(T); i++) {
      hash ^= static_cast<uint64_t>(bytes[i]);
      hash *= FNV_PRIME;
    }
    uint64_t h = hash;
    fp ^= h;
    fp *= FNV_PRIME;
  }
  return fp;
}
#endif  // FBW_CPP_FRAMEWORK_TEST_FINGERPRINT_H
