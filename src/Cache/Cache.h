#pragma once

#include "Memory/MainMemory.h"
#include <array>
#include <string_view>
#include <cstdint>
#include <string>

#ifndef SETS
#define SETS 64
#endif
#ifndef TAGF
#define TAGF 2
#endif

//const uint8_t MEMORY_ADRESS_SIZE = 32;  // 32-bit memory adresses
const uint8_t CACHE_LINE_SIZE = 64;     // 64-byte cache lines
const uint32_t CACHE_SETS = SETS;
const uint8_t CACHE_WAYS = 16;          // 16-way associative
const uint32_t TAG_FACTOR = TAGF;
const uint32_t MAX_TAGS = CACHE_WAYS * TAG_FACTOR;   // CACHE_WAYS stays as is

const uint8_t CACHE_LINE_BYTE_OFFSET_SIZE = 6; // 2^6 = 64 one for each 
const uint8_t CACHE_LINE_SET_INDEX_SIZE = 6; // 2^6 = 64 one for each set
const uint8_t CACHE_LINE_TAG = 20;

constexpr uint32_t log2u(uint32_t v) {
  uint32_t n = 0;
  while (v >>= 1){
    ++n;
  }
  
  return n;
}

constexpr uint32_t OFFSET_BITS = log2u(CACHE_LINE_SIZE);
constexpr uint32_t INDEX_BITS  = log2u(CACHE_SETS);

enum class CacheMode {
  STANDARD = 0,
  BDI = 1,
  BPC = 2,
  DXOR = 3,
  DXORV2 = 4
};

struct CacheLine {
  uint32_t addr = 0; //for write back
  uint32_t tag = 0;
  std::array<uint8_t, CACHE_LINE_SIZE> data = {0};
  bool dirty = false;
  bool valid = false;
  uint64_t last_used = 0;
  int comp_size = 0; // IF this was compressed, how many bytes would it be?
};

struct AddressParts {
  uint32_t tag;
  uint32_t setIndex;
  uint8_t byteOffset;

  AddressParts(uint32_t a) {
    byteOffset = a & (CACHE_LINE_SIZE - 1);
    setIndex = (a >> OFFSET_BITS) & (CACHE_SETS - 1);
    tag = a >> (OFFSET_BITS + INDEX_BITS);
  }
};

class CacheSet {
public:
  CacheSet();
  ~CacheSet();
  int used_bytes = 0;
  std::array<CacheLine, MAX_TAGS> lines;
  CacheLine* Find(uint32_t tag);
  CacheLine* Replace(uint32_t tag, uint32_t lineAddr, const uint8_t *src, uint64_t now, CacheMode mode, MainMemory* mem);
  void Resize(CacheLine* line, CacheMode mode, MainMemory *mem);

private:
  int CountValid() const;
  bool EvictLRU(MainMemory *mem, const CacheLine* keep);
};

class Cache {
public:
  void Initialize(MainMemory *memory);
  uint32_t Read(uint32_t address);
  void Write(uint32_t address, uint32_t data);
  void PrintStats(const std::string &workload_name) const; // Miss / Hit rate
  static constexpr std::string_view ModeToString(CacheMode mode);
  CacheMode current_mode = CacheMode::STANDARD;

private:
  std::array<CacheSet, CACHE_SETS> sets;
  MainMemory* mainMemory = nullptr;

  uint64_t global_clock = 0;
  uint64_t hits = 0;
  uint64_t misses = 0;
  //uint64_t compressed_lines = 0;
  //uint64_t comp_counter = 0;
};