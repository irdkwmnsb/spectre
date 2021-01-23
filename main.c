//
// Created by me on 22.01.2021.
//


#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "spectre.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <data> [<output>]\n", argv[0]);
    return -1;
  }
  char *data = argv[1];
  bool from_file = false;
  if (argc == 3) {
    from_file = true;
    output = fopen(argv[2], "w");
    if (output == NULL) {
      perror("Cannot open file");
      return -1;
    }
  } else {
    output = stdout;
  }
  init_spectre();
  char *read = read_byte_string_at((uint64_t) data);
  fprintf(output, "Read: %s\n", read);
  if(strlen(read) != strlen(data)) {
    fprintf(output, "Lengths mismatch! Attack failed!\n");
  }
  free(read);
  if (from_file) {
    if (!fclose(output)) {
      perror("Could not close file");
    }
  }
}