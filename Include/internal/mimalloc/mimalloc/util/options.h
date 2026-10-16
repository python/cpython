/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_OPTIONS_H
#define MI_INTERNAL_OPTIONS_H

#include "../types.h"

void          _mi_fputs(mi_output_fun* out, void* arg, const char* prefix, const char* message);
void          _mi_fprintf(mi_output_fun* out, void* arg, const char* fmt, ...);
void          _mi_raw_message(const char* fmt, ...);
void          _mi_warning_message(const char* fmt, ...);
void          _mi_verbose_message(const char* fmt, ...);
void          _mi_trace_message(const char* fmt, ...);
void          _mi_options_init(void);
void          _mi_options_post_init(void);
long          _mi_option_get_fast(mi_option_t option);
void          _mi_error_message(int err, const char* fmt, ...);
mi_block_t*   _mi_block_next_is_corrupted(const mi_page_t* page, const mi_block_t* block, const mi_block_t* next); 

/* -----------------------------------------------------------
  Options (exposed for the debugger)
----------------------------------------------------------- */
typedef enum mi_option_init_e {
  MI_OPTION_UNINIT,       // not yet initialized
  MI_OPTION_DEFAULTED,    // not found in the environment, use default value
  MI_OPTION_INITIALIZED   // found in environment or set explicitly
} mi_option_init_t;

typedef struct mi_option_desc_s {
  long              value;  // the value
  mi_option_init_t  init;   // is it initialized yet? (from the environment)
  mi_option_t       option; // for debugging: the option index should match the option
  const char*       name;   // option name without `mimalloc_` prefix
  const char*       legacy_name; // potential legacy option name
} mi_option_desc_t;


#endif
