// Route OpenSSL allocations through the raw memory allocators.
// Shared by the _ssl and _hashlib modules.

#ifndef Py_OPENSSL_MEM_H
#define Py_OPENSSL_MEM_H

#include "Python.h"

#include <openssl/crypto.h>       // CRYPTO_set_mem_functions()

// LibreSSL stubs out CRYPTO_set_mem_functions() and BoringSSL lacks it.
// AWS-LC has it, but unlike OpenSSL it does not refuse to install hooks
// after the first allocation, so earlier size-prefixed allocations would
// be freed with the wrong allocator.
#if !defined(LIBRESSL_VERSION_NUMBER) && !defined(OPENSSL_IS_BORINGSSL) \
    && !defined(OPENSSL_IS_AWSLC)
#  define _Py_OPENSSL_CAN_SET_MEM_FUNCTIONS
#endif

#ifdef _Py_OPENSSL_CAN_SET_MEM_FUNCTIONS

static void *
_PyOpenSSL_Malloc(size_t size, const char *Py_UNUSED(file),
                  int Py_UNUSED(line))
{
    return PyMem_RawMalloc(size);
}

static void *
_PyOpenSSL_Realloc(void *ptr, size_t size, const char *Py_UNUSED(file),
                   int Py_UNUSED(line))
{
    return PyMem_RawRealloc(ptr, size);
}

static void
_PyOpenSSL_Free(void *ptr, const char *Py_UNUSED(file),
                int Py_UNUSED(line))
{
    PyMem_RawFree(ptr);
}

#endif  // _Py_OPENSSL_CAN_SET_MEM_FUNCTIONS

static void
_PyOpenSSL_SetupMemFunctions(void)
{
#ifdef _Py_OPENSSL_CAN_SET_MEM_FUNCTIONS
    // Fails if OpenSSL has already allocated memory (e.g. another
    // libcrypto user in the process); it then keeps its current allocator.
    (void)CRYPTO_set_mem_functions(_PyOpenSSL_Malloc, _PyOpenSSL_Realloc,
                                   _PyOpenSSL_Free);
#endif
}

#endif  // !Py_OPENSSL_MEM_H
