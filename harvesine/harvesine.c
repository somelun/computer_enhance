#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include <string.h> // strcmp
#include <stddef.h>
#include <stdbool.h>

#define TOKENS_COUNT 255
#define EARTH_RADIUS 6372.8
typedef char              u8;
typedef unsigned long int u64;
typedef long int          i64;
typedef double            f64;

typedef enum {
  // invalid,        // not initialized
  number,         // any number
  string,         // any string in ""
  // comma,          // ,
  // colon,          // :
  // semicolon,      // ;
  // brace_open,     // {
  // brace_close,    // }
  // bracket_open,   // [
  // bracket_close,  // ]
} token_t;

struct buffer_t {
  i64 size;
  u8* data;
};

struct token_item_t {
  token_t token;
  u64 begin;
  u64 end;
};

struct coords_t {
  f64 a;
  f64 b;
  f64 c;
  f64 d;
};

// json tokeniser
void lexer(const struct buffer_t* const buffer, struct token_item_t* tokens);
struct token_item_t lex_string(const struct buffer_t* const buffer, u64* offset);
struct token_item_t lex_number(const struct buffer_t* const buffer, u64* offset);

// tokens parser
void parser(const struct buffer_t* const buffer, const struct token_item_t* const tokens, struct coords_t* pairs);
void parse_string(const struct buffer_t* const buffer, const u64 begin, const u64 end);
f64 parse_number(const struct buffer_t* const buffer, const u64 begin, const u64 end);

// harvesine calculations
f64 reference_haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius);

// helpers
f64 square(f64 a);
f64 degrees_to_radians(f64 degrees);

void print_token(const struct buffer_t* const buffer, const struct token_item_t* const token_item);

int main(int argc, char* argv[argc +1]) {
  FILE* input = NULL;
  if (argc > 1) {
    input = fopen(argv[1], "rb");
  }

  if (input == NULL) {
    return EXIT_FAILURE;
  }

  // load entire json file into the memory
  struct buffer_t buffer;
  fseek(input, 0, SEEK_END);
  buffer.size = ftell(input);
  fseek(input, 0, SEEK_SET);

  buffer.data = malloc(sizeof(u8) * (buffer.size + 1));
  fread(buffer.data, 1, buffer.size, input);
  fclose(input);

  // null terminatig the buffer
  buffer.data[buffer.size] = 0;

  struct token_item_t* tokens = malloc(sizeof(struct token_item_t) * TOKENS_COUNT);

  lexer(&buffer, tokens);

  u64 pairs_count = TOKENS_COUNT / 4;
  struct coords_t* pairs = malloc(sizeof(struct coords_t) * pairs_count);

  parser(&buffer, tokens, pairs);

  for (u64 i = 0; i < pairs_count; ++i) {
      f64 answer = reference_haversine(pairs[i].a, pairs[i].b, pairs[i].c, pairs[i].d, EARTH_RADIUS);
      if (answer > 0.0) {
        printf("%f\n", answer);
      }
  }

  free(pairs);
  free(tokens);
  free(buffer.data);

  return EXIT_SUCCESS;
}

void lexer(const struct buffer_t* const buffer, struct token_item_t* tokens) {
  const char json_header[] = "{\"pairs\":[\n";

  u64 offset = 11;
  u64 index = 0;
  while (offset++ < buffer->size) {
    switch (buffer->data[offset]) {
      // struct token_item_t token_item = { .token = invalid, .begin = 0, .end = 0 };

      //
      // if we see " then we found string
      case '"': {
        struct token_item_t token_item = lex_string(buffer, &offset);
        tokens[index++] = token_item;
        break;
      }
      //
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
        struct token_item_t token_item = lex_number(buffer, &offset);
        tokens[index++] = token_item;
        break;
      }
      //
      // just comma
      case ',': {
        // struct token_item_t token_item = { .token = comma, .begin = offset, .end = offset };
        // tokens[index++] = token_item;
        // break;
      }
      // just colon
      case ':': {
        // struct token_item_t token_item = { .token = colon, .begin = offset, .end = offset };
        // tokens[index++] = token_item;
        // break;
      }
      //
      // everything we ignore
      case ' ':
      default: break;
    }
  }
}

struct token_item_t lex_string(const struct buffer_t* const buffer, u64* offset) {
  struct token_item_t token_item;
  token_item.token = string;

  token_item.begin = (*offset) + 1;

  do {
    ++(*offset);
  } while (buffer->data[*offset] != '"');

  token_item.end = (*offset) - 1;

  return token_item;
}

struct token_item_t lex_number(const struct buffer_t* const buffer, u64* offset) {
  struct token_item_t token_item;
  token_item.token = number;

  token_item.begin = (*offset);

  while (buffer->data[*offset] != ',' && buffer->data[*offset] != '\n') {
    ++(*offset);
  }

  token_item.end = (*offset) - 1;

  return token_item;
}

void parser(const struct buffer_t* const buffer, const struct token_item_t* const tokens, struct coords_t* pairs) {
  struct coords_t coords;
  u8 counter = 0;
  u64 n = 0;
  f64 value = 0.0;
  for (size_t i = 0; i <= TOKENS_COUNT; ++i) {
    switch(tokens[i].token) {
      //
      case string: {
        parse_string(buffer, tokens[i].begin, tokens[i].end);

        break;
      }
      //
      case number: {
        value = parse_number(buffer, tokens[i].begin, tokens[i].end);
        counter++;
        break;
      }
    }

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
      pairs[n++] = coords;
      // f64 answer = reference_haversine(coords.a, coords.b, coords.c, coords.d, EARTH_RADIUS);
      // printf("%f %f %f %f %f\n", coords.a, coords.b, coords.c, coords.d, answer);
    }
  }
}

void parse_string(const struct buffer_t* const buffer, const u64 begin, const u64 end) {
}

f64 parse_number(const struct buffer_t* const buffer, const u64 begin, const u64 end) {
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

void print_token(const struct buffer_t* const buffer, const struct token_item_t* const token_item) {
  for (size_t i = token_item->begin; i <= token_item->end; ++i) {
    printf("%c", buffer->data[i]);
  }
  printf(" ");
}

