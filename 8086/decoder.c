#include "stdlib.h"
#include "stdio.h"
#include "string.h"

typedef uint8_t  u8;
typedef int8_t   i8;
typedef int16_t  i16;
typedef uint16_t u16;

// byte 1 format
#define MASK_D      0b00000010 // 1 bit
#define MASK_W      0b00000001 // 1 bit

// byte 2 format
#define MASK_MOD    0b11000000 // 2 bits
#define MASK_REG    0b00111000 // 3 bits
#define MASK_RM     0b00000111 // 3 bits
                               //
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 

// REG (register) field encoding when MOD = 11
char registers[2][8][2] = {
  {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
  {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}
};

char* effective_address[] = {
  "bx+si",
  "bx+di",
  "bp+si",
  "bp+di",
  "si",
  "di",
  "bp", // when MOD = 00 this will be direct address
  "bx"
};

size_t read_byte(FILE* input, u8* buffer) {
  return fread(buffer, sizeof(u8), 1, input);
}

size_t read_word(FILE* input, u16* buffer) {
  return fread(buffer, sizeof(u16), 1, input);
}

size_t read_signed(FILE* input, size_t buffer_size, i16* buffer) {
  size_t read_size = fread(buffer, buffer_size, 1, input);
  // if it is a negative 8 bit number we need to manually make
  // it negative 16 bit number
  if ((*buffer >> 7) == 0b000000001) {
    *buffer |= 0b1111111100000000;
  }
  return read_size;
}

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

    // loop for the buffer inside input loop
    u8 buffer;
    while (read_byte(input, &buffer)) {

      if (buffer >> 2 == 0b100010) { // register/memory to/from register
        u8 w = buffer & MASK_W;
        u8 d = (buffer & MASK_D) >> 1;

        read_byte(input, &buffer);

        u8 mod = (buffer & MASK_MOD) >> 6;
        u8 reg = (buffer & MASK_REG) >> 3;
        u8 rm = buffer & MASK_RM;

        switch (mod) {
          case 0b00: {
            if (rm == 0b110) {
              i16 data;
              read_signed(input, sizeof(i16), &data);
              printf("mov %c%c, [%d]\n", registers[w][reg][0],
                                        registers[w][reg][1],
                                        data);

            } else {
              if (d) {
                printf("mov %c%c, [%s]\n", registers[w][reg][0],
                                           registers[w][reg][1],
                                           effective_address[rm]);
              } else {
                printf("mov [%s], %c%c\n", effective_address[rm],
                                           registers[w][reg][0],
                                           registers[w][reg][1]);
              }
            }
            break;
          }
          case 0b01: {
            i16 disp = 0;
            read_signed(input, sizeof(i8), &disp);
            if (d) {
              printf("mov %c%c, [%s%-d]\n", registers[w][reg][0],
                                         registers[w][reg][1],
                                         effective_address[rm],
                                         disp);
            } else {
              printf("mov [%s%-d], %c%c\n", effective_address[rm],
                                         disp,
                                         registers[w][reg][0],
                                         registers[w][reg][1]);
            }
            break;
          }
          case 0b10: {
            i16 disp = 0;
            read_signed(input, sizeof(i16), &disp);

            if (d) {
              printf("mov %c%c, [%s%-d]\n", registers[w][reg][0],
                                         registers[w][reg][1],
                                         effective_address[rm],
                                         disp);
            } else {
              printf("mov [%s%-d], %c%c\n", effective_address[rm],
                                         disp,
                                         registers[w][reg][0],
                                         registers[w][reg][1]);
            }
            break;
          }
          case 0b11: {
            printf("mov %c%c, %c%c\n", registers[w][rm][0], registers[w][rm][1],
                                       registers[w][reg][0], registers[w][reg][1]);
            break;
          }
        }
      } else if (buffer >> 1 == 0b1100011) { // immediate to register/memory
        u8 w = buffer & MASK_W;

        read_byte(input, &buffer);

        u8 mod = (buffer & MASK_MOD) >> 6;
        u8 rm = buffer & MASK_RM;

        i16 disp = 0;
        if (mod == 0b11) { // register mode
          //
        } else { // memory mode
          if (mod == 0b01) { // 8 bit displacement;
            read_signed(input, sizeof(i8), &disp);
          } else if (mod == 0b10 || (mod == 0b00 && rm == 0b110)) { // 16 bit displacement
            read_signed(input, sizeof(i16), &disp);
          }
        }

        if (w == 0) {
          u8 data;
          read_byte(input, &data);
          printf("mov [%s + %d], byte %d\n", effective_address[rm], disp, data);
        } else {
          u16 data;
          read_word(input, &data);
          printf("mov [%s + %d], word %d\n", effective_address[rm], disp, data);
        }

      } else if (buffer >> 4 == 0b1011) { // immediate to register
        u8 reg = buffer & 0b111;
        u8 w = buffer >> 3 & 1;

        if (w == 1) {
          u16 data;
          read_word(input, &data);
          printf("mov %c%c, %d\n", registers[w][reg][0], registers[w][reg][1], data);
        } else {
          u8 data;
          read_byte(input, &data);
          printf("mov %c%c, %d\n", registers[w][reg][0], registers[w][reg][1], data);
        }
      } else if (buffer >> 2 == 0b101000) {  // memory/accumulator to accumulator/memory
        u8 d = (buffer & MASK_D) >> 1;
        i16 data;
        read_signed(input, sizeof(i16), &data);
        if (d == 0) { // memory to accumulator
          printf("mov ax, [%d]\n", data);
        } else { // accumulator to memory
          printf("mov [%d], ax\n", data);
        }
      } else {
        printf("%d\n", buffer);
      }

    }

  fclose(input);

  return EXIT_SUCCESS;
}

