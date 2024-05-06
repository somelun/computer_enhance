#include "stdlib.h"
#include "stdio.h"

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef double   f64;

f64 rand_in_range(f64 min, f64 max);
void insert_random_coords_to_file(FILE* file);

int main(int argc, char* argv[argc + 1]) {
  if (argc < 3) {
    printf("not enough parameters\n");
    return EXIT_FAILURE;
  }

  i32 seed;
  if (sscanf (argv[1], "%d", &seed) != 1) {
    fprintf(stderr, "error - argv[1] (seed) is not an integer");
  }
  srand(seed);

  i32 count;
  if (sscanf (argv[2], "%d", &count) != 1) {
    fprintf(stderr, "error - argv[2] (count) not an integer");
  }

  char output_name[64];
  snprintf(output_name, sizeof(output_name), "coords_%d.json", count);

  FILE* output = fopen(output_name, "w");
  fprintf(output, "{\"pairs\":[\n");

  for (i32 i = 0; i < count - 1; ++i) {
    insert_random_coords_to_file(output);
    fprintf(output, ",\n");
  }
  insert_random_coords_to_file(output);
  fprintf(output, "\n");

  fprintf(output, "]}");

  return EXIT_SUCCESS;
}

f64 rand_in_range(f64 min, f64 max) {
  f64 t = (f64)rand() / (f64)RAND_MAX;
  return (1.0 - t) * min + t * max;
}

void insert_random_coords_to_file(FILE* file) {
  const f64 x0 = rand_in_range(-180.0, 180.);
  const f64 y0 = rand_in_range(-90.0, 90.0);
  const f64 x1 = rand_in_range(-180.0, 180.);
  const f64 y1 = rand_in_range(-90.0, 90.0);

  fprintf(file, "    \"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f", x0, y0, x1, y1);
}

