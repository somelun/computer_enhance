#ifndef MEMORY_H_
#define MEMORY_H_

#include "stdio.h"
#include "stdbool.h"

#include "types.h"

struct memory {
  u8* data;
  size_t size;
  size_t position;
};

static bool init_from_file(struct memory* m, char* file_name);
static size_t read_byte_(struct memory* m, u8* output);
static size_t read_word_(struct memory* m, u16* output);


bool init_from_file(struct memory* m, char* file_name) {
  FILE* input = fopen(file_name, "rb");
  if (!input) {
    perror("fopen for input file failed\n");
    return false;
  }

  fseek(input, 0L, SEEK_END);
  m->size = ftell(input);
  rewind(input);

  if (m->size < 1) {
    printf("%s was empty, nothing to do\n", file_name);
    fclose(input);
    return false;
  }

  m->data = (u8*)malloc(m->size * sizeof(u8*));
  fread(m->data, m->size, 1, input);

  fclose(input);

  return true;
}

size_t read_byte_(struct memory* m, u8* output) {
  // &output = m->data[m->position];
  // return 0;
  // return read(output, sizeof(u8), 1, m->data);
  return 0;
}

size_t read_word_(struct memory* m, u16* output) {
  return 0;
}

#endif // #define MEMORY_H_

