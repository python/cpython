#ifndef BEOS_DL_EXPORT_H
#define BEOS_DL_EXPORT_H

/* There are no declarations here, so no #ifdef __cplusplus...
 *
 * This is the nasty declaration decorations required by certain systems
 * (in our case, BeOS) for dynamic object loading.
 *
 * This trivial header is released under the same license as the rest of
 * Python:
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the names of Stichting Mathematisch
 * Centrum or CWI or Corporation for National Research Initiatives or
 * CNRI not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 *
 * While CWI is the initial source for this software, a modified version
 * is made available by the Corporation for National Research Initiatives
 * (CNRI) at the Internet address ftp://ftp.python.org.
 *
 * STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
 * CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * - Chris Herborth (chrish@beoscentral.com)
 *   January 11, 1999
 */

#ifndef DL_EXPORT
#  define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#endif
#ifndef DL_IMPORT
#  ifdef USE_DL_EXPORT
#    define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#  else
#    define DL_IMPORT(RTYPE) __declspec(dllimport) RTYPE
#  endif
#endif

#endif
