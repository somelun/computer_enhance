#ifndef DECODE_DATA_H
#define DECODE_DATA_H

struct decodeData {
  FILE* input;
};

bool dd_init_with_file(struct decodeData* dd, const char* file_name) {
  dd->input = fopen(file_name, "rb");
  if (!dd->input) {
    perror("fopen for input file failed\n");
    return false;
  }

  return true;
}

void dd_deinit(struct decodeData* dd) {
  fclose(dd->input);
}

size_t dd_read_byte() {
  return 0;
}

size_t dd_read_word() {
  return 0;
}

#endif // DECODE_DATA_H

