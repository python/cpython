/*
 * Copyright (c) 1993 George V. Neville-Neil
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name, George Neville-Neil may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _TIMING_H_
#define _TIMING_H_

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else /* !TIME_WITH_SYS_TIME */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else /* !HAVE_SYS_TIME_H */
#include <time.h>
#endif /* !HAVE_SYS_TIME_H */
#endif /* !TIME_WITH_SYS_TIME */

static struct timeval aftertp, beforetp;

#define BEGINTIMING gettimeofday(&beforetp, NULL)

#define ENDTIMING gettimeofday(&aftertp, NULL); \
    if(beforetp.tv_usec > aftertp.tv_usec) \
    {  \
         aftertp.tv_usec += 1000000;  \
         aftertp.tv_sec--; \
    }

#define TIMINGUS (((aftertp.tv_sec - beforetp.tv_sec) * 1000000) + \
		  (aftertp.tv_usec - beforetp.tv_usec))

#define TIMINGMS (((aftertp.tv_sec - beforetp.tv_sec) * 1000) + \
		  ((aftertp.tv_usec - beforetp.tv_usec) / 1000))

#define TIMINGS  ((aftertp.tv_sec - beforetp.tv_sec) + \
		  (aftertp.tv_usec - beforetp.tv_usec) / 1000000)

#endif /* _TIMING_H_ */
