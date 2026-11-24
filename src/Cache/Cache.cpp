#include "Cache.h"
#include "Compression/Compression.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <string_view>

#include <fstream>

//constexpr int COMP_THRESH = 16; // Arbitrary and could change performance

CacheSet::CacheSet() {
  //replacement.SetWays(CACHE_WAYS);
}

CacheSet::~CacheSet() {
}

CacheLine *CacheSet::Find(uint32_t tag) {
  for (auto& line : lines) {
    if (line.valid && line.tag == tag) {
      return &line;
    }
  }
  return nullptr;
}

int CacheSet::CountValid() const {
  int n = 0;
  for (int way = 0; way < MAX_TAGS; way++) {
    if (lines[way].valid) n++;
  }
  return n;
}

bool CacheSet::EvictLRU(MainMemory* mem, const CacheLine* keep) { // Evicts based on the theoretical limit, not actual limit
  CacheLine *v = nullptr;
  for (int way = 0; way < MAX_TAGS; way++) {
    CacheLine &l = lines[way];
    if (l.valid && &l != keep && (!v || l.last_used < v->last_used)) { // Can't evict the line we just wrote, look for LRU
      v = &l;
    }
  }
  if (!v) return false;
  
  // Write back if dirty
  if (v->dirty) {
    mem->Write(v->addr, CACHE_LINE_SIZE, v->data.data()); 
  }
  
  used_bytes -= v->comp_size;
  v->valid = false;
  return true;
}

CacheLine *CacheSet::Replace(uint32_t tag, uint32_t lineAddr, const uint8_t* src, uint64_t now, CacheMode mode, MainMemory* mem) {
  int need = Final_Size(mode, src);
  int tag_limit = (mode == CacheMode::STANDARD) ? CACHE_WAYS : MAX_TAGS;
  const int budget = CACHE_WAYS * CACHE_LINE_SIZE;

  // Evict until have enough budget (theoretical)
  while (CountValid() >= tag_limit || used_bytes + need > budget) {
    if (!EvictLRU(mem, nullptr)) break;
  }

  CacheLine* s = nullptr;
  for (int way = 0; way < MAX_TAGS; way++) {
    if (!lines[way].valid) {
      s = &lines[way];
      break;
    }
  }

  s->tag = tag;
  s->addr = lineAddr;
  s->valid = true;
  s->dirty = false;
  s->last_used = now;
  s->comp_size = need;

  used_bytes += need;
  std::memcpy(s->data.data(), src, CACHE_LINE_SIZE);

  return s;
}

void CacheSet::Resize(CacheLine* line, CacheMode mode, MainMemory* mem) {
  const int new_size = Final_Size(mode, line->data.data()); // Re-calculate line size
  
  used_bytes += (new_size - line->comp_size);
  line->comp_size = new_size;
  
  // Run until we have space for the new one!
  const int max_set_bytes = CACHE_WAYS * CACHE_LINE_SIZE;
  while (used_bytes > max_set_bytes) {
    if (EvictLRU(mem, line)) break;
  }
}

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
    
    //find beginning of cache line
    uint32_t lineStart = address & ~(CACHE_LINE_SIZE - 1);
    //allocate data  bffer
    std::array<uint8_t, CACHE_LINE_SIZE> buffer{};
    //read data from memory
    mainMemory->Read(lineStart, CACHE_LINE_SIZE, buffer.data());
    //replace
    CacheLine *newLine = sets[addressParts.setIndex].Replace(addressParts.tag, lineStart, buffer.data(), global_clock, current_mode, mainMemory);
    
    return *reinterpret_cast<uint32_t*>(&newLine->data[addressParts.byteOffset]);
  }

  return 0;
}

void Cache::Write(uint32_t address, uint32_t data)
{
  global_clock++;
  AddressParts parts(address);
  
  //find the line
  CacheLine* line = sets[parts.setIndex].Find(parts.tag);

  if (line) { // Hit
    hits++;
    line->last_used = global_clock;
    *reinterpret_cast<uint32_t*>(&line->data[parts.byteOffset]) = data;
    line->dirty = true;
    sets[parts.setIndex].Resize(line, current_mode, mainMemory);
  } else { // Miss, get from RAM and write over
    misses++;
    uint32_t lineStart = address & ~(CACHE_LINE_SIZE - 1); //last bits
    std::array<uint8_t, CACHE_LINE_SIZE> buffer{};

    mainMemory->Read(lineStart, CACHE_LINE_SIZE, buffer.data());
    
    CacheLine *newLine = sets[parts.setIndex].Replace(parts.tag, lineStart, buffer.data(), global_clock, current_mode, mainMemory);
    
    *reinterpret_cast<uint32_t*>(&newLine->data[parts.byteOffset]) = data;
    newLine->dirty = true;
    // Resize the line since we overwrote a word
    sets[parts.setIndex].Resize(newLine, current_mode, mainMemory);
  }
}

constexpr std::string_view Cache::ModeToString(CacheMode mode){
  switch (mode) {
    case CacheMode::STANDARD: return "Standard";
    case CacheMode::BDI: return "BDI";
    case CacheMode::BPC: return "BPC (Lite)";
    case CacheMode::DXOR: return "Delta XOR";
    case CacheMode::DXORV2: return "Delta XOR V2";
  }
}

void Cache::PrintStats(const std::string &workload_name) const {
  uint64_t valid = 0, bytes = 0, le32 = 0, le16 = 0;
  
  for (const auto& set : sets) {
    for (int way = 0; way < MAX_TAGS; way++) {
      if (set.lines[way].valid) {
        valid++; 
        bytes += set.lines[way].comp_size;
        if (set.lines[way].comp_size <= 32) le32++;
        if (set.lines[way].comp_size <= 16) le16++;
      }
    }
  }

  double hit_rate = (hits + misses > 0) ? (static_cast<double>(hits) / (hits + misses) * 100.0) : 0;
  double avg_size = valid ? static_cast<double>(bytes) / valid : 0;
  double ratio = bytes ? static_cast<double>(valid * CACHE_LINE_SIZE) / bytes : 0;
  double eff_cap = static_cast<double>(valid) / (CACHE_SETS * CACHE_WAYS);
  
  double pct_le32 = valid ? (static_cast<double>(le32) / valid * 100.0) : 0;
  double pct_le16 = valid ? (static_cast<double>(le16) / valid * 100.0) : 0;

  std::cout << "\n=== " << ModeToString(current_mode) << " [" << workload_name << "] ===\n";
  std::cout << "Hit rate: " << hit_rate << "%\n";
  std::cout << "Avg Size: " << avg_size << " B\n";
  std::cout << "Comp Ratio: " << ratio << "x\n";
  std::cout << "Eff Capacity: " << eff_cap << "x\n";

  std::ofstream csv("results.csv", std::ios::app);
  if (csv.is_open()) {
    // Columns: Workload,Algorithm,Hit_Rate,Avg_Size,Ratio,Pct_le32,Pct_le16,Eff_Capacity
    csv << workload_name << ","
    << CACHE_SETS << ","
    << TAG_FACTOR << ","
    << ModeToString(current_mode) << ","
    << hit_rate << "," 
    << avg_size << "," 
    << ratio << "," 
    << pct_le32 << "," 
    << pct_le16 << "," 
    << eff_cap << "\n";
  }
}