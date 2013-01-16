

About
=====

_decimal.c is a wrapper for the libmpdec library. libmpdec is a fast C
library for correctly-rounded arbitrary precision decimal floating point
arithmetic. It is a complete implementation of Mike Cowlishaw/IBM's
General Decimal Arithmetic Specification.


Build process for the module
============================

As usual, the build process for _decimal.so is driven by setup.py in the top
level directory. setup.py autodetects the following build configurations:

   1) x64         - 64-bit Python, x86_64 processor (AMD, Intel)

   2) uint128     - 64-bit Python, compiler provides __uint128_t (gcc)

   3) ansi64      - 64-bit Python, ANSI C

   4) ppro        - 32-bit Python, x86 CPU, PentiumPro or later

   5) ansi32      - 32-bit Python, ANSI C

   6) ansi-legacy - 32-bit Python, compiler without uint64_t

   7) universal   - Mac OS only (multi-arch)


It is possible to override autodetection by exporting:

   PYTHON_DECIMAL_WITH_MACHINE=value, where value is one of the above options.


NOTE
====

decimal.so is not built from a static libmpdec.a since doing so led to
failures on AIX (user report) and Windows (mixing static and dynamic CRTs
causes locale problems and more).



