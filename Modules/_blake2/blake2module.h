#ifndef Py_BLAKE2MODULE_H
#define Py_BLAKE2MODULE_H

#ifdef HAVE_LIBB2
#include <blake2.h>

#else
// use vendored copy of blake2

// Prefix all public blake2 symbols with PyBlake2_
#define blake2b            PyBlake2_blake2b
#define blake2b_compress   PyBlake2_blake2b_compress
#define blake2b_final      PyBlake2_blake2b_final
#define blake2b_init       PyBlake2_blake2b_init
#define blake2b_init_key   PyBlake2_blake2b_init_key
#define blake2b_init_param PyBlake2_blake2b_init_param
#define blake2b_update     PyBlake2_blake2b_update
#define blake2bp           PyBlake2_blake2bp
#define blake2bp_final     PyBlake2_blake2bp_final
#define blake2bp_init      PyBlake2_blake2bp_init
#define blake2bp_init_key  PyBlake2_blake2bp_init_key
#define blake2bp_update    PyBlake2_blake2bp_update
#define blake2s            PyBlake2_blake2s
#define blake2s_compress   PyBlake2_blake2s_compress
#define blake2s_final      PyBlake2_blake2s_final
#define blake2s_init       PyBlake2_blake2s_init
#define blake2s_init_key   PyBlake2_blake2s_init_key
#define blake2s_init_param PyBlake2_blake2s_init_param
#define blake2s_update     PyBlake2_blake2s_update
#define blake2sp           PyBlake2_blake2sp
#define blake2sp_final     PyBlake2_blake2sp_final
#define blake2sp_init      PyBlake2_blake2sp_init
#define blake2sp_init_key  PyBlake2_blake2sp_init_key
#define blake2sp_update    PyBlake2_blake2sp_update

#include "impl/blake2.h"

#endif // HAVE_LIBB2

// for secure_zero_memory(), store32(), store48(), and store64()
#include "impl/blake2-impl.h"

#endif // Py_BLAKE2MODULE_H
