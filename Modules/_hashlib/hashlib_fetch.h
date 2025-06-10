/*
 * Interface for fetching a message digest from a digest-like identifier.
 *
 * The following table summaries the possible algorthms:
 *
 * +----------+--------------+--------------+---------------------------------+
 * | Family   | Algorithm    | Python Name  | Notes                           |
 * +==========+==============+==============+=================================+
 * | MD                                     @                                 |
 * |          +--------------+--------------+---------------------------------+
 * |          | MD5          | "md5"        |                                 |
 * +----------+--------------+--------------+---------------------------------+
 * | SHA1                                   @                                 |
 * |          +--------------+--------------+---------------------------------+
 * |          | SHA1-160     | "sha1"       |                                 |
 * +----------+--------------+--------------+---------------------------------+
 * | SHA2                                   @                                 |
 * |          +--------------+--------------+---------------------------------+
 * |          | SHA2-224     | "sha224"     |                                 |
 * |          | SHA2-256     | "sha256"     |                                 |
 * |          | SHA2-384     | "sha384"     |                                 |
 * |          | SHA2-512     | "sha512"     |                                 |
 * +----------+--------------+--------------+---------------------------------+
 * | SHA2t                                  @ Truncated SHA2-512              |
 * |          +--------------+--------------+---------------------------------+
 * |          | SHA2-512/224 | "sha512_224" |                                 |
 * |          | SHA2-512/256 | "sha512_256" |                                 |
 * +----------+--------------+--------------+---------------------------------+
 * | SHA3                                   @                                 |
 * |          +--------------+--------------+---------------------------------+
 * |          | SHA3-224     | "sha3_224"   |                                 |
 * |          | SHA3-256     | "sha3_256"   |                                 |
 * |          | SHA3-384     | "sha3_384"   |                                 |
 * |          | SHA3-512     | "sha3_512"   |                                 |
 * +----------+--------------+--------------+---------------------------------+
 * | SHA3-XOF                               @ Extensible Output Functions     |
 * |          +--------------+--------------+---------------------------------+
 * |          | SHAKE-128    | "shake_128"  |                                 |
 * |          | SHAKE-256    | "shake_256"  |                                 |
 * +----------+--------------+--------------+---------------------------------+
 * | BLAKE2                                 @                                 |
 * |          +--------------+--------------+---------------------------------+
 * |          | BLAKE2b      | "blake2b"    |                                 |
 * |          | BLAKE2s      | "blake2s"    |                                 |
 * +----------+--------------+--------------+---------------------------------+
 */

#ifndef _HASHLIB_HASHLIB_FETCH_H
#define _HASHLIB_HASHLIB_FETCH_H

#include "Python.h"

#define Py_HASHLIB_MD_NS(ATTR)          Py_hashlib_message_digest_ ## ATTR
#define Py_HASHLIB_MD_FAMILY(FAMILY_ID) Py_HASHLIB_MD_NS(family_ ## FAMILY_ID)
#define Py_HASHLIB_MD_MEMBER(MEMBER_ID) Py_HASHLIB_MD_NS(member_ ## MEMBER_ID)

#define Py_HASHLIB_MD_NAMES             Py_HASHLIB_MD_NS(NAMES)
#define Py_HASHLIB_MD_COUNT             Py_ARRAY_LENGTH(Py_HASHLIB_MD_NAMES)
#define Py_HASHLIB_MD_NAME(MEMBER_ID)                                   \
    (                                                                   \
        assert(Py_HASHLIB_MD_NAME(MEMBER_ID) < Py_HASHLIB_MD_COUNT),    \
        Py_HASHLIB_MD_NAMES[Py_HASHLIB_MD_MEMBER(MEMBER_ID)]            \
    )

typedef enum {
    Py_HASHLIB_MD_FAMILY(MD) = 0,
    Py_HASHLIB_MD_FAMILY(SHA1),
    Py_HASHLIB_MD_FAMILY(SHA2),
    Py_HASHLIB_MD_FAMILY(SHA2t),
    Py_HASHLIB_MD_FAMILY(SHA3),
    Py_HASHLIB_MD_FAMILY(SHA3_XOF),
    Py_HASHLIB_MD_FAMILY(BLAKE2),
} Py_HASHLIB_MD_NS(family);

typedef enum {
    /* MD-family */
    Py_HASHLIB_MD_MEMBER(md5) = 0,
    /* SHA-1 family */
    Py_HASHLIB_MD_MEMBER(sha1),
    /* SHA-2 family */
    Py_HASHLIB_MD_MEMBER(sha224),
    Py_HASHLIB_MD_MEMBER(sha256),
    Py_HASHLIB_MD_MEMBER(sha384),
    Py_HASHLIB_MD_MEMBER(sha512),
    /* Truncated SHA-2 family */
    Py_HASHLIB_MD_MEMBER(sha512_224),
    Py_HASHLIB_MD_MEMBER(sha512_256),
    /* SHA-3 family */
    Py_HASHLIB_MD_MEMBER(sha3_224),
    Py_HASHLIB_MD_MEMBER(sha3_256),
    Py_HASHLIB_MD_MEMBER(sha3_384),
    Py_HASHLIB_MD_MEMBER(sha3_512),
    /* SHA-3 XOF SHAKE family */
    Py_HASHLIB_MD_MEMBER(shake_128),
    Py_HASHLIB_MD_MEMBER(shake_256),
    /* BLAKE-2 family */
    Py_HASHLIB_MD_MEMBER(blake2b),
    Py_HASHLIB_MD_MEMBER(blake2s),
} Py_HASHLIB_MD_NS(member);

static const char *Py_HASHLIB_MD_NAMES[] = {
#define DECL_MESSAGE_DIGEST_NAME(ID)  [Py_HASHLIB_MD_MEMBER(ID)] = #ID
    /* MD-family */
    DECL_MESSAGE_DIGEST_NAME(md5),
    /* SHA-1 family */
    DECL_MESSAGE_DIGEST_NAME(sha1),
    /* SHA-2 family */
    DECL_MESSAGE_DIGEST_NAME(sha224),
    DECL_MESSAGE_DIGEST_NAME(sha256),
    DECL_MESSAGE_DIGEST_NAME(sha384),
    DECL_MESSAGE_DIGEST_NAME(sha512),
    /* Truncated SHA-2 family */
    DECL_MESSAGE_DIGEST_NAME(sha512_224),
    DECL_MESSAGE_DIGEST_NAME(sha512_256),
    /* SHA-3 family */
    DECL_MESSAGE_DIGEST_NAME(sha3_224),
    DECL_MESSAGE_DIGEST_NAME(sha3_256),
    DECL_MESSAGE_DIGEST_NAME(sha3_384),
    DECL_MESSAGE_DIGEST_NAME(sha3_512),
    /* SHA-3 XOF SHAKE family */
    DECL_MESSAGE_DIGEST_NAME(shake_128),
    DECL_MESSAGE_DIGEST_NAME(shake_256),
    /* BLAKE-2 family */
    DECL_MESSAGE_DIGEST_NAME(blake2b),
    DECL_MESSAGE_DIGEST_NAME(blake2s),
#undef DECL_MESSAGE_DIGEST_NAME
    NULL /* sentinel */
};

#endif // !_HASHLIB_HASHLIB_FETCH_H
