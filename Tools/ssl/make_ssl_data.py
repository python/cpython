#!/usr/bin/env python3

"""
This script should be called *manually* when we want to upgrade SSLError
`library` and `reason` mnemonics to a more recent OpenSSL version. Note
that error codes are version specific.

It takes two arguments:

- the path to the OpenSSL folder with the correct git checkout (see below)
- the path to the header file to be generated, usually

    Modules/_ssl_data_<MAJOR><MINOR><PATCH>.h

The OpenSSL git checkout should be at a specific tag, using commands like:

    git tag --list 'openssl-*'
    git switch --detach openssl-3.4.1

After generating the definitions, compare the result with newest pre-existing file.
You can use a command like:

    git diff --no-index Modules/_ssl_data_340.h Modules/_ssl_data_341.h

- If the new version *only* adds new definitions, remove the pre-existing file
  and adjust the #include in _ssl.c to point to the new version.
- If the new version removes or renumbers some definitions, keep both files and
  add a new #include in _ssl.c.

By convention, the latest OpenSSL mnemonics are gathered in the following file:

    Modules/_ssl_data_<MAJOR><MINOR>.h

If those mnemonics are renumbered or removed in a subsequent OpenSSL version,
the file is renamed to "Modules/_ssl_data_<MAJOR><MINOR><PATCH>.h" and the
latest mnemonics are stored in the patchless file (see below for an example).

A newly supported OpenSSL version should also be added to:

- Tools/ssl/multissltests.py
- .github/workflows/build.yml

Example: new mnemonics are added
--------------------------------
Assume that "Modules/_ssl_data_32x.h" contains the latest mnemonics for
CPython and was generated from OpenSSL 3.2.1. If only new mnemonics are
added in OpenSSL 3.2.2, the following commands should be executed:

    # in the OpenSSL git directory
    git switch --detach openssl-3.2.2

    # in the CPython git directory
    python make_ssl_data.py PATH_TO_OPENSSL_GIT_CLONE Modules/_ssl_data_322.h
    mv Modules/_ssl_data_322.h Modules/_ssl_data_32.h

Example: mnemonics are renamed/removed
--------------------------------------
Assume that the existing file is Modules/_ssl_data_34x.h and is based
on OpenSSL 3.4.0. Since some mnemonics were renamed in OpenSSL 3.4.1,
the following commands should be executed:

    # in the OpenSSL git directory
    git switch --detach openssl-3.4.1

    # in the CPython git directory
    mv Modules/_ssl_data_34.h Modules/_ssl_data_340.h
    python make_ssl_data.py PATH_TO_OPENSSL_GIT_CLONE Modules/_ssl_data_341.h
    mv Modules/_ssl_data_341.h Modules/_ssl_data_34.h
"""

import argparse
import datetime
import logging
import operator
import os
import re
import subprocess


logger = logging.getLogger(__name__)
parser = argparse.ArgumentParser(
    formatter_class=argparse.RawTextHelpFormatter,
    description="Generate SSL data headers from OpenSSL sources"
)
parser.add_argument("srcdir", help="OpenSSL source directory")
parser.add_argument(
    "output", nargs="?", default=None,
    help="output file (default: standard output)",
)


def error(format_string, *format_args, **kwargs):
    # do not use parser.error() to avoid printing short help
    logger.error(format_string, *format_args, **kwargs)
    raise SystemExit(1)


def _file_search(fname, pat):
    with open(fname, encoding="utf-8") as f:
        for line in f:
            match = pat.search(line)
            if match is not None:
                yield match


def parse_err_h(args):
    """Parse error codes from include/openssl/err.h.in.

    Detected lines match (up to spaces) "#define ERR_LIB_<LIBNAME> <ERRCODE>",
    e.g., "# define ERR_LIB_NONE 1".
    """
    pat = re.compile(r"#\s*define\W+(ERR_LIB_(\w+))\s+(\d+)")
    lib2errnum = {}
    for match in _file_search(args.err_h, pat):
        macroname, libname, num = match.groups()
        if macroname in ['ERR_LIB_OFFSET', 'ERR_LIB_MASK']:
            # ignore: "# define ERR_LIB_OFFSET                 23L"
            # ignore: "# define ERR_LIB_MASK                   0xFF"
            continue
        actual = int(num)
        expect = lib2errnum.setdefault(libname, actual)
        if actual != expect:
            logger.warning("OpenSSL inconsistency for ERR_LIB_%s (%d != %d)",
                           libname, actual, expect)
    return lib2errnum


def parse_openssl_error_text(args):
    """Parse error reasons from crypto/err/openssl.txt.

    Detected lines match "<LIBNAME>_R_<ERRNAME>:<ERRCODE>:<MESSAGE>",
    e.g., "ASN1_R_ADDING_OBJECT:171:adding object". The <MESSAGE> part
    is not stored as it will be recovered at runtime when needed.
    """
    # ignore backslash line continuation (placed before <MESSAGE> if present)
    pat = re.compile(r"^((\w+?)_R_(\w+)):(\d+):")
    seen = {}
    for match in _file_search(args.errtxt, pat):
        reason, libname, errname, num = match.groups()
        if "_F_" in reason:  # ignore function codes
            # FEAT(picnixz): in the future, we may want to also check
            # the consistency of the OpenSSL files with an external tool.
            # See https://github.com/python/cpython/issues/132745.
            continue
        yield reason, libname, errname, int(num)


def parse_extra_reasons(args):
    """Parse extra reasons from crypto/err/openssl.ec.

    Detected lines are matched against "R <LIBNAME>_R_<ERRNAME> <ERRCODE>",
    e.g., "R SSL_R_SSLV3_ALERT_UNEXPECTED_MESSAGE 1010".
    """
    pat = re.compile(r"^R\s+((\w+)_R_(\w+))\s+(\d+)")
    for match in _file_search(args.errcodes, pat):
        reason, libname, errname, num = match.groups()
        yield reason, libname, errname, int(num)


def gen_library_codes(args):
    """Generate table short libname to numeric code."""
    yield "/* generated from args.lib2errnum */"
    yield "static struct py_ssl_library_code library_codes[] = {"
    for libname in sorted(args.lib2errnum):
        yield f"#ifdef ERR_LIB_{libname}"
        yield f'    {{"{libname}", ERR_LIB_{libname}}},'
        yield "#endif"
    yield "    {NULL, 0}  /* sentinel */"
    yield "};"


def gen_error_codes(args):
    """Generate error code table for error reasons."""
    yield "/* generated from args.reasons */"
    yield "static struct py_ssl_error_code error_codes[] = {"
    for reason, libname, errname, num in args.reasons:
        yield f"  #ifdef {reason}"
        yield f'    {{"{errname}", ERR_LIB_{libname}, {reason}}},'
        yield "  #else"
        yield f'    {{"{errname}", {args.lib2errnum[libname]}, {num}}},'
        yield "  #endif"
    yield "    {NULL, 0, 0}  /* sentinel */"
    yield "};"


def get_openssl_git_commit(args):
    git_describe = subprocess.run(
        ['git', 'describe', '--long', '--dirty'],
        cwd=args.srcdir,
        capture_output=True,
        encoding='utf-8',
        check=True,
    )
    return git_describe.stdout.strip()


def main(args=None):
    args = parser.parse_args(args)
    if not os.path.isdir(args.srcdir):
        error(f"OpenSSL directory not found: {args.srcdir}")
    args.err_h = os.path.join(args.srcdir, "include", "openssl", "err.h")
    if not os.path.isfile(args.err_h):
        # Fall back to infile for OpenSSL 3.0.0 and later.
        args.err_h += ".in"
    args.errcodes = os.path.join(args.srcdir, "crypto", "err", "openssl.ec")
    if not os.path.isfile(args.errcodes):
        error(f"file {args.errcodes} not found in {args.srcdir}")
    args.errtxt = os.path.join(args.srcdir, "crypto", "err", "openssl.txt")
    if not os.path.isfile(args.errtxt):
        error(f"file {args.errtxt} not found in {args.srcdir}")

    # [("ERR_LIB_X509", "X509", 11), ...]
    args.lib2errnum = parse_err_h(args)

    # [('X509_R_AKID_MISMATCH', 'X509', 'AKID_MISMATCH', 110), ...]
    reasons = []
    reasons.extend(parse_openssl_error_text(args))
    reasons.extend(parse_extra_reasons(args))
    # sort by macro name and numeric error code
    args.reasons = sorted(reasons, key=operator.itemgetter(0, 3))

    commit = get_openssl_git_commit(args)
    lines = [
        "/* File generated by Tools/ssl/make_ssl_data.py */",
        f"/* Generated on {datetime.datetime.now(datetime.UTC).isoformat()} */",
        f"/* Generated from Git commit {commit} */",
        "",
    ]
    lines.extend(gen_library_codes(args))
    lines.append("")
    lines.extend(gen_error_codes(args))

    if args.output is None:
        for line in lines:
            print(line)
    else:
        with open(args.output, 'w') as output:
            for line in lines:
                print(line, file=output)


if __name__ == "__main__":
    main()
