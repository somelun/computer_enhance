#include <math.h>
#include <sys/stat.h>

#define PROFILER 1

#include "types.h"      // custom type aliases
#include "profiler.hpp" // custom profiler

#define EARTH_RADIUS 6372.8

struct Buffer {
  u64 size;
  u8* data;
};

struct TokenItem {
  u64 begin;
  u64 end;
};

struct Coords {
  f64 a;
  f64 b;
  f64 c;
  f64 d;
};

// json tokeniser
void lexer(const struct Buffer* const buffer, struct TokenItem* tokens);
struct TokenItem lex_number(const struct Buffer* const buffer, u64* offset);

// tokens parser
u64 parser(const struct Buffer* const buffer, const struct TokenItem* const tokens, const u64 tokens_size, struct Coords* pairs);
f64 parse_number(const struct Buffer* const buffer, const u64 begin, const u64 end);

// harvesine calculations
f64 reference_haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius);

//helpers
f64 square(f64 a);
f64 degrees_to_radians(f64 degrees);

int main(int argc, char* argv[argc +1]) {
  BeginProfile();

  FILE* input = NULL;
  if (argc > 1) {
    input = fopen(argv[1], "rb");
  }

  if (input == NULL) {
    printf("No input files!\n");
    return EXIT_FAILURE;
  }

  // load entire json file into the memory
  struct Buffer buffer;
  // fseek(input, 0, SEEK_END);
  // buffer.size = ftell(input);
  // fseek(input, 0, SEEK_SET);

  struct stat input_stat;
  stat(argv[1], &input_stat);

  buffer.size = input_stat.st_size;

  {
    TIME_BANDWIDTH("read file", buffer.size)
    buffer.data = (u8*)malloc(sizeof(u8) * (buffer.size + 1));
    fread(buffer.data, buffer.size, 1, input);
    fclose(input);
  }

  // null terminatig the buffer
  buffer.data[buffer.size] = 0;

  const u64 count = input_stat.st_size / 8;

  struct TokenItem* tokens = (TokenItem*)malloc(sizeof(struct TokenItem) * count);

  u64 max_pairs_count = count / 4;
  struct Coords* pairs = (Coords*)malloc(sizeof(struct Coords) * max_pairs_count);

  lexer(&buffer, tokens);

  u64 pairs_count = parser(&buffer, tokens, count, pairs);

  f64 average = 0.0f;
  u64 average_count = 0;
  {
    TIME_BANDWIDTH("harvesine sum", pairs_count * sizeof(Coords))
    for (u64 i = 0; i < pairs_count; ++i) {
      f64 answer = reference_haversine(pairs[i].a, pairs[i].b, pairs[i].c, pairs[i].d, EARTH_RADIUS);
      if (answer > 0.0) {
        average += answer;
        average_count++;
      }
    }
  }

  average = average / average_count;
  printf("\n");
  printf("Input size: %lu\n", buffer.size);
  printf("Pairs count: %lu\n", pairs_count);
  printf("Harvesine distance is %f\n", average);

  {
    TIME_BLOCK("free buffers")
    free(pairs);
    free(tokens);
    free(buffer.data);
  }

  EndAndPrintProfile();

  return EXIT_SUCCESS;
}

void lexer(const struct Buffer* const buffer, struct TokenItem* tokens) {
  TIME_FUNC;

  u64 offset = 11;
  u64 index = 0;
  while (offset++ < buffer->size) {
    switch (buffer->data[offset]) {
      // if we see - or 0..9 symbols we found number
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        struct TokenItem token_item = lex_number(buffer, &offset);
        tokens[index++] = token_item;
        break;
      }
      // everything else we ignore, even string
      case '"':
      case ',':
      case ':':
      case ' ':
      default: break;
    }
  }
}

struct TokenItem lex_number(const struct Buffer* const buffer, u64* offset) {
  TIME_FUNC;

  struct TokenItem token_item;

  token_item.begin = (*offset);

  while (buffer->data[*offset] != ',' && buffer->data[*offset] != '\n') {
    ++(*offset);
  }

  token_item.end = (*offset) - 1;

  return token_item;
}

u64 parser(const struct Buffer* const buffer, const struct TokenItem* const tokens,const u64 tokens_size, struct Coords* pairs) {
  TIME_FUNC;

  struct Coords coords;
  u8 counter = 0;
  u64 n = 0;
  f64 value = 0.0;
  for (size_t i = 0; i <= tokens_size; ++i) {
    value = parse_number(buffer, tokens[i].begin, tokens[i].end);
    counter++;

    switch (counter) {
      case 1:
        coords.a = value;
        break;
      case 2:
        coords.b = value;
        break;
      case 3:
        coords.c = value;
        break;
      case 4:
        coords.d = value;
        break;
      default:
        break;
    }

    if (counter == 4) {
      counter = 0;
      // slow place
      if (coords.a + coords.b + coords.c + coords.d != 0.0f) {
        pairs[n++] = coords;
      }
    }
  }

  return n;
}

f64 parse_number(const struct Buffer* const buffer, const u64 begin, const u64 end) {
  TIME_FUNC;

  bool fractional = false;

  f64 value = 0.0;

  u8 sign = 1;

  f64 digit = 10.0;
  for (size_t i = begin; i <= end; ++i) {
    const u8 ch = buffer->data[i];
    if (ch == '-') {
      sign = -1;
      continue;
    }

    if (ch == '.') {
      fractional = true;
      digit = 1;
      continue;
    }

    u8 number = atoi(&ch);

    if (fractional) {
      digit *= 10;
      value = value + (number / digit);
    } else {
      value = value * 10 + number;
    }

  }

  value *= sign;
  return value;
}

// NOTE(casey): EarthRadius is generally expected to be 6372.8
f64 reference_haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius) {
  // NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
  //  Instead, it attempts to follow, as closely as possible, the formula used in the real-world
  //  question on which these homework exercises are loosely based.

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
  return a * a;
}

f64 degrees_to_radians(f64 degrees) {
  f64 result = 0.01745329251994329577 * degrees;
  return result;
}

