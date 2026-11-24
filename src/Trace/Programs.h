#pragma once

#include<cstddef>

float trace_read(float &memory_location);
void trace_write(float &memory_location, float updated_value);
int trace_read_int(int &memory_location);
void trace_write_int(int &memory_location, int updated_value);

void run_simple_float();
void run_int_matrix();
void run_float_matrix();
void run_random_float_matrix();
void run_image_processing();
void run_scan(size_t kb, int passes, int pattern);