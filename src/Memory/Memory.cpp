#include "MainMemory.h"

#include <array>
#include <string>
#include <iostream>
#include <iomanip>

MainMemory::MainMemory() {
  // put on heap
  memory = std::make_unique<std::array<uint8_t, MAIN_MEMORY_SIZE>>();
  memory->fill(0x00);
}

MainMemory::~MainMemory() {
}

void MainMemory::Read(uint32_t startAdress, uint8_t size, uint8_t *destination){
  //std::array<uint8_t, MAIN_MEMORY_SIZE>* rawArrayPtr = memory.get(); //get pointer from smart object
  //uint8_t& byteValue = rawArrayPtr->at(startAdress); //
  //uint8_t* pointerToElement = &byteValue;
  std::memcpy(destination, &memory.get()->at(startAdress), size);
}

void MainMemory::Write(uint32_t startAdress, uint8_t size, uint8_t *source){
  std::memcpy(&memory->at(startAdress), source, size);
}

void MainMemory::Print() {
  std::cout << " --- NON-ZERO RAM MEMORY ---\n";
  for (uint32_t i = 0; i < MAIN_MEMORY_SIZE; i += 4) { //print full words
    bool nonZero = ((*memory)[i] != 0 || (*memory)[i+1] != 0 || (*memory)[i+2] != 0 || (*memory)[i+3] != 0);

    if (nonZero) {
      std::cout << "Address 0x" << std::setfill('0') << std::setw(8) << std::hex << i << ": ";
      std::cout << std::setw(2) << (int)(*memory)[i] << " "
                << std::setw(2) << (int)(*memory)[i+1] << " "
                << std::setw(2) << (int)(*memory)[i+2] << " "
                << std::setw(2) << (int)(*memory)[i+3] << "\n";
    }
  }
  std::cout << " --- NON-ZERO RAM MEMORY ---\n";
}