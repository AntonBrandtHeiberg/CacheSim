#pragma once

#include "MainMemory.h"
#include <array>
#include <cstdint>
#include <cstring>

const uint8_t MEMORY_ADRESS_SIZE = 32;  // 32-bit memory adresses
const uint8_t CACHE_LINE_SIZE = 64;     // 64-byte cache lines
const uint8_t CACHE_SETS = 64;          // cache with 64 sets
const uint8_t CACHE_WAYS = 16;          // 16-way associative

const uint8_t CACHE_LINE_BYTE_OFFSET_SIZE = 6; // 2^6 = 64 one for each 
const uint8_t CACHE_LINE_SET_INDEX_SIZE = 6; // 2^6 = 64 one for each set
const uint8_t CACHE_LINE_TAG = 20;

struct CacheLine {
  uint32_t tag = 0;
  std::array<uint8_t, CACHE_LINE_SIZE> data = {0};
  bool dirty = false;
  bool valid = false;
  uint64_t last_used = 0;
};

struct AddressParts {
  uint32_t tag;
  uint32_t setIndex;
  uint8_t byteOffset;

  AddressParts(uint32_t address) {
    tag = address >> (CACHE_LINE_BYTE_OFFSET_SIZE + CACHE_LINE_SET_INDEX_SIZE);
    setIndex = (address >> CACHE_LINE_BYTE_OFFSET_SIZE) & ((1 << CACHE_LINE_SET_INDEX_SIZE ) -1);
    byteOffset = address & (CACHE_LINE_SIZE - 1);
  }
};

class CacheSet {
public:
  CacheSet();
  ~CacheSet();

  CacheLine* Find(uint32_t tag);
  //
  CacheLine* Replace(uint32_t tag, uint8_t* sourceData, uint64_t currentTime);

private:
  std::array<CacheLine, CACHE_WAYS> lines;
};

class Cache { 
public:
  void Initialize(MainMemory* memory);
  uint32_t Read(uint32_t address);
  void Write(uint32_t address, uint32_t data);
  void PrintStats() const; // Miss / Hit rate
private:
  std::array<CacheSet, CACHE_SETS> sets;
  MainMemory* mainMemory = nullptr;

  uint64_t global_clock = 0;
  uint64_t hits = 0;
  uint64_t misses = 0;

  uint64_t xor_compressed_lines = 0;
};