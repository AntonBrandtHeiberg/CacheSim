#pragma once

#include <memory>

// Cannot put 4MB on the stack, make a pointer to an array and allocate it dynamically.
const uint32_t MAIN_MEMORY_SIZE = 4 * 1024 * 1024;  // 4MB


class MainMemory {
public:
  MainMemory();
  ~MainMemory();

  void Read(uint32_t startAdress, uint8_t size, uint8_t* destination);
  void Write(uint32_t startAdress, uint8_t size, uint8_t* source);
  void Print();

private:
  // unique so it releases memory at the end of the program
  std::unique_ptr<std::array<uint8_t, MAIN_MEMORY_SIZE>> memory;
};