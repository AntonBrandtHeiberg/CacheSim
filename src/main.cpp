#include "Memory/MainMemory.h"
#include "Cache/Cache.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

void TraceLine(Cache *cache, std::string &line) {
  std::istringstream iss(line); //string stream

  char op;
  uint32_t address = 0, data = 0;

  iss >> op >> std::hex >> address; //break up into vars

  if (op == 'W') {
    iss >> std::hex >> data;
    cache->Write(address, data);
  } else if (op == 'R' ) {
    cache->Read(address);
  }
}

void RunSimulation(CacheMode mode, const std::string &trace_filename, const std::string &name) {
  auto cache = std::make_unique<Cache>(); //smart pointer to heap
  MainMemory ram;
  cache->Initialize(&ram);
  cache->current_mode = mode;

  std::ifstream traceFile(trace_filename); //opne file
  std::string line;

  while (std::getline(traceFile, line)) {
    TraceLine(cache.get(), line);
  }

  cache->PrintStats(name);
}

int main(int argc, char** argv) {
  std::cout << "[MAIN] Starting simulation ... \n";

  std::string file = argc > 1 ? argv[1] : "trace.txt";
  std::string name = argc > 2 ? argv[2] : "unknown";

  const CacheMode modes[] = {
    CacheMode::STANDARD,
    CacheMode::BDI,
    CacheMode::BPC,
    CacheMode::DXOR,
    CacheMode::DXORV2
  };

  for (CacheMode mode : modes) {
    RunSimulation(mode, file, name);
  }
  
  return 0;
}