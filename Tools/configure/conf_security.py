"""conf_security — getrandom, OpenSSL, hash algorithm, and HACL* modules.

Checks for the Linux getrandom() syscall and getrandom() libc function;
handles --with-openssl and --with-openssl-rpath including pkg-config
detection and version validation; handles --with-ssl-default-suites;
handles --with-builtin-hashlib-hashes; handles --with-hash-algorithm;
and sets up HACL* compilation flags and HACL*-based hash modules.
"""

from __future__ import annotations

import pyconf

WITH_OPENSSL = pyconf.arg_with(
    "openssl", metavar="DIR", help="root of the OpenSSL directory"
)
WITH_OPENSSL_RPATH = pyconf.arg_with(
    "openssl-rpath",
    metavar="DIR|auto|no",
    help="Set runtime library directory (rpath) for OpenSSL libraries, "
    "no (default): don't set rpath, auto: auto-detect from --with-openssl "
    "and pkg-config, DIR: set an explicit rpath",
)
WITH_SSL_DEFAULT_SUITES = pyconf.arg_with(
    "ssl-default-suites",
    metavar="python|openssl|STRING",
    help="override default cipher suites string, python: use Python's preferred "
    "selection (default), openssl: leave OpenSSL's defaults untouched, "
    "STRING: use a custom string",
)
WITH_BUILTIN_HASHLIB_HASHES = pyconf.arg_with(
    "builtin-hashlib-hashes",
    metavar="md5,sha1,sha2,sha3,blake2",
    help="builtin hash modules, md5, sha1, sha2, sha3 (with shake), blake2",
)


def check_getrandom(v):
    # ---------------------------------------------------------------------------
    # getrandom syscall (Linux)
    # ---------------------------------------------------------------------------

    if pyconf.link_check(
        "for the Linux getrandom() syscall",
        """
#include <stddef.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>
int main(void) {
    char buffer[1];
    const size_t buflen = sizeof(buffer);
    const int flags = GRND_NONBLOCK;
    (void)syscall(SYS_getrandom, buffer, buflen, flags);
    return 0;
}
""",
    ):
        pyconf.define(
            "HAVE_GETRANDOM_SYSCALL",
            1,
            "Define to 1 if the Linux getrandom() syscall is available",
        )

    # getrandom() libc function (Solaris/glibc)
    # the test was written for the Solaris function of <sys/random.h>
    if pyconf.link_check(
        "for the getrandom() function",
        """
#include <stddef.h>
#include <sys/random.h>
int main(void) {
    char buffer[1];
    const size_t buflen = sizeof(buffer);
    const int flags = 0;
    (void)getrandom(buffer, buflen, flags);
    return 0;
}
""",
    ):
        pyconf.define(
            "HAVE_GETRANDOM",
            1,
            "Define to 1 if the getrandom() function is available",
        )


def check_openssl(v):
    # ---------------------------------------------------------------------------
    # OpenSSL
    # ---------------------------------------------------------------------------

    # AX_CHECK_OPENSSL: detect OpenSSL via --with-openssl or pkg-config.
    # Sets OPENSSL_INCLUDES, OPENSSL_LIBS, OPENSSL_LDFLAGS, have_openssl.
    with_openssl = WITH_OPENSSL.value

    v.OPENSSL_INCLUDES = ""
    v.OPENSSL_LIBS = ""
    v.OPENSSL_LDFLAGS = ""
    v.have_openssl = False

    if with_openssl not in (None, "no"):
        # Explicit --with-openssl=DIR
        ssl_dir = str(with_openssl).rstrip("/")
        v.OPENSSL_INCLUDES = f"-I{ssl_dir}/include"
        v.OPENSSL_LIBS = "-lssl -lcrypto"
        v.OPENSSL_LDFLAGS = f"-L{ssl_dir}/lib"
        v.have_openssl = True
        pyconf.checking("for OpenSSL directory")
        pyconf.result(ssl_dir)
    else:
        # Try pkg-config
        pkg_config = v.PKG_CONFIG
        if pkg_config:
            s1, pc_cflags = pyconf.cmd_status(
                [pkg_config, "--cflags", "openssl"]
            )
            s2, pc_libs = pyconf.cmd_status([pkg_config, "--libs", "openssl"])
            s3, pc_ldflags = pyconf.cmd_status(
                [pkg_config, "--libs-only-L", "openssl"]
            )
            if s1 == 0 and s2 == 0 and s3 == 0:
                # Split includes (-I flags) from other cflags
                inc_parts = []
                for f in pc_cflags.split():
                    if f.startswith("-I"):
                        inc_parts.append(f)
                inc_flags = " ".join(inc_parts)
                # Extract only -l flags from --libs output (not -L, which is in LDFLAGS)
                lib_parts = []
                for f in pc_libs.split():
                    if not f.startswith("-L"):
                        lib_parts.append(f)
                lib_flags = " ".join(lib_parts)
                v.OPENSSL_INCLUDES = inc_flags
                v.OPENSSL_LIBS = lib_flags
                v.OPENSSL_LDFLAGS = pc_ldflags
                v.have_openssl = True
                pyconf.checking(
                    "whether compiling and linking against OpenSSL works"
                )
                pyconf.result("yes")
            else:
                pyconf.checking(
                    "whether compiling and linking against OpenSSL works"
                )
                pyconf.result("no")
        else:
            # Try default system paths
            ssl_h_paths = [
                "/usr/include",
                "/usr/local/include",
                "/usr/include/openssl",
                "/usr/local/include/openssl",
            ]
            found_ssl = False
            for p in ssl_h_paths:
                if pyconf.path_exists(pyconf.path_join([p, "openssl/ssl.h"])):
                    found_ssl = True
                    break
            pyconf.checking(
                "whether compiling and linking against OpenSSL works"
            )
            if found_ssl:
                v.OPENSSL_INCLUDES = ""
                v.OPENSSL_LIBS = "-lssl -lcrypto"
                v.OPENSSL_LDFLAGS = ""
                v.have_openssl = True
                pyconf.result("yes")
            else:
                pyconf.result("no")

    # rpath for OpenSSL
    if v.GNULD:
        rpath_arg = "-Wl,--enable-new-dtags,-rpath="
    elif v.ac_sys_system == "Darwin":
        rpath_arg = "-Wl,-rpath,"
    else:
        rpath_arg = "-Wl,-rpath="

    pyconf.checking("for --with-openssl-rpath")
    openssl_rpath_opt = WITH_OPENSSL_RPATH.value_or("no")
    v.OPENSSL_LDFLAGS_RPATH = ""
    if openssl_rpath_opt in ("auto", "yes"):
        v.OPENSSL_RPATH = "auto"
        for arg in v.OPENSSL_LDFLAGS.split():
            if arg.startswith("-L"):
                v.OPENSSL_LDFLAGS_RPATH += (
                    f" {rpath_arg}{arg.removeprefix('-L')}"
                )
        v.OPENSSL_LDFLAGS_RPATH = v.OPENSSL_LDFLAGS_RPATH.strip()
    elif openssl_rpath_opt == "no":
        v.OPENSSL_RPATH = ""
    else:
        if pyconf.path_is_dir(openssl_rpath_opt):
            v.OPENSSL_RPATH = openssl_rpath_opt
            v.OPENSSL_LDFLAGS_RPATH = f"{rpath_arg}{openssl_rpath_opt}"
        else:
            pyconf.error(
                f'--with-openssl-rpath "{openssl_rpath_opt}" is not a directory'
            )
    pyconf.result(v.OPENSSL_RPATH)

    v.export("OPENSSL_RPATH")
    v.export("OPENSSL_LDFLAGS_RPATH")

    # Static OpenSSL build (unsupported, not advertised)
    if v.PY_UNSUPPORTED_OPENSSL_BUILD == "static":
        pyconf.checking("for unsupported static openssl build")
        new_ssl_libs = []
        for arg in v.OPENSSL_LIBS.split():
            if arg.startswith("-l"):
                libname = arg.removeprefix("-l")
                new_ssl_libs.append(
                    f"-l:lib{libname}.a -Wl,--exclude-libs,lib{libname}.a"
                )
            else:
                new_ssl_libs.append(arg)
        v.OPENSSL_LIBS = " ".join(new_ssl_libs) + f" {v.ZLIB_LIBS}".rstrip()
        pyconf.result(v.OPENSSL_LIBS)

    # libcrypto-only libs (exclude -lssl/-Wl*ssl*)
    LIBCRYPTO_LIBS = []
    for arg in v.OPENSSL_LIBS.split():
        if "ssl" not in arg:
            LIBCRYPTO_LIBS.append(arg)
    v.LIBCRYPTO_LIBS = " ".join(LIBCRYPTO_LIBS)
    # include libz for OpenSSL build flavors with compression support
    v.export("OPENSSL_INCLUDES")
    v.export("OPENSSL_LIBS")
    v.export("OPENSSL_LDFLAGS")
    # AX_CHECK_OPENSSL does not export libcrypto-only libs
    v.export("LIBCRYPTO_LIBS")

    # Check OpenSSL ssl module APIs
    with pyconf.save_env():
        v.LIBS = f"{v.LIBS} {v.OPENSSL_LIBS}".strip()
        v.CFLAGS = f"{v.CFLAGS} {v.OPENSSL_INCLUDES}".strip()
        v.LDFLAGS = f"{v.LDFLAGS} {v.OPENSSL_LDFLAGS} {v.OPENSSL_LDFLAGS_RPATH}".strip()
        working_ssl = pyconf.link_check(
            "whether OpenSSL provides required ssl module APIs",
            """
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#if OPENSSL_VERSION_NUMBER < 0x10101000L
  #error "OpenSSL >= 1.1.1 is required"
#endif
static void keylog_cb(const SSL *ssl, const char *line) {}
int main(void) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_keylog_callback(ctx, keylog_cb);
    SSL *ssl = SSL_new(ctx);
    X509_VERIFY_PARAM *param = SSL_get0_param(ssl);
    X509_VERIFY_PARAM_set1_host(param, "python.org", 0);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    return 0;
}
""",
        )
    v.ac_cv_working_openssl_ssl = working_ssl

    # Check OpenSSL hashlib module APIs
    with pyconf.save_env():
        v.LIBS = f"{v.LIBS} {v.LIBCRYPTO_LIBS}".strip()
        v.CFLAGS = f"{v.CFLAGS} {v.OPENSSL_INCLUDES}".strip()
        v.LDFLAGS = f"{v.LDFLAGS} {v.OPENSSL_LDFLAGS} {v.OPENSSL_LDFLAGS_RPATH}".strip()
        working_hashlib = pyconf.link_check(
            "whether OpenSSL provides required hashlib module APIs",
            """
#include <openssl/opensslv.h>
#include <openssl/evp.h>
#if OPENSSL_VERSION_NUMBER < 0x10101000L
  #error "OpenSSL >= 1.1.1 is required"
#endif
int main(void) {
    OBJ_nid2sn(NID_md5);
    OBJ_nid2sn(NID_sha1);
    OBJ_nid2sn(NID_sha512);
    OBJ_nid2sn(NID_sha3_512);
    EVP_PBE_scrypt(NULL, 0, NULL, 0, 2, 8, 1, 0, NULL, 0);
    return 0;
}
""",
        )
    v.ac_cv_working_openssl_hashlib = working_hashlib


def check_ssl_cipher_suites(v):
    # ---------------------------------------------------------------------------
    # SSL default cipher suites
    # ---------------------------------------------------------------------------

    pyconf.define_template(
        "PY_SSL_DEFAULT_CIPHERS",
        "Default cipher suites list for ssl module. "
        "1: Python's preferred selection, 2: leave OpenSSL defaults untouched, "
        "0: custom string",
    )
    pyconf.define_template(
        "PY_SSL_DEFAULT_CIPHER_STRING",
        "Cipher suite string for PY_SSL_DEFAULT_CIPHERS=0",
    )

    pyconf.checking("for --with-ssl-default-suites")
    ssl_suites = WITH_SSL_DEFAULT_SUITES.value_or("python")
    pyconf.result(ssl_suites)
    if ssl_suites == "python":
        pyconf.define("PY_SSL_DEFAULT_CIPHERS", 1)
    elif ssl_suites == "openssl":
        pyconf.define("PY_SSL_DEFAULT_CIPHERS", 2)
    else:
        pyconf.define("PY_SSL_DEFAULT_CIPHERS", 0)
        pyconf.define_unquoted(
            "PY_SSL_DEFAULT_CIPHER_STRING", f'"{ssl_suites}"'
        )


def check_builtin_hashlib_hashes(v):
    # ---------------------------------------------------------------------------
    # --with-builtin-hashlib-hashes
    # ---------------------------------------------------------------------------

    default_hashlib_hashes = "md5,sha1,sha2,sha3,blake2"

    pyconf.checking("for --with-builtin-hashlib-hashes")
    bh_raw = WITH_BUILTIN_HASHLIB_HASHES.value
    if bh_raw is None or bh_raw == "yes":
        builtin_hashes = default_hashlib_hashes
    elif bh_raw == "no":
        builtin_hashes = ""
    else:
        builtin_hashes = bh_raw
    pyconf.result(builtin_hashes)

    pyconf.define_unquoted(
        "PY_BUILTIN_HASHLIB_HASHES",
        f'"{builtin_hashes}"',
        "enabled builtin hash modules",
    )

    v.with_builtin_md5 = "md5" in builtin_hashes.split(",")
    v.with_builtin_sha1 = "sha1" in builtin_hashes.split(",")
    v.with_builtin_sha2 = "sha2" in builtin_hashes.split(",")
    v.with_builtin_sha3 = "sha3" in builtin_hashes.split(",")
    v.with_builtin_blake2 = "blake2" in builtin_hashes.split(",")


WITH_HASH_ALGORITHM = pyconf.arg_with(
    "hash-algorithm",
    metavar="fnv|siphash13|siphash24",
    help="select hash algorithm for use in Python/pyhash.c (default is SipHash13)",
)


def setup_hash_algorithm():
    # ---------------------------------------------------------------------------
    # str, bytes and memoryview hash algorithm
    # ---------------------------------------------------------------------------

    pyconf.define_template(
        "Py_HASH_ALGORITHM",
        "Define hash algorithm for str, bytes and memoryview. "
        "SipHash24: 1, FNV: 2, SipHash13: 3, externally defined: 0",
    )

    pyconf.checking("for --with-hash-algorithm")
    hash_val = WITH_HASH_ALGORITHM.value
    if hash_val is not None:
        if hash_val == "siphash13":
            pyconf.define("Py_HASH_ALGORITHM", 3)
            pyconf.result("siphash13")
        elif hash_val == "siphash24":
            pyconf.define("Py_HASH_ALGORITHM", 1)
            pyconf.result("siphash24")
        elif hash_val == "fnv":
            pyconf.define("Py_HASH_ALGORITHM", 2)
            pyconf.result("fnv")
        else:
            pyconf.error(f"unknown hash algorithm '{hash_val}'")
    else:
        pyconf.result("default")


def _hacl_module(v, component, extname, enabled):
    """Create a HACL*-based stdlib module entry."""
    ldflags = f"$(LIBHACL_{component}_LIB_{v.LIBHACL_LDEPS_LIBTYPE})"
    pyconf.stdlib_module(
        extname, enabled=enabled, cflags=v.LIBHACL_CFLAGS, ldflags=ldflags
    )


def setup_hacl(v):
    # ---------------------------------------------------------------------------
    # HACL* compilation flags
    # ---------------------------------------------------------------------------

    # LIBHACL_FLAG_I: '-I' flags passed to $(CC) for HACL* and HACL*-based modules
    # LIBHACL_FLAG_D: '-D' flags passed to $(CC) for HACL* and HACL*-based modules
    # LIBHACL_CFLAGS: compiler flags passed for HACL* and HACL*-based modules
    # LIBHACL_LDFLAGS: linker flags passed for HACL* and HACL*-based modules
    LIBHACL_FLAG_I = (
        "-I$(srcdir)/Modules/_hacl -I$(srcdir)/Modules/_hacl/include"
    )
    LIBHACL_FLAG_D = "-D_BSD_SOURCE -D_DEFAULT_SOURCE"
    if v.ac_sys_system.startswith("Linux"):
        if v.ac_cv_func_explicit_bzero is False:
            LIBHACL_FLAG_D += " -DLINUX_NO_EXPLICIT_BZERO"

    v.export(
        "LIBHACL_CFLAGS",
        f"{LIBHACL_FLAG_I} {LIBHACL_FLAG_D} $(PY_STDMODULE_CFLAGS) $(CCSHARED)",
    )

    v.export("LIBHACL_LDFLAGS", "")

    # universal2 / Apple aarch64 detection
    build_cpu = v.build_cpu
    build_vendor = v.build_vendor
    use_hacl_universal2 = v.UNIVERSAL_ARCHS == "universal2" or (
        build_cpu == "aarch64" and build_vendor == "apple"
    )

    # SIMD128 (SSE)
    # The SIMD files use aligned_alloc, which is not available on older versions of
    # Android. The *mmintrin.h headers are x86-family-specific, so can't be used on WASI.
    if v.ac_sys_system not in ("Linux-android", "WASI") or (
        v.ANDROID_API_LEVEL and int(v.ANDROID_API_LEVEL) >= 28
    ):
        if pyconf.check_compile_flag(
            "-msse -msse2 -msse3 -msse4.1 -msse4.2", extra_cflags="-Werror"
        ):
            v.LIBHACL_SIMD128_FLAGS = "-msse -msse2 -msse3 -msse4.1 -msse4.2"
            pyconf.define(
                "_Py_HACL_CAN_COMPILE_VEC128",
                1,
                "HACL* library can compile SIMD128 implementations",
            )
            # macOS universal2 builds *support* the -msse etc flags because they're
            # available on x86_64. However, performance of the HACL SIMD128 implementation
            # isn't great, so it's disabled on ARM64.
            if use_hacl_universal2:
                v.LIBHACL_BLAKE2_SIMD128_OBJS = (
                    "Modules/_hacl/Hacl_Hash_Blake2s_Simd128_universal2.o"
                )
            else:
                v.LIBHACL_BLAKE2_SIMD128_OBJS = (
                    "Modules/_hacl/Hacl_Hash_Blake2s_Simd128.o"
                )
        else:
            v.LIBHACL_SIMD128_FLAGS = ""
            v.LIBHACL_BLAKE2_SIMD128_OBJS = ""
    else:
        v.LIBHACL_SIMD128_FLAGS = ""
        v.LIBHACL_BLAKE2_SIMD128_OBJS = ""
    v.export("LIBHACL_SIMD128_FLAGS")
    v.export("LIBHACL_BLAKE2_SIMD128_OBJS")

    # SIMD256 (AVX2)
    # The SIMD files use aligned_alloc, which is not available on older versions of
    # Android. The *mmintrin.h headers are x86-family-specific, so can't be used on WASI.
    # Although AVX support is not guaranteed on Android
    # (https://developer.android.com/ndk/guides/abis#86-64), this is safe because we do a
    # runtime CPUID check.
    if v.ac_sys_system not in ("Linux-android", "WASI") or (
        v.ANDROID_API_LEVEL and int(v.ANDROID_API_LEVEL) >= 28
    ):
        if pyconf.check_compile_flag("-mavx2", extra_cflags="-Werror"):
            v.LIBHACL_SIMD256_FLAGS = "-mavx2"
            pyconf.define(
                "_Py_HACL_CAN_COMPILE_VEC256",
                1,
                "HACL* library can compile SIMD256 implementations",
            )
            # macOS universal2 builds *support* the -mavx2 compiler flag because it's
            # available on x86_64; but the HACL SIMD256 build then fails because the
            # implementation requires symbols that aren't available on ARM64. Use a
            # wrapped implementation if we're building for universal2.
            if use_hacl_universal2:
                v.LIBHACL_BLAKE2_SIMD256_OBJS = (
                    "Modules/_hacl/Hacl_Hash_Blake2b_Simd256_universal2.o"
                )
            else:
                v.LIBHACL_BLAKE2_SIMD256_OBJS = (
                    "Modules/_hacl/Hacl_Hash_Blake2b_Simd256.o"
                )
        else:
            v.LIBHACL_SIMD256_FLAGS = ""
            v.LIBHACL_BLAKE2_SIMD256_OBJS = ""
    else:
        v.LIBHACL_SIMD256_FLAGS = ""
        v.LIBHACL_BLAKE2_SIMD256_OBJS = ""
    v.export("LIBHACL_SIMD256_FLAGS")
    v.export("LIBHACL_BLAKE2_SIMD256_OBJS")

    # HACL* library linking type (WASI -> static, else shared)
    # Used to complete the "MODULE_<NAME>_LDEPS" Makefile variable.
    # The LDEPS variable is a Makefile rule prerequisite.
    v.export(
        "LIBHACL_LDEPS_LIBTYPE",
        "STATIC" if v.ac_sys_system == "WASI" else "SHARED",
    )

    # ---------------------------------------------------------------------------
    # HACL*-based cryptographic primitives  (PY_HACL_CREATE_MODULE)
    # ---------------------------------------------------------------------------

    # By default we always compile these even when OpenSSL is available
    # (see bpo-14693). The modules are small.
    _hacl_module(v, "MD5", "_md5", v.with_builtin_md5)
    _hacl_module(v, "SHA1", "_sha1", v.with_builtin_sha1)
    _hacl_module(v, "SHA2", "_sha2", v.with_builtin_sha2)
    _hacl_module(v, "SHA3", "_sha3", v.with_builtin_sha3)
    _hacl_module(v, "BLAKE2", "_blake2", v.with_builtin_blake2)
    # HMAC builtin library does not need OpenSSL for now. In the future
    # we might want to rely on OpenSSL EVP/NID interface or implement
    # our own for algorithm resolution.
    # For Emscripten, we disable HACL* HMAC as it is tricky to make it work.
    # See https://github.com/python/cpython/issues/133042.
    _hacl_module(v, "HMAC", "_hmac", v.ac_sys_system != "Emscripten")
