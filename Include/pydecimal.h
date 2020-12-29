/*
 * Copyright (c) 2020 Stefan Krah. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef CPYTHON_DECIMAL_H_
#define CPYTHON_DECIMAL_H_


#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*                               Libmpdec API                               */
/****************************************************************************/

#ifndef LIBMPDEC_MPDECIMAL_H_
struct mpd_t;  /* ABI-stable in the libmpdec-2.x series */

/* status cases for getting a triple */
enum mpd_triple_class {
  MPD_TRIPLE_NORMAL,
  MPD_TRIPLE_INF,
  MPD_TRIPLE_QNAN,
  MPD_TRIPLE_SNAN,
  MPD_TRIPLE_ERROR,
};

typedef struct {
  enum mpd_triple_class tag;
  uint8_t sign;
  uint64_t hi;
  uint64_t lo;
  int64_t exp;
} mpd_uint128_triple_t;
#endif


/****************************************************************************/
/*                                Capsule API                               */
/****************************************************************************/

/* Simple API */
#define PyDec_TypeCheck_INDEX 0
#define PyDec_TypeCheck_RETURN int
#define PyDec_TypeCheck_ARGS (const PyObject *)

#define PyDec_IsSpecial_INDEX 1
#define PyDec_IsSpecial_RETURN int
#define PyDec_IsSpecial_ARGS (const PyObject *)

#define PyDec_IsNaN_INDEX 2
#define PyDec_IsNaN_RETURN int
#define PyDec_IsNaN_ARGS (const PyObject *)

#define PyDec_IsInfinite_INDEX 3
#define PyDec_IsInfinite_RETURN int
#define PyDec_IsInfinite_ARGS (const PyObject *)

#define PyDec_GetDigits_INDEX 4
#define PyDec_GetDigits_RETURN int64_t
#define PyDec_GetDigits_ARGS (const PyObject *)

#define PyDec_AsUint128Triple_INDEX 5
#define PyDec_AsUint128Triple_RETURN mpd_uint128_triple_t
#define PyDec_AsUint128Triple_ARGS (const PyObject *)

#define PyDec_FromUint128Triple_INDEX 6
#define PyDec_FromUint128Triple_RETURN PyObject *
#define PyDec_FromUint128Triple_ARGS (const mpd_uint128_triple_t *triple)

/* Advanced API */
#define PyDec_Alloc_INDEX 7
#define PyDec_Alloc_RETURN PyObject *
#define PyDec_Alloc_ARGS (void)

#define PyDec_Get_INDEX 8
#define PyDec_Get_RETURN mpd_t *
#define PyDec_Get_ARGS (PyObject *)

#define PyDec_GetConst_INDEX 9
#define PyDec_GetConst_RETURN const mpd_t *
#define PyDec_GetConst_ARGS (const PyObject *)

#define CPYTHON_DECIMAL_MAX_API 10


#ifdef CPYTHON_DECIMAL_MODULE
/* Simple API */
static PyDec_TypeCheck_RETURN PyDec_TypeCheck PyDec_TypeCheck_ARGS;
static PyDec_IsSpecial_RETURN PyDec_IsSpecial PyDec_IsSpecial_ARGS;
static PyDec_IsNaN_RETURN PyDec_IsNaN PyDec_IsNaN_ARGS;
static PyDec_IsInfinite_RETURN PyDec_IsInfinite PyDec_IsInfinite_ARGS;
static PyDec_GetDigits_RETURN PyDec_GetDigits PyDec_GetDigits_ARGS;
static PyDec_AsUint128Triple_RETURN PyDec_AsUint128Triple PyDec_AsUint128Triple_ARGS;
static PyDec_FromUint128Triple_RETURN PyDec_FromUint128Triple PyDec_FromUint128Triple_ARGS;

/* Advanced API */
static PyDec_Alloc_RETURN PyDec_Alloc PyDec_Alloc_ARGS;
static PyDec_Get_RETURN PyDec_Get PyDec_Get_ARGS;
static PyDec_GetConst_RETURN PyDec_GetConst PyDec_GetConst_ARGS;
#else
static void **_decimal_api;

/* Simple API */
#define PyDec_TypeCheck \
    (*(PyDec_TypeCheck_RETURN (*)PyDec_TypeCheck_ARGS) _decimal_api[PyDec_TypeCheck_INDEX])

#define PyDec_IsSpecial \
    (*(PyDec_IsSpecial_RETURN (*)PyDec_IsSpecial_ARGS) _decimal_api[PyDec_IsSpecial_INDEX])

#define PyDec_IsNaN \
    (*(PyDec_IsNaN_RETURN (*)PyDec_IsNaN_ARGS) _decimal_api[PyDec_IsNaN_INDEX])

#define PyDec_IsInfinite \
    (*(PyDec_IsInfinite_RETURN (*)PyDec_IsInfinite_ARGS) _decimal_api[PyDec_IsInfinite_INDEX])

#define PyDec_GetDigits \
    (*(PyDec_GetDigits_RETURN (*)PyDec_GetDigits_ARGS) _decimal_api[PyDec_GetDigits_INDEX])

#define PyDec_AsUint128Triple \
    (*(PyDec_AsUint128Triple_RETURN (*)PyDec_AsUint128Triple_ARGS) _decimal_api[PyDec_AsUint128Triple_INDEX])

#define PyDec_FromUint128Triple \
    (*(PyDec_FromUint128Triple_RETURN (*)PyDec_FromUint128Triple_ARGS) _decimal_api[PyDec_FromUint128Triple_INDEX])

/* Advanced API */
#define PyDec_Alloc \
    (*(PyDec_Alloc_RETURN (*)PyDec_Alloc_ARGS) _decimal_api[PyDec_Alloc_INDEX])

#define PyDec_Get \
    (*(PyDec_Get_RETURN (*)PyDec_Get_ARGS) _decimal_api[PyDec_Get_INDEX])

#define PyDec_GetConst \
    (*(PyDec_GetConst_RETURN (*)PyDec_GetConst_ARGS) _decimal_api[PyDec_GetConst_INDEX])


static int
import_decimal(void)
{
    _decimal_api = (void **)PyCapsule_Import("_decimal._API", 0);
    if (_decimal_api == NULL) {
        return -1;
    }

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif  /* CPYTHON_DECIMAL_H_ */
