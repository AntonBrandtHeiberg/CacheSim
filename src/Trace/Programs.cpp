#include"Programs.h"
#include<vector>
#include<iostream>


// Programs
void run_simple_float() {
  std::cout << "[TRACER] Running Simple Float\n";
  float a, b, c;
  trace_write(a, 21.5f);
  trace_write(b, 2.0f);

  float sum = trace_read(a) + trace_read(b);
  trace_write(c, sum);
}

void run_int_matrix() {
  std::cout << "[TRACER] Running Matrix Int\n";
  static int A[64][64];
  static int B[64][64];
  static int C[64][64];

  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < 64; j++) {
      trace_write_int(A[i][j], 20 + i + j);
      trace_write_int(B[i][j], 10 + i + j);
    }
  }

  // Multiply them:
  for (int i = 0; i < 64; i++) {
    for(int j = 0; j < 64; j++) {
      int sum = 0;
      for (int k = 0; k < 64; k++) {
        sum += trace_read_int(A[i][k]) * trace_read_int(B[k][j]);
      }
      trace_write_int(C[i][j], sum);
    }
  }
}

void run_scan(size_t kb, int passes, int pattern) {
  size_t total_elements = kb * 256; //(1 int = 4 bytes, 4 * 256 = 1KB)
  
  std::vector<int> data(total_elements);

  for (size_t i = 0; i < total_elements; i++) {
    int value_to_write = 0;

    if (pattern == 0) {
      value_to_write = 0;
    } else if (pattern == 1) {
      value_to_write = i;
    } else if (pattern == 2) {
      value_to_write = (i * 1000);
    } else {
      value_to_write = rand();
    }

    trace_write_int(data[i], value_to_write);
  }


  for (int pass = 0; pass < passes; pass++) {
    for (size_t i = 0; i < total_elements; i += 16) { //read one word per cache line
      trace_read_int(data[i]);
    }
  }
}

void run_float_matrix() {
  std::cout << "[TRACER] Running Matrix Float\n";
  static float A[64][64];
  static float B[64][64];
  static float C[64][64];

  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < 64; j++) {
      trace_write(A[i][j], 20.0f + (i * 0.05f) + (j * 0.02f));
      trace_write(B[i][j], 10.0f + (i * 0.01f) + (j * 0.03f));
    }
  }

  // Multiply them:
  for (int i = 0; i < 64; i++) {
    for(int j = 0; j < 64; j++) {
      float sum = 0.0f;
      for (int k = 0; k < 64; k++) {
        sum += trace_read(A[i][k]) * trace_read(B[k][j]);
      }
      trace_write(C[i][j], sum);
    }
  }
}

void run_random_float_matrix() {
  std::cout << "[TRACER] Running Random Matrix Float\n";
  static float A[64][64];
  static float B[64][64];
  static float C[64][64];

  for (int i = 0; i < 64; i++) {
    for (int j = 0; j <64; j++) {
      float rand_a = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 100.0f;
      float rand_b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 100.0f;
      trace_write(A[i][j], rand_a);
      trace_write(B[i][j], rand_b);
    }
  }

  // Multiply them:
  for (int i = 0; i < 64; i++) {
    for(int j = 0; j < 64; j++) {
      float sum = 0.0f;
      for (int k = 0; k < 64; k++) {
        sum += trace_read(A[i][k]) * trace_read(B[k][j]);
      }
      trace_write(C[i][j], sum);
    }
  }
}

void run_image_processing() {
  std::cout << "[TRACER] Running Image Blur\n";
  static int InputImage[64][64];
  static int OutputImage[64][64];

  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < 64; j++) {
      int pixel_value = 100 + (i / 2) + (j / 3);
      trace_write_int(InputImage[i][j], pixel_value % 256);
    }
  }

  // Blur
  for (int i = 1; i < 63; i++) {
    for (int j = 1; j < 63; j++) {
      int sum = 0;
      // Read surrounding pixels
      for (int di = -1; di <= 1; di++) {
        for (int dj = -1; dj <= 1; dj++) {
          sum += trace_read_int(InputImage[i + di][j + dj]);
        }
      }
      trace_write_int(OutputImage[i][j], sum / 9);
    }
  }
}
