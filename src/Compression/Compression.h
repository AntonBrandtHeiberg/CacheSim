#pragma once
#include<bitset>

static int signed_bytes(uint32_t v) {
  int32_t s = static_cast<int32_t>(v);
  if (s == 0) return 0;
  if (s >= -128 && s <= 127) return 1;
  if (s >= -32768 && s <= 32767) return 2;
  return 4;
}

static int zero_repeat_size(const uint32_t *w) {
  bool zero = true, rep = true; //two edge cases
  for (int i = 0; i < 16; i++) {
    if (w[i] != 0) {
      zero = false;
    } 
    if (w[i] != w[0]) {
      rep = false;
    }
    if (!zero && !rep) {
      break;
    }
  }

  if (zero) return 1; // We are just storing a zero, 1 byte
  if (rep) return 4; // Repeated words
  return 64;
}

static int bdi_size(const uint32_t *w) {
  int size = 4; // Base word size
  
  for (int i = 1; i < 16; i++) {
    // FIrst Delta
    int32_t delta = static_cast<int32_t>(w[i] - w[0]);
    
    size += signed_bytes(delta); // 0, 1, 2, or 4
  }
  
  int metadata_bytes = 4; // BDI needs ~4 bytes of tags. Need to store the size of every delta (4 possible) that means 2 bits * max 15 = 30 bits rounding up to nearest byte is 4 bytes
  return std::min(size + metadata_bytes, 64);
}

static int dxor_size(const uint32_t *w) { // Delta 
  const int BLK = 4; // 4 32-bit words per block, performance might cahnge if different
  const int blocks = 16 / BLK;

  // Base word and 3 deltaXOR words + 2-bit tag to remember original size
  int size = (blocks * (BLK - 1) * 2 + 7) / 8; //rounded
  
  for (int b = 0; b < blocks; b++) { // Loop inside the 4 blocks
    const uint32_t *p = w + (b * BLK); //points to the start of the 16 words, moves forward
    size += 4; // Store base word
    
    uint32_t prev_d = p[1] - p[0]; // First Delta
    size += signed_bytes(prev_d);
    
    for (int i = 2; i < BLK; i++) { // For the other two
      uint32_t d = p[i] - p[i - 1];
      size += signed_bytes(d ^ prev_d); // The secret sauce
      prev_d = d;
    }
  }
  return std::min(size, 64);
}

// Upgraded DXORV2, don't separate into blocks, smaller metadata
static int dxor_v2_size(const uint32_t* w) {
  // Base word
  int size = 4; 
  
  // First delta
  uint32_t prev_d = w[1] - w[0];
  size += signed_bytes(prev_d); 
  
  // Find MAX byte size for the XOR
  int max_xor_bytes = 0;
  for (int i = 2; i < 16; i++) {
    uint32_t d = w[i] - w[i - 1];
    int current_bytes = signed_bytes(d ^ prev_d);
    if (current_bytes > max_xor_bytes) {
      max_xor_bytes = current_bytes;
    }
    prev_d = d;
  }
  
  size += (14 * max_xor_bytes);

  size += 1; //tag
  
  return std::min(size, 64);
}

// Old and retired this from the tests (bad performance)
// From FB, For real time database compression (delta of delta).
static int gorilla_size(const uint32_t* w) {
  int bits = 32;
  uint32_t prev_d = 0; ;

  for (int i = 1; i < 16; i++) {
    uint32_t d  = w[i] - w[i - 1];
    int32_t  dd = (int32_t)(d - prev_d);
    prev_d = d;
    if (dd == 0) bits += 1;
    else if (dd >= -63   && dd <= 64) bits += 2 + 7;
    else if (dd >= -255  && dd <= 256) bits += 3 + 9;
    else if (dd >= -2047 && dd <= 2048) bits += 4 + 12;
    else bits += 4 + 32;
  }
  
  return std::min((bits + 7) / 8, 64);
}

// BPC-Lite
// Takes delta, bit-plane transpose, XOR neighbouring planes, compress the planes
// Delta between words and then vertically lines them up to form planes
// Mantissa hopefully similar so XOR can take care for them
static int bpc_lite_size(const uint32_t* w) {
  const int NUM_DELTAS = 15;
  const int NUM_PLANES = 33;
  const uint16_t ALL_ONES = (uint16_t) 0x7FFF;

  // Delta between 32-bit words
  uint64_t deltas[NUM_DELTAS];
  for (int i = 0; i < NUM_DELTAS; i++) {
    int64_t diff = (int64_t) w[i + 1] - (int64_t) w[i];

    //keep lower 33 bits
    const uint64_t MASK_33 = 0x1FFFFFFFF;
    deltas[i] = (uint64_t) diff & MASK_33;
  }

  // 33 vertical planes from 15 horizontal delta
  uint16_t planes[NUM_PLANES] = {0};

  for (int p = 0; p < NUM_PLANES; p++) {
    std::bitset<16> current_plane_bits(0);

    for (int i = 0; i < NUM_DELTAS; i++) {
      std::bitset<64> delta_bits(deltas[i]);

      if (delta_bits.test(p)) { //tests if bit at pos p is 1
        current_plane_bits.set(i); // Set to 1
      }
    }

    planes[p] = static_cast<uint16_t>(current_plane_bits.to_ulong());
  }

  // XOR all planes
  int bits = 32;
  int zero_planes_in_a_row = 0;
  uint16_t plane_above = 0;

  for (int p = NUM_PLANES - 1; p >= 0; p--) {
    uint16_t xor_plane = planes[p] ^ plane_above;
    plane_above = planes[p]; //save the plane itself, not the XOR
    
    if (xor_plane == 0) { // Count the identical planes
      zero_planes_in_a_row++;
    } else {
      // Here are the 4 patterns BPC can compress, zero planes, all ones, a single one
      // 2 bits to store what pattern was used
      // 6 bits are for counting #zero planes, between 1 and 33
      if (zero_planes_in_a_row > 0) {
        bits += 2 + 6; // Prefix and #zero planes
        zero_planes_in_a_row = 0; // Stop counting identical planes
      }

      if (xor_plane == ALL_ONES) {
        bits += 2;
      } else if (__builtin_popcount(xor_plane) == 1) { //popcount checks # of 1's
        bits += 2 + 4; //1 + the single ones its position
      } else {
        bits += 2 + NUM_DELTAS; // Uncompressed. Raw plane, 2 bit tag + 15 data bits
      }
    }
  }

  if (zero_planes_in_a_row > 0) {
    bits += 2 + 6; // Count last one aswell
  }

  return std::min((bits + 7) / 8, 64); // Round up to nearest byte
}

int Final_Size(CacheMode mode, const uint8_t* data) {
  if (mode == CacheMode::STANDARD) return CACHE_LINE_SIZE;
  const uint32_t* w = reinterpret_cast<const uint32_t*>(data);

  int size = zero_repeat_size(w);

  if (size == 64) { // Meaning not easily compressed
    if (mode == CacheMode::BDI) size = bdi_size(w);
    else if (mode == CacheMode::DXOR) size = dxor_size(w);
    else if (mode == CacheMode::DXORV2) size = dxor_v2_size(w);
    else if (mode == CacheMode::BPC) size = bpc_lite_size(w);
  }

  // Round up to 8 byte segment
  const int SEG = 8;
  int final_size = std::min(static_cast<int>(CACHE_LINE_SIZE), ((std::max(size, 1) + SEG - 1) / SEG) * SEG);
  //debug
  //std::cout << "Raw size: " << size << " -> Segmented size: " << final_size << "\n";

  return final_size;
}
