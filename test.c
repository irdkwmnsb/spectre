//
// Created by me on 22.01.2021.
//

#include "spectre.h"

int main(int argc, char *argv[]) {
  if (argc > 1) {
    fprintf(stderr, "%s ignores arguments\n", argv[0]);
  }
//char *secret =
//    "Привет мир! Эту память читают напрямую!";
  char *secret =
      "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
//char *secret =
//    "MyPasswordMyPasswordMyPasswordMyPasswordMyPasswordMyPasswordMyPasswordMyPasswordMyPasswordMyPassword";
//  usleep(500);
  output = stderr;
  uint32_t correct = 0;
  uint32_t incorrect = 0;
  uint32_t repeat = 20;
  size_t size = strlen(secret);
  uint8_t byte;
  uint64_t addr = (uint64_t) secret;
  init_spectre();
  printf("Reading...\n");
  fflush(stdout);
  for (int i = 0; i < repeat; i++) {
    uint32_t read = 0;
    do {
      byte = read_at(addr + read);
      if (byte == secret[read]) {
        correct++;
      } else {
        printf("expected %c found 0x%02X (%c)\n", secret[read], byte, byte);
        incorrect++;
      }
      printf("%c", byte);
      read++;
    } while (read < size);
    printf("\n");
    fflush(stdout);
  }
  printf("percent=%u\n", (correct * 100) / (correct + incorrect));
  printf("errors=%u\n", incorrect);
  printf("total read=%u\n", incorrect + correct);
  return 0;
}