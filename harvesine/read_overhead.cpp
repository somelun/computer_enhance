#include <stdlib.h>
#include <sys/stat.h>

#include "repetition_tester.hpp"

#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

struct Buffer {
    size_t count;
    u8 *data;
};

static Buffer allocate_buffer(size_t count) {
  Buffer result = {};
  result.data = (u8*)malloc(count);
  if (result.data) {
    result.count = count;
  } else {
    fprintf(stderr, "ERROR: Unable to allocate %lu bytes.\n", count);
  }

  return result;
}

static void free_buffer(Buffer *buffer) {
  if (buffer->data) {
    free(buffer->data);
  }

  buffer->count = 0;
  buffer->data = nullptr;
}

struct ReadParameters {
  Buffer destination;
  const char* file_name;
};

typedef void read_overhead_test_func(RepTester *tester, ReadParameters *params);

struct TestFunction {
  const char *name;
  read_overhead_test_func *func;
};

static void read_with_fread(RepTester *tester, ReadParameters *params) {
  while (is_testing(tester)) {
    FILE *file = fopen(params->file_name, "rb");
    if (file) {
      Buffer dest_buffer = params->destination;

      begin_time(tester);
      size_t result = fread(dest_buffer.data, dest_buffer.count, 1, file);
      end_time(tester);

      if (result == 1) {
        count_bytes(tester, dest_buffer.count);
      } else {
        error(tester, "fread failed");
      }

      fclose(file);
    } else {
      error(tester, "fopen failed");
    }
  }
}

TestFunction testFunctions[] = {
  {"fread", read_with_fread}
};

int main(int argc, char **argv) {
  u64 cpu_freq = read_cpu_timer_freq(); //estimate_block_freq();

  if (argc == 2) {
    char* file_name = argv[1];
    struct stat input_stat;
    stat(file_name, &input_stat);

    ReadParameters params = {};
    params.destination = allocate_buffer(input_stat.st_size);
    params.file_name = file_name;

    printf("\n");

    if (params.destination.count > 0) {
      RepTester testers[ARRAY_COUNT(testFunctions)] = {};
      u64 it = 0;
      for (;;) {
        printf("%-20s %lu\n", "Iteration:", ++it);
        printf("%-20s %-4.2f MHz\n", "CPU Frequency:", cpu_freq * 1e-6f);
        printf("%-20s %llu bytes\n", "File size:", input_stat.st_size);

        for (u32 func_index = 0; func_index < ARRAY_COUNT(testFunctions); ++func_index) {
          RepTester *tester = testers + func_index;
          TestFunction test_func = testFunctions[func_index];

          printf("\n--- %s ---\n", test_func.name);
          new_test_wave(tester, params.destination.count, cpu_freq);
          test_func.func(tester, &params);
        }
      }
    } else {
      fprintf(stderr, "ERROR: Test data size must be non-zero\n");
    }

    free_buffer(&params.destination);
  } else {
      fprintf(stderr, "Usage: %s [existing filename]\n", argv[0]);
  }


  return 0;
}

