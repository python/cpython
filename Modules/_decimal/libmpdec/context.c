/*
 * Copyright (c) 2008-2020 Stefan Krah. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include "mpdecimal.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>


void
mpd_dflt_traphandler(mpd_context_t *ctx)
{
    (void)ctx;
    raise(SIGFPE);
}

void (* mpd_traphandler)(mpd_context_t *) = mpd_dflt_traphandler;


/* Set guaranteed minimum number of coefficient words. The function may
   be used once at program start. Setting MPD_MINALLOC to out-of-bounds
   values is a catastrophic error, so in that case the function exits rather
   than relying on the user to check a return value. */
void
mpd_setminalloc(mpd_ssize_t n)
{
    static int minalloc_is_set = 0;

    if (minalloc_is_set) {
        mpd_err_warn("mpd_setminalloc: ignoring request to set "
                     "MPD_MINALLOC a second time\n");
        return;
    }
    if (n < MPD_MINALLOC_MIN || n > MPD_MINALLOC_MAX) {
        mpd_err_fatal("illegal value for MPD_MINALLOC"); /* GCOV_NOT_REACHED */
    }
    MPD_MINALLOC = n;
    minalloc_is_set = 1;
}

void
mpd_init(mpd_context_t *ctx, mpd_ssize_t prec)
{
    mpd_ssize_t ideal_minalloc;

    mpd_defaultcontext(ctx);

    if (!mpd_qsetprec(ctx, prec)) {
        mpd_addstatus_raise(ctx, MPD_Invalid_context);
        return;
    }

    ideal_minalloc = 2 * ((prec+MPD_RDIGITS-1) / MPD_RDIGITS);
    if (ideal_minalloc < MPD_MINALLOC_MIN) ideal_minalloc = MPD_MINALLOC_MIN;
    if (ideal_minalloc > MPD_MINALLOC_MAX) ideal_minalloc = MPD_MINALLOC_MAX;

    mpd_setminalloc(ideal_minalloc);
}

void
mpd_maxcontext(mpd_context_t *ctx)
{
    ctx->prec=MPD_MAX_PREC;
    ctx->emax=MPD_MAX_EMAX;
    ctx->emin=MPD_MIN_EMIN;
    ctx->round=MPD_ROUND_HALF_EVEN;
    ctx->traps=MPD_Traps;
    ctx->status=0;
    ctx->newtrap=0;
    ctx->clamp=0;
    ctx->allcr=1;
}

void
mpd_defaultcontext(mpd_context_t *ctx)
{
    ctx->prec=2*MPD_RDIGITS;
    ctx->emax=MPD_MAX_EMAX;
    ctx->emin=MPD_MIN_EMIN;
    ctx->round=MPD_ROUND_HALF_UP;
    ctx->traps=MPD_Traps;
    ctx->status=0;
    ctx->newtrap=0;
    ctx->clamp=0;
    ctx->allcr=1;
}

void
mpd_basiccontext(mpd_context_t *ctx)
{
    ctx->prec=9;
    ctx->emax=MPD_MAX_EMAX;
    ctx->emin=MPD_MIN_EMIN;
    ctx->round=MPD_ROUND_HALF_UP;
    ctx->traps=MPD_Traps|MPD_Clamped;
    ctx->status=0;
    ctx->newtrap=0;
    ctx->clamp=0;
    ctx->allcr=1;
}

int
mpd_ieee_context(mpd_context_t *ctx, int bits)
{
    if (bits <= 0 || bits > MPD_IEEE_CONTEXT_MAX_BITS || bits % 32) {
        return -1;
    }

    ctx->prec = 9 * (bits/32) - 2;
    ctx->emax = 3 * ((mpd_ssize_t)1<<(bits/16+3));
    ctx->emin = 1 - ctx->emax;
    ctx->round=MPD_ROUND_HALF_EVEN;
    ctx->traps=0;
    ctx->status=0;
    ctx->newtrap=0;
    ctx->clamp=1;
    ctx->allcr=1;

    return 0;
}

mpd_ssize_t
mpd_getprec(const mpd_context_t *ctx)
{
    return ctx->prec;
}

mpd_ssize_t
mpd_getemax(const mpd_context_t *ctx)
{
    return ctx->emax;
}

mpd_ssize_t
mpd_getemin(const mpd_context_t *ctx)
{
    return ctx->emin;
}

int
mpd_getround(const mpd_context_t *ctx)
{
    return ctx->round;
}

uint32_t
mpd_gettraps(const mpd_context_t *ctx)
{
    return ctx->traps;
}

uint32_t
mpd_getstatus(const mpd_context_t *ctx)
{
    return ctx->status;
}

int
mpd_getclamp(const mpd_context_t *ctx)
{
    return ctx->clamp;
}

int
mpd_getcr(const mpd_context_t *ctx)
{
    return ctx->allcr;
}


int
mpd_qsetprec(mpd_context_t *ctx, mpd_ssize_t prec)
{
    if (prec <= 0 || prec > MPD_MAX_PREC) {
        return 0;
    }
    ctx->prec = prec;
    return 1;
}

int
mpd_qsetemax(mpd_context_t *ctx, mpd_ssize_t emax)
{
    if (emax < 0 || emax > MPD_MAX_EMAX) {
        return 0;
    }
    ctx->emax = emax;
    return 1;
}

int
mpd_qsetemin(mpd_context_t *ctx, mpd_ssize_t emin)
{
    if (emin > 0 || emin < MPD_MIN_EMIN) {
        return 0;
    }
    ctx->emin = emin;
    return 1;
}

int
mpd_qsetround(mpd_context_t *ctx, int round)
{
    if (!(0 <= round && round < MPD_ROUND_GUARD)) {
        return 0;
    }
    ctx->round = round;
    return 1;
}

int
mpd_qsettraps(mpd_context_t *ctx, uint32_t flags)
{
    if (flags > MPD_Max_status) {
        return 0;
    }
    ctx->traps = flags;
    return 1;
}

int
mpd_qsetstatus(mpd_context_t *ctx, uint32_t flags)
{
    if (flags > MPD_Max_status) {
        return 0;
    }
    ctx->status = flags;
    return 1;
}

int
mpd_qsetclamp(mpd_context_t *ctx, int c)
{
    if (c != 0 && c != 1) {
        return 0;
    }
    ctx->clamp = c;
    return 1;
}

int
mpd_qsetcr(mpd_context_t *ctx, int c)
{
    if (c != 0 && c != 1) {
        return 0;
    }
    ctx->allcr = c;
    return 1;
}


void
mpd_addstatus_raise(mpd_context_t *ctx, uint32_t flags)
{
    ctx->status |= flags;
    if (flags&ctx->traps) {
        ctx->newtrap = (flags&ctx->traps);
        mpd_traphandler(ctx);
    }
}
