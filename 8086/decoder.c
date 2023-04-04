#include "stdlib.h"
#include "stdio.h"
#include "string.h"

typedef uint8_t u8;

// byte 1 format
#define MASK_OPCODE 0b11111100 // 6 bits
#define MASK_D      0b00000010 // 1 bit
#define MASK_W      0b00000001 // 1 bit

// byte 2 format
#define MASK_MOD    0b11000000 // 2 bits
#define MASK_REG    0b00111000 // 3 bits
#define MASK_RM     0b00000111 // 3 bits

#define OPCODE_MOV  0b10001000

// REG (register) field encoding
char namesByte[8][2] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
char namesWord[8][2] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

char test[2][8][2] = {{"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"}, {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}};

int main(int argc,  char* argv[argc + 1]) {
  if (argc < 2) {
    return EXIT_FAILURE;
  }

  FILE* input = fopen(argv[1], "rb");
  if (!input) {
    perror("fopen for input file failed");
    return EXIT_FAILURE;
  }

    printf("bits 16\n\n");
    // loop for the data inside input loop
    u8 data[2];
    do {
      size_t read_size = fread(&data, sizeof(u8), 2, input);
      if (read_size == 2) {
        u8 inst = data[0] & MASK_OPCODE;
          if (inst == OPCODE_MOV) {
            printf("mov ");
            u8 d = (data[0] & MASK_D) >> 1;
            u8 w = data[0] & MASK_W;

            u8 mod = (data[1] & MASK_MOD) >> 6;
            u8 reg = (data[1] & MASK_REG) >> 3;
            u8 rm = data[1] & MASK_RM;

            if (mod == 3) {
              printf("%c%c, %c%c", test[w][rm][0], test[w][rm][1], test[w][reg][0], test[w][reg][1]);
            }

            printf("\n");
        }
      }
    } while (!feof(input));

  fclose(input);

  return EXIT_SUCCESS;
}

