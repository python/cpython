#ifndef _PYTHON_HACL_NAMESPACES_H
#define _PYTHON_HACL_NAMESPACES_H

/*
 * Use globally unique names to avoid linkage conflicts with builds linking
 * or dynamically loading other code potentially using HACL* libraries.
 *
 * Assuming that the current working directory is Modules/_hacl,
 * use the following command to generate a list of candidates:

    nm -j *.o | grep -i hacl | grep -P '^[a-zA-Z_][a-zA-Z0-9_]+' \
        | sed -e 's/^_Py_LibHacl_//g' \
        | sed 's/\(.*\)/#define \1 _Py_LibHacl_\1/g' \
        | sort -u

 * Compare the entries to add as follows:

    diff -y --suppress-common-lines \
        <(grep -P '^#define (?!_PY.+_H)' python_hacl_namespaces.h | sort -u) \
        <(nm -j *.o | grep -i hacl | grep -P '^[a-zA-Z_][a-zA-Z0-9_]+' \
            | sed -e 's/^_Py_LibHacl_//g' \
            | sed 's/\(.*\)/#define \1 _Py_LibHacl_\1/g' | sort -u)
 */

// --- Utils ------------------------------------------------------------------
#define Lib_Memzero0_memzero0 _Py_LibHacl_Lib_Memzero0_memzero0
// --- HASH-BLAKE-2b ----------------------------------------------------------
#define Hacl_Hash_Blake2b_copy _Py_LibHacl_Hacl_Hash_Blake2b_copy
#define Hacl_Hash_Blake2b_digest _Py_LibHacl_Hacl_Hash_Blake2b_digest
#define Hacl_Hash_Blake2b_finish _Py_LibHacl_Hacl_Hash_Blake2b_finish
#define Hacl_Hash_Blake2b_free _Py_LibHacl_Hacl_Hash_Blake2b_free
#define Hacl_Hash_Blake2b_hash_with_key _Py_LibHacl_Hacl_Hash_Blake2b_hash_with_key
#define Hacl_Hash_Blake2b_hash_with_key_and_params _Py_LibHacl_Hacl_Hash_Blake2b_hash_with_key_and_params
#define Hacl_Hash_Blake2b_info _Py_LibHacl_Hacl_Hash_Blake2b_info
#define Hacl_Hash_Blake2b_init _Py_LibHacl_Hacl_Hash_Blake2b_init
#define Hacl_Hash_Blake2b_malloc _Py_LibHacl_Hacl_Hash_Blake2b_malloc
#define Hacl_Hash_Blake2b_malloc_with_key _Py_LibHacl_Hacl_Hash_Blake2b_malloc_with_key
#define Hacl_Hash_Blake2b_malloc_with_params_and_key _Py_LibHacl_Hacl_Hash_Blake2b_malloc_with_params_and_key
#define Hacl_Hash_Blake2b_reset _Py_LibHacl_Hacl_Hash_Blake2b_reset
#define Hacl_Hash_Blake2b_reset_with_key _Py_LibHacl_Hacl_Hash_Blake2b_reset_with_key
#define Hacl_Hash_Blake2b_reset_with_key_and_params _Py_LibHacl_Hacl_Hash_Blake2b_reset_with_key_and_params
#define Hacl_Hash_Blake2b_Simd256_copy _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_copy
#define Hacl_Hash_Blake2b_Simd256_copy_internal_state _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_copy_internal_state
#define Hacl_Hash_Blake2b_Simd256_digest _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_digest
#define Hacl_Hash_Blake2b_Simd256_finish _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_finish
#define Hacl_Hash_Blake2b_Simd256_free _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_free
#define Hacl_Hash_Blake2b_Simd256_hash_with_key _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_hash_with_key
#define Hacl_Hash_Blake2b_Simd256_hash_with_key_and_params _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_hash_with_key_and_params
#define Hacl_Hash_Blake2b_Simd256_info _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_info
#define Hacl_Hash_Blake2b_Simd256_init _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_init
#define Hacl_Hash_Blake2b_Simd256_load_state256b_from_state32 _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_load_state256b_from_state32
#define Hacl_Hash_Blake2b_Simd256_malloc _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_malloc
#define Hacl_Hash_Blake2b_Simd256_malloc_internal_state_with_key _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_malloc_internal_state_with_key
#define Hacl_Hash_Blake2b_Simd256_malloc_with_key _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_malloc_with_key
#define Hacl_Hash_Blake2b_Simd256_malloc_with_params_and_key _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_malloc_with_params_and_key
#define Hacl_Hash_Blake2b_Simd256_reset _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_reset
#define Hacl_Hash_Blake2b_Simd256_reset_with_key _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_reset_with_key
#define Hacl_Hash_Blake2b_Simd256_reset_with_key_and_params _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_reset_with_key_and_params
#define Hacl_Hash_Blake2b_Simd256_store_state256b_to_state32 _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_store_state256b_to_state32
#define Hacl_Hash_Blake2b_Simd256_update _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_update
#define Hacl_Hash_Blake2b_Simd256_update_last _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_update_last
#define Hacl_Hash_Blake2b_Simd256_update_last_no_inline _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_update_last_no_inline
#define Hacl_Hash_Blake2b_Simd256_update_multi _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_update_multi
#define Hacl_Hash_Blake2b_Simd256_update_multi_no_inline _Py_LibHacl_Hacl_Hash_Blake2b_Simd256_update_multi_no_inline
#define Hacl_Hash_Blake2b_update _Py_LibHacl_Hacl_Hash_Blake2b_update
#define Hacl_Hash_Blake2b_update_last _Py_LibHacl_Hacl_Hash_Blake2b_update_last
#define Hacl_Hash_Blake2b_update_multi _Py_LibHacl_Hacl_Hash_Blake2b_update_multi
// --- HASH-BLAKE-2s ----------------------------------------------------------
#define Hacl_Hash_Blake2s_copy _Py_LibHacl_Hacl_Hash_Blake2s_copy
#define Hacl_Hash_Blake2s_digest _Py_LibHacl_Hacl_Hash_Blake2s_digest
#define Hacl_Hash_Blake2s_finish _Py_LibHacl_Hacl_Hash_Blake2s_finish
#define Hacl_Hash_Blake2s_free _Py_LibHacl_Hacl_Hash_Blake2s_free
#define Hacl_Hash_Blake2s_hash_with_key _Py_LibHacl_Hacl_Hash_Blake2s_hash_with_key
#define Hacl_Hash_Blake2s_hash_with_key_and_params _Py_LibHacl_Hacl_Hash_Blake2s_hash_with_key_and_params
#define Hacl_Hash_Blake2s_info _Py_LibHacl_Hacl_Hash_Blake2s_info
#define Hacl_Hash_Blake2s_init _Py_LibHacl_Hacl_Hash_Blake2s_init
#define Hacl_Hash_Blake2s_malloc _Py_LibHacl_Hacl_Hash_Blake2s_malloc
#define Hacl_Hash_Blake2s_malloc_with_key _Py_LibHacl_Hacl_Hash_Blake2s_malloc_with_key
#define Hacl_Hash_Blake2s_malloc_with_params_and_key _Py_LibHacl_Hacl_Hash_Blake2s_malloc_with_params_and_key
#define Hacl_Hash_Blake2s_reset _Py_LibHacl_Hacl_Hash_Blake2s_reset
#define Hacl_Hash_Blake2s_reset_with_key _Py_LibHacl_Hacl_Hash_Blake2s_reset_with_key
#define Hacl_Hash_Blake2s_reset_with_key_and_params _Py_LibHacl_Hacl_Hash_Blake2s_reset_with_key_and_params
#define Hacl_Hash_Blake2s_Simd128_copy _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_copy
#define Hacl_Hash_Blake2s_Simd128_copy_internal_state _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_copy_internal_state
#define Hacl_Hash_Blake2s_Simd128_digest _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_digest
#define Hacl_Hash_Blake2s_Simd128_finish _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_finish
#define Hacl_Hash_Blake2s_Simd128_free _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_free
#define Hacl_Hash_Blake2s_Simd128_hash_with_key _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_hash_with_key
#define Hacl_Hash_Blake2s_Simd128_hash_with_key_and_params _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_hash_with_key_and_params
#define Hacl_Hash_Blake2s_Simd128_info _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_info
#define Hacl_Hash_Blake2s_Simd128_init _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_init
#define Hacl_Hash_Blake2s_Simd128_load_state128s_from_state32 _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_load_state128s_from_state32
#define Hacl_Hash_Blake2s_Simd128_malloc _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_malloc
#define Hacl_Hash_Blake2s_Simd128_malloc_internal_state_with_key _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_malloc_internal_state_with_key
#define Hacl_Hash_Blake2s_Simd128_malloc_with_key _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_malloc_with_key
#define Hacl_Hash_Blake2s_Simd128_malloc_with_params_and_key _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_malloc_with_params_and_key
#define Hacl_Hash_Blake2s_Simd128_reset _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_reset
#define Hacl_Hash_Blake2s_Simd128_reset_with_key _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_reset_with_key
#define Hacl_Hash_Blake2s_Simd128_reset_with_key_and_params _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_reset_with_key_and_params
#define Hacl_Hash_Blake2s_Simd128_store_state128s_to_state32 _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_store_state128s_to_state32
#define Hacl_Hash_Blake2s_Simd128_update _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_update
#define Hacl_Hash_Blake2s_Simd128_update_last _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_update_last
#define Hacl_Hash_Blake2s_Simd128_update_last_no_inline _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_update_last_no_inline
#define Hacl_Hash_Blake2s_Simd128_update_multi _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_update_multi
#define Hacl_Hash_Blake2s_Simd128_update_multi_no_inline _Py_LibHacl_Hacl_Hash_Blake2s_Simd128_update_multi_no_inline
#define Hacl_Hash_Blake2s_update _Py_LibHacl_Hacl_Hash_Blake2s_update
#define Hacl_Hash_Blake2s_update_last _Py_LibHacl_Hacl_Hash_Blake2s_update_last
#define Hacl_Hash_Blake2s_update_multi _Py_LibHacl_Hacl_Hash_Blake2s_update_multi
// --- HASH-MD5 ---------------------------------------------------------------
#define Hacl_Hash_MD5_copy _Py_LibHacl_Hacl_Hash_MD5_copy
#define Hacl_Hash_MD5_digest _Py_LibHacl_Hacl_Hash_MD5_digest
#define Hacl_Hash_MD5_finish _Py_LibHacl_Hacl_Hash_MD5_finish
#define Hacl_Hash_MD5_free _Py_LibHacl_Hacl_Hash_MD5_free
#define Hacl_Hash_MD5_hash _Py_LibHacl_Hacl_Hash_MD5_hash
#define Hacl_Hash_MD5_hash_oneshot _Py_LibHacl_Hacl_Hash_MD5_hash_oneshot
#define Hacl_Hash_MD5_init _Py_LibHacl_Hacl_Hash_MD5_init
#define Hacl_Hash_MD5_malloc _Py_LibHacl_Hacl_Hash_MD5_malloc
#define Hacl_Hash_MD5_reset _Py_LibHacl_Hacl_Hash_MD5_reset
#define Hacl_Hash_MD5_update _Py_LibHacl_Hacl_Hash_MD5_update
#define Hacl_Hash_MD5_update_last _Py_LibHacl_Hacl_Hash_MD5_update_last
#define Hacl_Hash_MD5_update_multi _Py_LibHacl_Hacl_Hash_MD5_update_multi
// --- HASH-SHA-1 -------------------------------------------------------------
#define Hacl_Hash_SHA1_copy _Py_LibHacl_Hacl_Hash_SHA1_copy
#define Hacl_Hash_SHA1_digest _Py_LibHacl_Hacl_Hash_SHA1_digest
#define Hacl_Hash_SHA1_finish _Py_LibHacl_Hacl_Hash_SHA1_finish
#define Hacl_Hash_SHA1_free _Py_LibHacl_Hacl_Hash_SHA1_free
#define Hacl_Hash_SHA1_hash _Py_LibHacl_Hacl_Hash_SHA1_hash
#define Hacl_Hash_SHA1_hash_oneshot _Py_LibHacl_Hacl_Hash_SHA1_hash_oneshot
#define Hacl_Hash_SHA1_init _Py_LibHacl_Hacl_Hash_SHA1_init
#define Hacl_Hash_SHA1_malloc _Py_LibHacl_Hacl_Hash_SHA1_malloc
#define Hacl_Hash_SHA1_reset _Py_LibHacl_Hacl_Hash_SHA1_reset
#define Hacl_Hash_SHA1_update _Py_LibHacl_Hacl_Hash_SHA1_update
#define Hacl_Hash_SHA1_update_last _Py_LibHacl_Hacl_Hash_SHA1_update_last
#define Hacl_Hash_SHA1_update_multi _Py_LibHacl_Hacl_Hash_SHA1_update_multi
// --- HASH-SHA-2 -------------------------------------------------------------
#define Hacl_Hash_SHA2_copy_256 _Py_LibHacl_Hacl_Hash_SHA2_copy_256
#define Hacl_Hash_SHA2_copy_512 _Py_LibHacl_Hacl_Hash_SHA2_copy_512
#define Hacl_Hash_SHA2_digest_224 _Py_LibHacl_Hacl_Hash_SHA2_digest_224
#define Hacl_Hash_SHA2_digest_256 _Py_LibHacl_Hacl_Hash_SHA2_digest_256
#define Hacl_Hash_SHA2_digest_384 _Py_LibHacl_Hacl_Hash_SHA2_digest_384
#define Hacl_Hash_SHA2_digest_512 _Py_LibHacl_Hacl_Hash_SHA2_digest_512
#define Hacl_Hash_SHA2_free_224 _Py_LibHacl_Hacl_Hash_SHA2_free_224
#define Hacl_Hash_SHA2_free_256 _Py_LibHacl_Hacl_Hash_SHA2_free_256
#define Hacl_Hash_SHA2_free_384 _Py_LibHacl_Hacl_Hash_SHA2_free_384
#define Hacl_Hash_SHA2_free_512 _Py_LibHacl_Hacl_Hash_SHA2_free_512
#define Hacl_Hash_SHA2_hash_224 _Py_LibHacl_Hacl_Hash_SHA2_hash_224
#define Hacl_Hash_SHA2_hash_256 _Py_LibHacl_Hacl_Hash_SHA2_hash_256
#define Hacl_Hash_SHA2_hash_384 _Py_LibHacl_Hacl_Hash_SHA2_hash_384
#define Hacl_Hash_SHA2_hash_512 _Py_LibHacl_Hacl_Hash_SHA2_hash_512
#define Hacl_Hash_SHA2_malloc_224 _Py_LibHacl_Hacl_Hash_SHA2_malloc_224
#define Hacl_Hash_SHA2_malloc_256 _Py_LibHacl_Hacl_Hash_SHA2_malloc_256
#define Hacl_Hash_SHA2_malloc_384 _Py_LibHacl_Hacl_Hash_SHA2_malloc_384
#define Hacl_Hash_SHA2_malloc_512 _Py_LibHacl_Hacl_Hash_SHA2_malloc_512
#define Hacl_Hash_SHA2_reset_224 _Py_LibHacl_Hacl_Hash_SHA2_reset_224
#define Hacl_Hash_SHA2_reset_256 _Py_LibHacl_Hacl_Hash_SHA2_reset_256
#define Hacl_Hash_SHA2_reset_384 _Py_LibHacl_Hacl_Hash_SHA2_reset_384
#define Hacl_Hash_SHA2_reset_512 _Py_LibHacl_Hacl_Hash_SHA2_reset_512
#define Hacl_Hash_SHA2_sha224_finish _Py_LibHacl_Hacl_Hash_SHA2_sha224_finish
#define Hacl_Hash_SHA2_sha224_init _Py_LibHacl_Hacl_Hash_SHA2_sha224_init
#define Hacl_Hash_SHA2_sha224_update_last _Py_LibHacl_Hacl_Hash_SHA2_sha224_update_last
#define Hacl_Hash_SHA2_sha224_update_nblocks _Py_LibHacl_Hacl_Hash_SHA2_sha224_update_nblocks
#define Hacl_Hash_SHA2_sha256_finish _Py_LibHacl_Hacl_Hash_SHA2_sha256_finish
#define Hacl_Hash_SHA2_sha256_init _Py_LibHacl_Hacl_Hash_SHA2_sha256_init
#define Hacl_Hash_SHA2_sha256_update_last _Py_LibHacl_Hacl_Hash_SHA2_sha256_update_last
#define Hacl_Hash_SHA2_sha256_update_nblocks _Py_LibHacl_Hacl_Hash_SHA2_sha256_update_nblocks
#define Hacl_Hash_SHA2_sha384_finish _Py_LibHacl_Hacl_Hash_SHA2_sha384_finish
#define Hacl_Hash_SHA2_sha384_init _Py_LibHacl_Hacl_Hash_SHA2_sha384_init
#define Hacl_Hash_SHA2_sha384_update_last _Py_LibHacl_Hacl_Hash_SHA2_sha384_update_last
#define Hacl_Hash_SHA2_sha384_update_nblocks _Py_LibHacl_Hacl_Hash_SHA2_sha384_update_nblocks
#define Hacl_Hash_SHA2_sha512_finish _Py_LibHacl_Hacl_Hash_SHA2_sha512_finish
#define Hacl_Hash_SHA2_sha512_init _Py_LibHacl_Hacl_Hash_SHA2_sha512_init
#define Hacl_Hash_SHA2_sha512_update_last _Py_LibHacl_Hacl_Hash_SHA2_sha512_update_last
#define Hacl_Hash_SHA2_sha512_update_nblocks _Py_LibHacl_Hacl_Hash_SHA2_sha512_update_nblocks
#define Hacl_Hash_SHA2_update_224 _Py_LibHacl_Hacl_Hash_SHA2_update_224
#define Hacl_Hash_SHA2_update_256 _Py_LibHacl_Hacl_Hash_SHA2_update_256
#define Hacl_Hash_SHA2_update_384 _Py_LibHacl_Hacl_Hash_SHA2_update_384
#define Hacl_Hash_SHA2_update_512 _Py_LibHacl_Hacl_Hash_SHA2_update_512
// --- HASH-SHA-3 -------------------------------------------------------------
#define Hacl_Hash_SHA3_absorb_inner_32 _Py_LibHacl_Hacl_Hash_SHA3_absorb_inner_32
#define Hacl_Hash_SHA3_block_len _Py_LibHacl_Hacl_Hash_SHA3_block_len
#define Hacl_Hash_SHA3_copy _Py_LibHacl_Hacl_Hash_SHA3_copy
#define Hacl_Hash_SHA3_digest _Py_LibHacl_Hacl_Hash_SHA3_digest
#define Hacl_Hash_SHA3_free _Py_LibHacl_Hacl_Hash_SHA3_free
#define Hacl_Hash_SHA3_get_alg _Py_LibHacl_Hacl_Hash_SHA3_get_alg
#define Hacl_Hash_SHA3_hash_len _Py_LibHacl_Hacl_Hash_SHA3_hash_len
#define Hacl_Hash_SHA3_init_ _Py_LibHacl_Hacl_Hash_SHA3_init_
#define Hacl_Hash_SHA3_is_shake _Py_LibHacl_Hacl_Hash_SHA3_is_shake
#define Hacl_Hash_SHA3_keccak_piln _Py_LibHacl_Hacl_Hash_SHA3_keccak_piln
#define Hacl_Hash_SHA3_keccak_rndc _Py_LibHacl_Hacl_Hash_SHA3_keccak_rndc
#define Hacl_Hash_SHA3_keccak_rotc _Py_LibHacl_Hacl_Hash_SHA3_keccak_rotc
#define Hacl_Hash_SHA3_malloc _Py_LibHacl_Hacl_Hash_SHA3_malloc
#define Hacl_Hash_SHA3_reset _Py_LibHacl_Hacl_Hash_SHA3_reset
#define Hacl_Hash_SHA3_sha3_224 _Py_LibHacl_Hacl_Hash_SHA3_sha3_224
#define Hacl_Hash_SHA3_sha3_256 _Py_LibHacl_Hacl_Hash_SHA3_sha3_256
#define Hacl_Hash_SHA3_sha3_384 _Py_LibHacl_Hacl_Hash_SHA3_sha3_384
#define Hacl_Hash_SHA3_sha3_512 _Py_LibHacl_Hacl_Hash_SHA3_sha3_512
#define Hacl_Hash_SHA3_shake128 _Py_LibHacl_Hacl_Hash_SHA3_shake128
#define Hacl_Hash_SHA3_shake128_absorb_final _Py_LibHacl_Hacl_Hash_SHA3_shake128_absorb_final
#define Hacl_Hash_SHA3_shake128_absorb_nblocks _Py_LibHacl_Hacl_Hash_SHA3_shake128_absorb_nblocks
#define Hacl_Hash_SHA3_shake128_squeeze_nblocks _Py_LibHacl_Hacl_Hash_SHA3_shake128_squeeze_nblocks
#define Hacl_Hash_SHA3_shake256 _Py_LibHacl_Hacl_Hash_SHA3_shake256
#define Hacl_Hash_SHA3_squeeze _Py_LibHacl_Hacl_Hash_SHA3_squeeze
#define Hacl_Hash_SHA3_state_free _Py_LibHacl_Hacl_Hash_SHA3_state_free
#define Hacl_Hash_SHA3_state_malloc _Py_LibHacl_Hacl_Hash_SHA3_state_malloc
#define Hacl_Hash_SHA3_update _Py_LibHacl_Hacl_Hash_SHA3_update
#define Hacl_Hash_SHA3_update_last_sha3 _Py_LibHacl_Hacl_Hash_SHA3_update_last_sha3
#define Hacl_Hash_SHA3_update_multi_sha3 _Py_LibHacl_Hacl_Hash_SHA3_update_multi_sha3
// --- STREAMING-MAC ----------------------------------------------------------
#define Hacl_Streaming_HMAC_copy _Py_LibHacl_Hacl_Streaming_HMAC_copy
#define Hacl_Streaming_HMAC_digest _Py_LibHacl_Hacl_Streaming_HMAC_digest
#define Hacl_Streaming_HMAC_free _Py_LibHacl_Hacl_Streaming_HMAC_free
#define Hacl_Streaming_HMAC_get_impl _Py_LibHacl_Hacl_Streaming_HMAC_get_impl
#define Hacl_Streaming_HMAC_index_of_state _Py_LibHacl_Hacl_Streaming_HMAC_index_of_state
#define Hacl_Streaming_HMAC_malloc_ _Py_LibHacl_Hacl_Streaming_HMAC_malloc_
#define Hacl_Streaming_HMAC_reset _Py_LibHacl_Hacl_Streaming_HMAC_reset
#define Hacl_Streaming_HMAC_s1 _Py_LibHacl_Hacl_Streaming_HMAC_s1
#define Hacl_Streaming_HMAC_s2 _Py_LibHacl_Hacl_Streaming_HMAC_s2
#define Hacl_Streaming_HMAC_update _Py_LibHacl_Hacl_Streaming_HMAC_update
// --- HMAC-MD5 ---------------------------------------------------------------
#define Hacl_HMAC_compute_md5 _Py_LibHacl_Hacl_HMAC_compute_md5
// --- HMAC-SHA-1 -------------------------------------------------------------
#define Hacl_HMAC_compute_sha1 _Py_LibHacl_Hacl_HMAC_compute_sha1
// --- HMAC-SHA-2 -------------------------------------------------------------
#define Hacl_HMAC_compute_sha2_224 _Py_LibHacl_Hacl_HMAC_compute_sha2_224
#define Hacl_HMAC_compute_sha2_256 _Py_LibHacl_Hacl_HMAC_compute_sha2_256
#define Hacl_HMAC_compute_sha2_384 _Py_LibHacl_Hacl_HMAC_compute_sha2_384
#define Hacl_HMAC_compute_sha2_512 _Py_LibHacl_Hacl_HMAC_compute_sha2_512
// --- HMAC-SHA-3 -------------------------------------------------------------
#define Hacl_HMAC_compute_sha3_224 _Py_LibHacl_Hacl_HMAC_compute_sha3_224
#define Hacl_HMAC_compute_sha3_256 _Py_LibHacl_Hacl_HMAC_compute_sha3_256
#define Hacl_HMAC_compute_sha3_384 _Py_LibHacl_Hacl_HMAC_compute_sha3_384
#define Hacl_HMAC_compute_sha3_512 _Py_LibHacl_Hacl_HMAC_compute_sha3_512
// --- HMAC-BLAKE-2 -----------------------------------------------------------
#define Hacl_HMAC_compute_blake2b_32 _Py_LibHacl_Hacl_HMAC_compute_blake2b_32
#define Hacl_HMAC_compute_blake2s_32 _Py_LibHacl_Hacl_HMAC_compute_blake2s_32

#endif  // _PYTHON_HACL_NAMESPACES_H
