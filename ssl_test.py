#! /opt/python-master/bin/python3

import os
import sys

import ssl
import socket
import binascii

hostname = 'www.python.org'
context = ssl.create_default_context()

km_len = 32
sock = socket.create_connection((hostname, 443))
ssock = context.wrap_socket(sock, server_hostname=hostname)

label = b'XYZZY'
context = b'PLUGH'
# context = None

print()
print("ssl")

print("ssl.OPENSSL_VERSION_NUMBER", hex(ssl.OPENSSL_VERSION_NUMBER))
print("ssl.OPENSSL_VERSION", ssl.OPENSSL_VERSION)
print("ssl.OPENSSL_CFLAGS", ssl.OPENSSL_CFLAGS)
print("ssl.OPENSSL_BUILT_ON", ssl.OPENSSL_BUILT_ON)
print("ssl.OPENSSL_PLATFORM", ssl.OPENSSL_PLATFORM)
print("ssl.OPENSSL_DIR", ssl.OPENSSL_DIR)

crypto_dll_path = ssl._ssl.CRYPTO_DLL_PATH
print("CRYPTO_DLL_PATH:", crypto_dll_path)

ssl_dll_path = ssl._ssl.SSL_DLL_PATH
print("SSL_DLL_PATH:", ssl_dll_path)

print("ssock.version:", repr(ssock.version()))
ssl_addr = ssock._sslobj.get_internal_addr()

if hasattr(ssock, "export_keying_material"):
    # Try using a built in version
    km = ssock.export_keying_material(label, km_len, context)
    print("%s:" % ssock.export_keying_material.__name__, binascii.b2a_hex(km))

    # And delete it so that the extension version is installed next
    del ssl.SSLSocket.export_keying_material

print()
print("extension")
import ssl_export_keying_material
km = ssock.export_keying_material(label, km_len, context)
print("%s:" % ssock.export_keying_material.__name__, binascii.b2a_hex(km))

print()
print("ctypes")

import ctypes
import ctypes.util

crypto_lib = ctypes.CDLL(crypto_dll_path)
ssl_lib = ctypes.CDLL(ssl_dll_path)

crypto_lib.OpenSSL_version_num.restype = ctypes.c_uint

crypto_lib.OpenSSL_version.argtypes = (ctypes.c_int, )
crypto_lib.OpenSSL_version.restype = ctypes.c_char_p

print("ctypes OpenSSL_version_num:", hex(crypto_lib.OpenSSL_version_num()))
print("ctypes OpenSSL_version:", crypto_lib.OpenSSL_version(0))
assert ssl.OPENSSL_CFLAGS == crypto_lib.OpenSSL_version(1).decode('utf-8')
assert ssl.OPENSSL_BUILT_ON == crypto_lib.OpenSSL_version(2).decode('utf-8')
assert ssl.OPENSSL_PLATFORM == crypto_lib.OpenSSL_version(3).decode('utf-8')

ssl_lib.SSL_get_version.argtypes = (ctypes.c_void_p, )
ssl_lib.SSL_get_version.restype = ctypes.c_char_p

ssl_lib.SSL_export_keying_material.argtypes = (
    ctypes.c_void_p,                  # SSL pointer
    ctypes.c_void_p, ctypes.c_size_t, # out pointer, out length
    ctypes.c_void_p, ctypes.c_size_t, # label buffer, label length
    ctypes.c_void_p, ctypes.c_size_t, # context, context length
    ctypes.c_int)                     # use context flag
ssl_lib.SSL_export_keying_material.restype = ctypes.c_int

print("ctypes SSL_get_version:", repr(ssl_lib.SSL_get_version(ssl_addr)))

c_km = ctypes.create_string_buffer(km_len)
c_label = ctypes.create_string_buffer(label, len(label))
if context is None:
    c_context_len = 0
    c_context = 0
    c_use_context = 0
else:
    c_context_len = len(context)
    c_context = ctypes.create_string_buffer(context, c_context_len)
    c_use_context = 1

if ssl_lib.SSL_export_keying_material(
        ssl_addr,
        c_km, len(c_km),
        c_label, len(c_label),
        c_context, c_context_len, c_use_context):
    print("ctypes SSL_export_keying_material", binascii.b2a_hex(c_km))

print()
print("cffi")

import cffi

# Create a CFFI interface to the library
crypto_src = '''
unsigned long OpenSSL_version_num(void);
const char *OpenSSL_version(int type);
unsigned int OPENSSL_version_major(void);
unsigned int OPENSSL_version_minor(void);
unsigned int OPENSSL_version_patch(void);
'''
crypto_ffi = cffi.FFI()
crypto_ffi.cdef(crypto_src)
crypto_lib = crypto_ffi.dlopen(crypto_dll_path)

ssl_src = '''
typedef struct ssl_st SSL;
int SSL_export_keying_material(
    SSL *s,
    unsigned char *out, size_t olen,
    const char *label, size_t llen,
    const unsigned char *context, size_t contextlen,
    int use_context);
const char *SSL_get_version(const SSL *ssl);
'''

ssl_ffi = cffi.FFI()
ssl_ffi.cdef(ssl_src)
ssl_lib = ssl_ffi.dlopen(ssl_dll_path)

print("cffi OpenSSL_version_num:", hex(crypto_lib.OpenSSL_version_num()))
print("cffi OpenSSL_version(0):", crypto_ffi.string(crypto_lib.OpenSSL_version(0)))
assert ssl.OPENSSL_CFLAGS == crypto_ffi.string(crypto_lib.OpenSSL_version(1)).decode('utf-8')
assert ssl.OPENSSL_BUILT_ON == crypto_ffi.string(crypto_lib.OpenSSL_version(2)).decode('utf-8')
assert ssl.OPENSSL_PLATFORM == crypto_ffi.string(crypto_lib.OpenSSL_version(3)).decode('utf-8')

ssl_ptr = ssl_ffi.cast('void *', ssl_addr)
print("cffi SSL_get_version:", repr(ssl_ffi.string(ssl_lib.SSL_get_version(ssl_ptr))))

f_km = ssl_ffi.new('unsigned char [%u]' % km_len)
f_label = ssl_ffi.new('unsigned char [%u]' % len(label), label)
if context is None:
    f_context_len = 0
    f_context = ssl_ffi.NULL
    f_use_context = 0
else:
    f_context_len = len(context)
    f_context = ssl_ffi.new('unsigned char [%u]' % f_context_len, context)
    f_use_context = 1


if ssl_lib.SSL_export_keying_material(ssl_ptr,
                                      f_km, len(f_km),
                                      f_label, len(f_label),
                                      f_context, f_context_len, f_use_context):
    print("cffi SSL_export_keying_material", binascii.b2a_hex(bytes(f_km)))
