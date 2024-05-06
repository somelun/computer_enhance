#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include <string.h> // strcmp
#include <stddef.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "timers.h"

#define EARTH_RADIUS 6372.8

typedef char          u8;
typedef unsigned long u64;
typedef long          i64;
typedef double        f64;

typedef enum {
  number,         // any number
  string,         // any string in ""
} t_token;

struct t_buffer {
  u64 size;
  u8* data;
};

struct t_token_item {
  t_token token;
  u64 begin;
  u64 end;
};

struct t_coords {
  f64 a;
  f64 b;
  f64 c;
  f64 d;
};

// json tokeniser
void lexer(const struct t_buffer* const buffer, struct t_token_item* tokens);
struct t_token_item lex_string(const struct t_buffer* const buffer, u64* offset);
struct t_token_item lex_number(const struct t_buffer* const buffer, u64* offset);

// tokens parser
void parser(const struct t_buffer* const buffer, const struct t_token_item* const tokens, const u64 tokens_size, struct t_coords* pairs);
void parse_string(const struct t_buffer* const buffer, const u64 begin, const u64 end);
f64 parse_number(const struct t_buffer* const buffer, const u64 begin, const u64 end);

// harvesine calculations
f64 reference_haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius);

// helpers
f64 square(f64 a);
f64 degrees_to_radians(f64 degrees);

void print_token(const struct t_buffer* const buffer, const struct t_token_item* const token_item);
void print_time_elapsed(const char* text, u64 total_time, u64 begin, u64 end);

int main(int argc, char* argv[argc +1]) {
  const u64 profile_begin = read_cpu_timer();

  FILE* input = NULL;
  if (argc > 1) {
    input = fopen(argv[1], "rb");
  }

  if (input == NULL) {
    printf("No input files!\n");
    return EXIT_FAILURE;
  }

  // load entire json file into the memory
  struct t_buffer buffer;
  // fseek(input, 0, SEEK_END);
  // buffer.size = ftell(input);
  // fseek(input, 0, SEEK_SET);

  struct stat input_stat;
  stat(argv[1], &input_stat);

  buffer.size = input_stat.st_size;

  const u64 profile_read = read_cpu_timer();

  buffer.data = malloc(sizeof(u8) * (buffer.size + 1));
  fread(buffer.data, 1, buffer.size, input);
  fclose(input);

  const u64 profile_misc = read_cpu_timer();

  // null terminatig the buffer
  buffer.data[buffer.size] = 0;

  const u64 count = input_stat.st_size / 8;

  struct t_token_item* tokens = malloc(sizeof(struct t_token_item) * count);

  const u64 profile_lexer = read_cpu_timer();

  u64 pairs_count = count / 4;
  struct t_coords* pairs = malloc(sizeof(struct t_coords) * pairs_count);

  lexer(&buffer, tokens);

  const u64 profile_parser = read_cpu_timer();

  parser(&buffer, tokens, count, pairs);

  const u64 profile_harv = read_cpu_timer();

  f64 average = 0.0f;
  u64 average_count = 0;
  for (u64 i = 0; i < pairs_count; ++i) {
      f64 answer = reference_haversine(pairs[i].a, pairs[i].b, pairs[i].c, pairs[i].d, EARTH_RADIUS);
      if (answer > 0.0) {
        // printf("%f\n", answer);
        average += answer;
        average_count++;
      }
  }

  const u64 profile_end = read_cpu_timer();

  average = average / average_count;
  printf("Harvesine distance is %f\n", average);

  free(pairs);
  free(tokens);
  free(buffer.data);

  const f64 freq = (f64)get_cpu_freq();

  u64 timer_total = profile_end - profile_begin;

  printf("total time : %f\n", timer_total / freq);

  print_time_elapsed("startup", timer_total, profile_begin, profile_read);
  print_time_elapsed("read", timer_total, profile_read, profile_misc);
  print_time_elapsed("lexer", timer_total, profile_lexer, profile_parser);
  print_time_elapsed("parser", timer_total, profile_parser, profile_harv);
  print_time_elapsed("harvesine", timer_total, profile_harv, profile_end);

  return EXIT_SUCCESS;
}

void lexer(const struct t_buffer* const buffer, struct t_token_item* tokens) {
  u64 offset = 11;
  u64 index = 0;
  while (offset++ < buffer->size) {
    switch (buffer->data[offset]) {
      //
      // if we see " then we found string
      case '"': {
        struct t_token_item token_item = lex_string(buffer, &offset);
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
        struct t_token_item token_item = lex_number(buffer, &offset);
        tokens[index++] = token_item;
        break;
      }
      //
      // everything else we ignore
      case ',':
      case ':':
      case ' ':
      default: break;
    }
  }
}

struct t_token_item lex_string(const struct t_buffer* const buffer, u64* offset) {
  struct t_token_item token_item;
  token_item.token = string;

  token_item.begin = (*offset) + 1;

  do {
    ++(*offset);
  } while (buffer->data[*offset] != '"');

  token_item.end = (*offset) - 1;

  return token_item;
}

struct t_token_item lex_number(const struct t_buffer* const buffer, u64* offset) {
  struct t_token_item token_item;
  token_item.token = number;

  token_item.begin = (*offset);

  while (buffer->data[*offset] != ',' && buffer->data[*offset] != '\n') {
    ++(*offset);
  }

  token_item.end = (*offset) - 1;

  return token_item;
}

void parser(const struct t_buffer* const buffer, const struct t_token_item* const tokens,const u64 tokens_size, struct t_coords* pairs) {
  struct t_coords coords;
  u8 counter = 0;
  u64 n = 0;
  f64 value = 0.0;
  for (size_t i = 0; i <= tokens_size; ++i) {
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

void parse_string(const struct t_buffer* const buffer, const u64 begin, const u64 end) {
}

f64 parse_number(const struct t_buffer* const buffer, const u64 begin, const u64 end) {
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

void print_token(const struct t_buffer* const buffer, const struct t_token_item* const token_item) {
  for (size_t i = token_item->begin; i <= token_item->end; ++i) {
    printf("%c", buffer->data[i]);
  }
  printf(" ");
}

void print_time_elapsed(const char* label, u64 total_time, u64 begin, u64 end) {
  u64 elapsed = end - begin;
  f64 percent = 100.0 * ((f64)elapsed / (f64)total_time);
  printf("  %s: %lu (%.2f%%)\n", label, elapsed, percent);
}

