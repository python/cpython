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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by George V. Neville-Neil
 * 4. The name, George Neville-Neil may not be used to endorse or promote 
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

/*
 *
 * File: $Id$
 *
 * Author: George V. Neville-Neil
 *
 * Update History: $Log$
 * Update History: Revision 2.1  1994/01/02 23:22:19  guido
 * Update History: Added George Neville-Neil's timing module
 * Update History:
 * Revision 1.1  93/12/28  13:14:19  gnn
 * Initial revision
 * 
 * Revision 1.1  93/12/23  18:59:10  gnn
 * Initial revision
 * 
 * Revision 1.3  93/07/22  15:57:39  15:57:39  gnn (George Neville-Neil)
 * Added copyright message.
 * 
 * Revision 1.2  1993/07/22  12:12:34  gnn
 * Latest macros.  They now evaluate to rvalues.
 *
 * Revision 1.1  93/07/19  16:56:18  16:56:18  gnn (George Neville-Neil)
 * Initial revision
 * 
 *
 *
 */

#ifndef _TIMING_H_
#define _TIMING_H_

#include <sys/time.h>

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
