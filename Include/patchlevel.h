/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Newfangled version identification scheme.

   This scheme was added in Python 1.5.2b2; before that time, only PATCHLEVEL
   was available.  To test for presence of the scheme, test for
   defined(PY_MAJOR_VERSION).

   When the major or minor version changes, the VERSION variable in
   configure.in must also be changed.

   There is also (independent) API version information in modsupport.h.
*/

/* Values for PY_RELEASE_LEVEL */
#define PY_RELEASE_LEVEL_ALPHA	0xA
#define PY_RELEASE_LEVEL_BETA	0xB
#define PY_RELEASE_LEVEL_FINAL	0xF	/* Serial should be 0 here */

/* Version parsed out into numeric values */
#define PY_MAJOR_VERSION	1
#define PY_MINOR_VERSION	5
#define PY_MICRO_VERSION	2
#define PY_RELEASE_LEVEL	PY_RELEASE_LEVEL_BETA
#define PY_RELEASE_SERIAL	2

/* Version as a string */
#define PY_VERSION		"1.5.2b2"

/* Historic */
#define PATCHLEVEL		"1.5.2b2"

/* Version as a single 4-byte hex number, e.g. 0x010502B2 == 1.5.2b2.
   Use this for numeric comparisons, e.g. #if PY_VERSION_HEX >= ... */
#define PY_VERSION_HEX ((PY_MAJOR_VERSION << 24) | \
			(PY_MINOR_VERSION << 16) | \
			(PY_MICRO_VERSION <<  8) | \
			(PY_RELEASE_LEVEL <<  4) | \
			(PY_RELEASE_SERIAL << 0))
