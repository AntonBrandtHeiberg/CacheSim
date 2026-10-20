#include "Cache.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>

CacheSet::CacheSet() {
  //replacement.SetWays(CACHE_WAYS);
}

CacheSet::~CacheSet() {
}

CacheLine *CacheSet::Find(uint32_t tag) {
  for (uint8_t way = 0; way < CACHE_WAYS; way++) {
    if (lines[way].valid && lines[way].tag == tag) {
      return &lines[way];
    }
  }
  return nullptr;
}

/*
CacheLine *CacheSet::Replace(uint32_t tag, uint8_t *sourceData, uint64_t currentTime) {
  uint8_t to_be_replaced = 0; //replacement.GetReplacement();

  bool found_empty = false;
  // Search for empty or invalid position
  for (uint8_t way = 0; way < CACHE_WAYS; way++) {
    if (!lines[way].valid) {
      to_be_replaced = way;
      found_empty = true;
      break;
    }
  }

  // If cache is full, choose the LRU
  if (!found_empty) {
    uint64_t min_last_used = lines[0].last_used;
    for (uint8_t way = 1; way < CACHE_WAYS; way++) {
      if (lines[way].last_used < min_last_used) {
        min_last_used = lines[way].last_used;
        to_be_replaced = way;
      }
    }
  }

  lines[to_be_replaced].tag = tag;
  lines[to_be_replaced].valid = true;
  lines[to_be_replaced].last_used = currentTime;
  std::memcpy(lines[to_be_replaced].data.data(), sourceData, CACHE_LINE_SIZE);

  return &lines[to_be_replaced];
}*/

void Cache::Initialize(MainMemory *memory){
  mainMemory = memory;
}

uint32_t Cache::Read(uint32_t address) {
  global_clock++;
  AddressParts addressParts(address);

  CacheLine* line = sets[addressParts.setIndex].Find(addressParts.tag);

  if(line) { // Hit
    hits++;
    line->last_used = global_clock;
    return *reinterpret_cast<uint32_t*>(&line->data[addressParts.byteOffset]);
  } else { // Miss
    misses++;
     // Find beginning of cache line
     uint32_t lineStart = address & ~(CACHE_LINE_SIZE - 1);
     // Allocate data bffer
     std::array<uint8_t, CACHE_LINE_SIZE> buffer;
     // Read data from memory
     mainMemory->Read(lineStart, CACHE_LINE_SIZE, buffer.data());
     // Replace
     CacheLine* newLine = sets[addressParts.setIndex].Replace(addressParts.tag, buffer.data(), global_clock);

     return *reinterpret_cast<uint32_t*>(&newLine->data[addressParts.byteOffset]);
  }

  return 0;
}

void Cache::Write(uint32_t address, uint32_t data)
{
  global_clock++;
  AddressParts parts(address);

  // Find the line
  CacheLine* line = sets[parts.setIndex].Find(parts.tag);

  if (line) { // Hit
    hits++;
    line->last_used = global_clock;
    *reinterpret_cast<uint32_t*>(&line->data[parts.byteOffset]) = data;
    line->dirty = true;
  } else { // Miss, get from RAM and write over
    misses++;
    uint32_t lineStart = address & ~(CACHE_LINE_SIZE - 1);
    std::array<uint8_t, CACHE_LINE_SIZE> buffer;

    mainMemory->Read(lineStart, CACHE_LINE_SIZE, buffer.data());

    CacheLine* newLine = sets[parts.setIndex].Replace(parts.tag, buffer.data(), global_clock);
    *reinterpret_cast<uint32_t*>(&newLine->data[parts.byteOffset]) = data;
    newLine->dirty = true;
  }
}

void Cache::PrintStats() const {
  uint64_t total_access = hits + misses;
  double hit_rate = (static_cast<double>(hits) / total_access) * 100.0;

  std::cout << "\n======= STATS =======\n";
  std::cout << "Total access: " << total_access << "\n";
  std::cout << "Hits: " << hits << "\n";
  std::cout << "Misses: " << misses << "\n";
  std::cout << "Hit rate: " << hit_rate << "%\n";
  std::cout << "======= END =======\n\n";
}