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

/*
 * Internal error messages used for reporting an unsupported hash algorithm.
 * The algorithm can be given by its name, a callable or a PEP-247 module.
 * The same message is raised by Lib/hashlib.py::__get_builtin_constructor()
 * and _hmacmodule.c::find_hash_info().
 */
#define _Py_HASHLIB_UNSUPPORTED_ALGORITHM       "unsupported hash algorithm %S"
#define _Py_HASHLIB_UNSUPPORTED_STR_ALGORITHM   "unsupported hash algorithm %s"

#define _Py_HASHLIB_MD_OID(ID)                  _Py_hashlib_MD_OID_ ## ID

typedef enum {
    /* MD-family */
    _Py_HASHLIB_MD_OID(md5) = 0,
    /* SHA-1 family */
    _Py_HASHLIB_MD_OID(sha1),
    /* SHA-2 family */
    _Py_HASHLIB_MD_OID(sha224),
    _Py_HASHLIB_MD_OID(sha256),
    _Py_HASHLIB_MD_OID(sha384),
    _Py_HASHLIB_MD_OID(sha512),
    /* Truncated SHA-2 family */
    _Py_HASHLIB_MD_OID(sha512_224),
    _Py_HASHLIB_MD_OID(sha512_256),
    /* SHA-3 family */
    _Py_HASHLIB_MD_OID(sha3_224),
    _Py_HASHLIB_MD_OID(sha3_256),
    _Py_HASHLIB_MD_OID(sha3_384),
    _Py_HASHLIB_MD_OID(sha3_512),
    /* SHA-3 XOF SHAKE family */
    _Py_HASHLIB_MD_OID(shake_128),
    _Py_HASHLIB_MD_OID(shake_256),
    /* BLAKE-2 family */
    _Py_HASHLIB_MD_OID(blake2b),
    _Py_HASHLIB_MD_OID(blake2s),
} _Py_hashlib_MD_OID;

static const char *_Py_HASHLIB_MD_NAMES[] = {
#define DECL_MESSAGE_DIGEST_NAME(ID)  [_Py_HASHLIB_MD_OID(ID)] = #ID
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

static inline const char *
_Py_hashlib_md_name(_Py_hashlib_MD_OID id)
{
    assert(id >= 0);
    assert(id < Py_ARRAY_LENGTH(_Py_HASHLIB_MD_NAMES));
    return _Py_HASHLIB_MD_NAMES[id];
}

#endif // !_HASHLIB_HASHLIB_FETCH_H
