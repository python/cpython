// A double-to-string conversion library: https://github.com/vitaut/zmij/
//
// Copyright (c) 2025 - present, Victor Zverovich
// Distributed under the MIT license (see LICENSE) or alternatively
// the Boost Software License, Version 1.0.

#ifndef ZMIJ_C_H_
#define ZMIJ_C_H_

#include <stddef.h>  // size_t
#include <string.h>  // memcpy

// Implementation details, use zmij_write_* instead.
char* zmij_detail_write_float(float value, char* buffer);
char* zmij_detail_write_double(double value, char* buffer);

enum {
  zmij_float_buffer_size = 17,
  zmij_double_buffer_size = 34,
};

/// Writes the shortest correctly rounded decimal representation of `value` to
/// `out` without a null terminator. Returns a pointer past the last character
/// written; if the representation exceeds `n` characters, only the first `n`
/// are written.
static inline char* zmij_write_float(char* out, size_t n, float value) {
  if (n >= zmij_float_buffer_size) return zmij_detail_write_float(value, out);
  char buffer[zmij_float_buffer_size];
  size_t size = zmij_detail_write_float(value, buffer) - buffer;
  if (size > n) size = n;
  memcpy(out, buffer, size);
  return out + size;
}

/// Writes the shortest correctly rounded decimal representation of `value` to
/// `out` without a null terminator. Returns a pointer past the last character
/// written; if the representation exceeds `n` characters, only the first `n`
/// are written.
static inline char* zmij_write_double(char* out, size_t n, double value) {
  if (n >= zmij_double_buffer_size) return zmij_detail_write_double(value, out);
  char buffer[zmij_double_buffer_size];
  size_t size = zmij_detail_write_double(value, buffer) - buffer;
  if (size > n) size = n;
  memcpy(out, buffer, size);
  return out + size;
}

#endif  // ZMIJ_C_H_
