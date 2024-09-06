#include <stdlib.h>
#include <sys/stat.h>

#include "repetition_tester.hpp"

#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

///////////////////////////////////////////////////////////////
/// Helper data structures
struct Buffer {
    size_t count;
    u8 *data;
};

enum class AllocationType : u8 {
  none = 0,
  malloc,

  COUNT,
};

struct ReadParameters {
  Buffer destination;
  const char* file_name;
  AllocationType alloc_type;
};

typedef void read_overhead_test_func(RepTester *tester, ReadParameters *params);

struct TestFunction {
  const char *name;
  read_overhead_test_func *func;
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

static const char* alloc_type_descripion(AllocationType alloc_type) {
  const char* result;
  switch (alloc_type) {
    case AllocationType::none:
      result = "none";
      break;
    case AllocationType::malloc:
      result = "malloc";
      break;
    default:
      break;
  };

  return result;
};

static void handle_allocation(ReadParameters* params, Buffer* buffer) {
  switch (params->alloc_type) {
    case AllocationType::none:
      break;
    case AllocationType::malloc:
      *buffer = allocate_buffer(params->destination.count);
      break;
    default:
      break;
  };
}

static void handle_deallocation(ReadParameters* params, Buffer* buffer) {
  switch (params->alloc_type) {
    case AllocationType::none:
      break;
    case AllocationType::malloc:
      free_buffer(buffer);
      break;
    default:
      break;
  };
}

///////////////////////////////////////////////////////////////
/// Teste functions
static void read_with_fread(RepTester *tester, ReadParameters *params) {
  while (is_testing(tester)) {
    FILE *file = fopen(params->file_name, "rb");
    if (file) {
      Buffer dest_buffer = params->destination;
      handle_allocation(params, &dest_buffer);

      begin_time(tester);
      size_t result = fread(dest_buffer.data, dest_buffer.count, 1, file);
      end_time(tester);

      if (result == 1) {
        count_bytes(tester, dest_buffer.count);
      } else {
        error(tester, "fread failed");
      }

      handle_deallocation(params, &dest_buffer);
      fclose(file);
    } else {
      error(tester, "fopen failed");
    }
  }
}

TestFunction testFunctions[] = {
  {"fread", read_with_fread}
};

///////////////////////////////////////////////////////////////
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
      // for (;;) {
      // while(true) {
        for (u32 func_index = 0; func_index < ARRAY_COUNT(testFunctions); ++func_index) {

          printf("%-20s %lu\n", "Iteration:", ++it);
          printf("%-20s %-4.2f MHz\n", "CPU Frequency:", cpu_freq * 1e-6f);
          printf("%-20s %llu bytes\n", "File size:", input_stat.st_size);

          const u8 alloc_type_count = static_cast<u8>(AllocationType::COUNT);
          for (u8 alloc_index = 0; alloc_index < alloc_type_count; ++alloc_index) {
            params.alloc_type = static_cast<AllocationType>(alloc_index);

            RepTester *tester = testers + func_index;
            TestFunction test_func = testFunctions[func_index];

            printf("\n--- %s (%s)---\n", test_func.name, alloc_type_descripion(static_cast<AllocationType>(alloc_index)));
            new_test_wave(tester, params.destination.count, cpu_freq);
            test_func.func(tester, &params);
          }
        }
      // }
    } else {
      fprintf(stderr, "ERROR: Test data size must be non-zero\n");
    }

    free_buffer(&params.destination);
  } else {
      fprintf(stderr, "Usage: %s [existing filename]\n", argv[0]);
  }

  return 0;
}

