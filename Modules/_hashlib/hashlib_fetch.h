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

#endif // !_HASHLIB_HASHLIB_FETCH_H
