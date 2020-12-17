/*
 * regcomp and regexec - front ends to re_ routines
 *
 * Mostly for implementation of backward-compatibility kludges. Note that
 * these routines exist ONLY in char versions.
 *
 * Copyright (c) 1998, 1999 Henry Spencer.  All rights reserved.
 *
 * Development of this software was funded, in part, by Cray Research Inc.,
 * UUNET Communications Services Inc., Sun Microsystems Inc., and Scriptics
 * Corporation, none of whom are responsible for the results.  The author
 * thanks all of them.
 *
 * Redistribution and use in source and binary forms -- with or without
 * modification -- are permitted for any purpose, provided that
 * redistributions in source form retain this entire copyright notice and
 * indicate the origin and nature of any modifications.
 *
 * I'd appreciate being given credit for this package in the documentation of
 * software which uses it, but that is not a requirement.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * HENRY SPENCER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "regguts.h"

/*
 - regcomp - compile regular expression
 */
int
regcomp(
    regex_t *re,
    const char *str,
    int flags)
{
    size_t len;
    int f = flags;

    if (f&REG_PEND) {
	len = re->re_endp - str;
	f &= ~REG_PEND;
    } else {
	len = strlen(str);
    }

    return re_comp(re, str, len, f);
}

/*
 - regexec - execute regular expression
 */
int
regexec(
    regex_t *re,
    const char *str,
    size_t nmatch,
    regmatch_t pmatch[],
    int flags)
{
    const char *start;
    size_t len;
    int f = flags;

    if (f & REG_STARTEND) {
	start = str + pmatch[0].rm_so;
	len = pmatch[0].rm_eo - pmatch[0].rm_so;
	f &= ~REG_STARTEND;
    } else {
	start = str;
	len = strlen(str);
    }

    return re_exec(re, start, len, nmatch, pmatch, f);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
