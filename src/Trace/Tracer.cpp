#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "Programs.h"
#include "Programs.cpp"

#include "Cache/Cache.h"

std::ofstream traceFile;
uint64_t base_address = 0;
bool first_access = true;

// Floats
float trace_read(float &memory_location) {
  uint64_t raw_address = (uint64_t) &memory_location;

  if (first_access) {
    base_address = raw_address - 0x20000; //marginal
    first_access = false;
  }

  uint32_t sim_address = (uint32_t)(raw_address - base_address);
  traceFile << "R 0x" << std::hex << sim_address << "\n";
  return memory_location;
}

void trace_write(float &memory_location, float updated_value) {
  uint64_t raw_address = (uint64_t) &memory_location;
    
  if (first_access) {
    base_address = raw_address - 0x20000;
    first_access = false;
  }

  uint32_t sim_address = (uint32_t) (raw_address - base_address);

  unsigned int bits = *reinterpret_cast<unsigned int*>(&updated_value);

  traceFile << "W 0x" << std::hex << sim_address << " 0x" << std::hex << bits << "\n";
  memory_location = updated_value;
}

// Ints
int trace_read_int(int &memory_location) {
  uint64_t raw_address = (uint64_t) &memory_location;

  if (first_access) {
    base_address = raw_address - 0x20000; //marginal
    first_access = false;
  }

  uint32_t sim_address = (uint32_t)(raw_address - base_address);
  traceFile << "R 0x" << std::hex << sim_address << "\n";
  return memory_location;
}

void trace_write_int(int &memory_location, int updated_value) {
  uint64_t raw_address = (uint64_t) &memory_location;
    
  if (first_access) {
    base_address = raw_address - 0x20000;
    first_access = false;
  }

  uint32_t sim_address = (uint32_t) (raw_address - base_address);

  traceFile << "W 0x" << std::hex << sim_address << " 0x" << std::hex << updated_value << "\n";
  memory_location = updated_value;
}

int main(int argc, char** argv) {
  std::string w = (argc > 1) ? argv[1] : "blur";
  std::string filename = "traces/trace_" + w + ".txt";

  traceFile.open(filename);
  
  if (w == "blur") run_image_processing();
  else if (w == "float_matrix") run_float_matrix();
  else if (w == "int_matrix") run_int_matrix();
  else if (w == "random_float") run_random_float_matrix();
  else if (w == "scan_zero") run_scan(48, 20, 0);
  else if (w == "scan_ramp") run_scan(48, 20, 1);
  else if (w == "scan_stride") run_scan(48, 20, 2);
  else if (w == "scan_random") run_scan(48, 20, 3);
  else { return 0; }
}