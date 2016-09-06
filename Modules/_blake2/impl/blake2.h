/*
   BLAKE2 reference source code package - reference C implementations
  
   Copyright 2012, Samuel Neves <sneves@dei.uc.pt>.  You may use this under the
   terms of the CC0, the OpenSSL Licence, or the Apache Public License 2.0, at
   your option.  The terms of these licenses can be found at:
  
   - CC0 1.0 Universal : http://creativecommons.org/publicdomain/zero/1.0
   - OpenSSL license   : https://www.openssl.org/source/license.html
   - Apache 2.0        : http://www.apache.org/licenses/LICENSE-2.0
  
   More information about the BLAKE2 hash function can be found at
   https://blake2.net.
*/
#pragma once
#ifndef __BLAKE2_H__
#define __BLAKE2_H__

#include <stddef.h>
#include <stdint.h>

#ifdef BLAKE2_NO_INLINE
#define BLAKE2_LOCAL_INLINE(type) static type
#endif

#ifndef BLAKE2_LOCAL_INLINE
#define BLAKE2_LOCAL_INLINE(type) static inline type
#endif

#if defined(__cplusplus)
extern "C" {
#endif

  enum blake2s_constant
  {
    BLAKE2S_BLOCKBYTES = 64,
    BLAKE2S_OUTBYTES   = 32,
    BLAKE2S_KEYBYTES   = 32,
    BLAKE2S_SALTBYTES  = 8,
    BLAKE2S_PERSONALBYTES = 8
  };

  enum blake2b_constant
  {
    BLAKE2B_BLOCKBYTES = 128,
    BLAKE2B_OUTBYTES   = 64,
    BLAKE2B_KEYBYTES   = 64,
    BLAKE2B_SALTBYTES  = 16,
    BLAKE2B_PERSONALBYTES = 16
  };

  typedef struct __blake2s_state
  {
    uint32_t h[8];
    uint32_t t[2];
    uint32_t f[2];
    uint8_t  buf[2 * BLAKE2S_BLOCKBYTES];
    size_t   buflen;
    uint8_t  last_node;
  } blake2s_state;

  typedef struct __blake2b_state
  {
    uint64_t h[8];
    uint64_t t[2];
    uint64_t f[2];
    uint8_t  buf[2 * BLAKE2B_BLOCKBYTES];
    size_t   buflen;
    uint8_t  last_node;
  } blake2b_state;

  typedef struct __blake2sp_state
  {
    blake2s_state S[8][1];
    blake2s_state R[1];
    uint8_t buf[8 * BLAKE2S_BLOCKBYTES];
    size_t  buflen;
  } blake2sp_state;

  typedef struct __blake2bp_state
  {
    blake2b_state S[4][1];
    blake2b_state R[1];
    uint8_t buf[4 * BLAKE2B_BLOCKBYTES];
    size_t  buflen;
  } blake2bp_state;


#pragma pack(push, 1)
  typedef struct __blake2s_param
  {
    uint8_t  digest_length; /* 1 */
    uint8_t  key_length;    /* 2 */
    uint8_t  fanout;        /* 3 */
    uint8_t  depth;         /* 4 */
    uint32_t leaf_length;   /* 8 */
    uint8_t  node_offset[6];// 14
    uint8_t  node_depth;    /* 15 */
    uint8_t  inner_length;  /* 16 */
    /* uint8_t  reserved[0]; */
    uint8_t  salt[BLAKE2S_SALTBYTES]; /* 24 */
    uint8_t  personal[BLAKE2S_PERSONALBYTES];  /* 32 */
  } blake2s_param;

  typedef struct __blake2b_param
  {
    uint8_t  digest_length; /* 1 */
    uint8_t  key_length;    /* 2 */
    uint8_t  fanout;        /* 3 */
    uint8_t  depth;         /* 4 */
    uint32_t leaf_length;   /* 8 */
    uint64_t node_offset;   /* 16 */
    uint8_t  node_depth;    /* 17 */
    uint8_t  inner_length;  /* 18 */
    uint8_t  reserved[14];  /* 32 */
    uint8_t  salt[BLAKE2B_SALTBYTES]; /* 48 */
    uint8_t  personal[BLAKE2B_PERSONALBYTES];  /* 64 */
  } blake2b_param;
#pragma pack(pop)

  /* Streaming API */
  int blake2s_init( blake2s_state *S, const uint8_t outlen );
  int blake2s_init_key( blake2s_state *S, const uint8_t outlen, const void *key, const uint8_t keylen );
  int blake2s_init_param( blake2s_state *S, const blake2s_param *P );
  int blake2s_update( blake2s_state *S, const uint8_t *in, uint64_t inlen );
  int blake2s_final( blake2s_state *S, uint8_t *out, uint8_t outlen );

  int blake2b_init( blake2b_state *S, const uint8_t outlen );
  int blake2b_init_key( blake2b_state *S, const uint8_t outlen, const void *key, const uint8_t keylen );
  int blake2b_init_param( blake2b_state *S, const blake2b_param *P );
  int blake2b_update( blake2b_state *S, const uint8_t *in, uint64_t inlen );
  int blake2b_final( blake2b_state *S, uint8_t *out, uint8_t outlen );

  int blake2sp_init( blake2sp_state *S, const uint8_t outlen );
  int blake2sp_init_key( blake2sp_state *S, const uint8_t outlen, const void *key, const uint8_t keylen );
  int blake2sp_update( blake2sp_state *S, const uint8_t *in, uint64_t inlen );
  int blake2sp_final( blake2sp_state *S, uint8_t *out, uint8_t outlen );

  int blake2bp_init( blake2bp_state *S, const uint8_t outlen );
  int blake2bp_init_key( blake2bp_state *S, const uint8_t outlen, const void *key, const uint8_t keylen );
  int blake2bp_update( blake2bp_state *S, const uint8_t *in, uint64_t inlen );
  int blake2bp_final( blake2bp_state *S, uint8_t *out, uint8_t outlen );

  /* Simple API */
  int blake2s( uint8_t *out, const void *in, const void *key, const uint8_t outlen, const uint64_t inlen, uint8_t keylen );
  int blake2b( uint8_t *out, const void *in, const void *key, const uint8_t outlen, const uint64_t inlen, uint8_t keylen );

  int blake2sp( uint8_t *out, const void *in, const void *key, const uint8_t outlen, const uint64_t inlen, uint8_t keylen );
  int blake2bp( uint8_t *out, const void *in, const void *key, const uint8_t outlen, const uint64_t inlen, uint8_t keylen );

  static inline int blake2( uint8_t *out, const void *in, const void *key, const uint8_t outlen, const uint64_t inlen, uint8_t keylen )
  {
    return blake2b( out, in, key, outlen, inlen, keylen );
  }

#if defined(__cplusplus)
}
#endif

#endif

