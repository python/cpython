

libmpdec
========

libmpdec is a fast C/C++ library for correctly-rounded arbitrary precision
decimal floating point arithmetic. It is a complete implementation of
Mike Cowlishaw/IBM's General Decimal Arithmetic Specification.


Files required for the Python _decimal module
=============================================

  Core files for small and medium precision arithmetic
  ----------------------------------------------------

    basearith.{c,h}  ->  Core arithmetic in base 10**9 or 10**19.
    bits.h           ->  Portable detection of least/most significant one-bit.
    constants.{c,h}  ->  Constants that are used in multiple files.
    context.c        ->  Context functions.
    io.{c,h}         ->  Conversions between mpd_t and ASCII strings,
                         mpd_t formatting (allows UTF-8 fill character).
    mpalloc.{c,h}    ->  Allocation handlers with overflow detection
                         and functions for switching between static
                         and dynamic mpd_t.
    mpdecimal.{c,h}  ->  All (quiet) functions of the specification.
    typearith.h      ->  Fast primitives for double word multiplication,
                         division etc.

    Visual Studio only:
    ~~~~~~~~~~~~~~~~~~~
      vcdiv64.asm   ->  Double word division used in typearith.h. VS 2008 does
                        not allow inline asm for x64. Also, it does not provide
                        an intrinsic for double word division.

  Files for bignum arithmetic:
  ----------------------------

    The following files implement the Fast Number Theoretic Transform
    used for multiplying coefficients with more than 1024 words (see
    mpdecimal.c: _mpd_fntmul()).

      umodarith.h        ->  Fast low level routines for unsigned modular arithmetic.
      numbertheory.{c,h} ->  Routines for setting up the Number Theoretic Transform.
      difradix2.{c,h}    ->  Decimation in frequency transform, used as the
                             "base case" by the following three files:

        fnt.{c,h}        ->  Transform arrays up to 4096 words.
        sixstep.{c,h}    ->  Transform larger arrays of length 2**n.
        fourstep.{c,h}   ->  Transform larger arrays of length 3 * 2**n.

      convolute.{c,h}    ->  Fast convolution using one of the three transform
                             functions.
      transpose.{c,h}    ->  Transpositions needed for the sixstep algorithm.
      crt.{c,h}          ->  Chinese Remainder Theorem: use information from three
                             transforms modulo three different primes to get the
                             final result.


Pointers to literature, proofs and more
=======================================

  literature/
  -----------

    REFERENCES.txt  ->  List of relevant papers.
    bignum.txt      ->  Explanation of the Fast Number Theoretic Transform (FNT).
    fnt.py          ->  Verify constants used in the FNT; Python demo for the
                        O(N**2) discrete transform.

    matrix-transform.txt -> Proof for the Matrix Fourier Transform used in
                            fourstep.c.
    six-step.txt         -> Show that the algorithm used in sixstep.c is
                            a variant of the Matrix Fourier Transform.
    mulmod-64.txt        -> Proof for the mulmod64 algorithm from
                            umodarith.h.
    mulmod-ppro.txt      -> Proof for the x87 FPU modular multiplication
                            from umodarith.h.
    umodarith.lisp       -> ACL2 proofs for many functions from umodarith.h.


Library Author
==============

  Stefan Krah <skrah@bytereef.org>



