#include "stdlib.h"
#include "stdio.h"
#include "math.h"

typedef char     u8;
typedef long int i64;
typedef double   f64;

enum Token {
  Letter,
  Digit,
  Whitespace,
  Comma,
  Semicolon,
  SquareBracketOpen,
  SquareBracketClose,
  CurlyBracketOpen,
  CurlyBrakcetClose,
};

// harvesine calculations
f64 reference_haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius);

// helpers
f64 square(f64 a);
f64 degrees_to_radians(f64 degrees);

int main(int argc, char* argv[argc +1]) {
  FILE* input = NULL;
  if (argc > 1) {
    input = fopen(argv[1], "rb");
  }

  if (input == NULL) {
    return EXIT_FAILURE;
  }

  // load entire json file into the memory
  fseek(input, 0, SEEK_END);
  i64 input_size = ftell(input);
  fseek(input, 0, SEEK_SET);

  u8* buffer = malloc(input_size + 1);
  fread(buffer, input_size, 1, input);
  fclose(input);

  // null terminatig the buffer
  buffer[input_size] = 0;

  // do things
  printf("%s\n", buffer);

  free(buffer);

  return EXIT_SUCCESS;
}

// NOTE(casey): EarthRadius is generally expected to be 6372.8
f64 reference_haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius) {
  // NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
  //  Instead, it attempts to follow, as closely as possible, the formula used in the real-world
  //  question on which these homework exercises are loosely based.
  //

  f64 lat1 = y0;
  f64 lat2 = y1;
  f64 lon1 = x0;
  f64 lon2 = x1;

  f64 dLat = degrees_to_radians(lat2 - lat1);
  f64 dLon = degrees_to_radians(lon2 - lon1);
  lat1 = degrees_to_radians(lat1);
  lat2 = degrees_to_radians(lat2);

  f64 a = square(sin(dLat / 2.0)) + cos(lat1) * cos(lat2) * square(sin(dLon / 2));
  f64 c = 2.0 * asin(sqrt(a));

  f64 result = earth_radius * c;

  return result;
}

f64 square(f64 a) {
  f64 result = (a * a);
  return result;
}

f64 degrees_to_radians(f64 degrees) {
  f64 result = 0.01745329251994329577 * degrees;
  return result;
}

