#include "MainMemory.h"
#include "Cache.h"
#include <iostream>

int main() {
  MainMemory ram;
  Cache cache;
  cache.Initialize(&ram);

  std::cout << "Test start\n ";
  cache.Write(0x1000, 0x12345678);
  cache.Read(0x1000);
  cache.Read(0x1004);
  cache.Read(0x5000);

  cache.PrintStats();

  return 0;
}