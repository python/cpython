/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef HAVE_GETADDRINFO

/*
 * Error return codes from getaddrinfo()
 */
#ifdef EAI_ADDRFAMILY
/* If this is defined, there is a conflicting implementation
   in the C library, which can't be used for some reason.
   Make sure it won't interfere with this emulation. */

#undef EAI_ADDRFAMILY
#undef EAI_AGAIN
#undef EAI_BADFLAGS
#undef EAI_FAIL
#undef EAI_FAMILY
#undef EAI_MEMORY
#undef EAI_NODATA
#undef EAI_NONAME
#undef EAI_SERVICE
#undef EAI_SOCKTYPE
#undef EAI_SYSTEM
#undef EAI_BADHINTS
#undef EAI_PROTOCOL
#undef EAI_MAX
#undef getaddrinfo
#define getaddrinfo fake_getaddrinfo
#endif /* EAI_ADDRFAMILY */

#define EAI_ADDRFAMILY   1      /* address family for hostname not supported */
#define EAI_AGAIN        2      /* temporary failure in name resolution */
#define EAI_BADFLAGS     3      /* invalid value for ai_flags */
#define EAI_FAIL         4      /* non-recoverable failure in name resolution */
#define EAI_FAMILY       5      /* ai_family not supported */
#define EAI_MEMORY       6      /* memory allocation failure */
#define EAI_NODATA       7      /* no address associated with hostname */
#define EAI_NONAME       8      /* hostname nor servname provided, or not known */
#define EAI_SERVICE      9      /* servname not supported for ai_socktype */
#define EAI_SOCKTYPE    10      /* ai_socktype not supported */
#define EAI_SYSTEM      11      /* system error returned in errno */
#define EAI_BADHINTS    12
#define EAI_PROTOCOL    13
#define EAI_MAX         14

/*
 * Flag values for getaddrinfo()
 */
#ifdef AI_PASSIVE
#undef AI_PASSIVE
#undef AI_CANONNAME
#undef AI_NUMERICHOST
#undef AI_MASK
#undef AI_ALL
#undef AI_V4MAPPED_CFG
#undef AI_ADDRCONFIG
#undef AI_V4MAPPED
#undef AI_DEFAULT
#endif /* AI_PASSIVE */

#define AI_PASSIVE      0x00000001 /* get address to use bind() */
#define AI_CANONNAME    0x00000002 /* fill ai_canonname */
#define AI_NUMERICHOST  0x00000004 /* prevent name resolution */
/* valid flags for addrinfo */
#define AI_MASK         (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST)

#define AI_ALL          0x00000100 /* IPv6 and IPv4-mapped (with AI_V4MAPPED) */
#define AI_V4MAPPED_CFG 0x00000200 /* accept IPv4-mapped if kernel supports */
#define AI_ADDRCONFIG   0x00000400 /* only if any address is assigned */
#define AI_V4MAPPED     0x00000800 /* accept IPv4-mapped IPv6 address */
/* special recommended flags for getipnodebyname */
#define AI_DEFAULT      (AI_V4MAPPED_CFG | AI_ADDRCONFIG)

#endif /* !HAVE_GETADDRINFO */

#ifndef HAVE_GETNAMEINFO

/*
 * Constants for getnameinfo()
 */
#ifndef NI_MAXHOST
#define NI_MAXHOST      1025
#define NI_MAXSERV      32
#endif /* !NI_MAXHOST */

/*
 * Flag values for getnameinfo()
 */
#ifndef NI_NOFQDN
#define NI_NOFQDN       0x00000001
#define NI_NUMERICHOST  0x00000002
#define NI_NAMEREQD     0x00000004
#define NI_NUMERICSERV  0x00000008
#define NI_DGRAM        0x00000010
#endif /* !NI_NOFQDN */

#endif /* !HAVE_GETNAMEINFO */

#ifndef HAVE_ADDRINFO
struct addrinfo {
    int         ai_flags;       /* AI_PASSIVE, AI_CANONNAME */
    int         ai_family;      /* PF_xxx */
    int         ai_socktype;    /* SOCK_xxx */
    int         ai_protocol;    /* 0 or IPPROTO_xxx for IPv4 and IPv6 */
    size_t      ai_addrlen;     /* length of ai_addr */
    char        *ai_canonname;  /* canonical name for hostname */
    struct sockaddr *ai_addr;           /* binary address */
    struct addrinfo *ai_next;           /* next structure in linked list */
};
#endif /* !HAVE_ADDRINFO */

#ifndef HAVE_SOCKADDR_STORAGE
/*
 * RFC 2553: protocol-independent placeholder for socket addresses
 */
#define _SS_MAXSIZE     128
#ifdef HAVE_LONG_LONG
#define _SS_ALIGNSIZE   (sizeof(PY_LONG_LONG))
#else
#define _SS_ALIGNSIZE   (sizeof(double))
#endif /* HAVE_LONG_LONG */
#define _SS_PAD1SIZE    (_SS_ALIGNSIZE - sizeof(u_char) * 2)
#define _SS_PAD2SIZE    (_SS_MAXSIZE - sizeof(u_char) * 2 - \
                _SS_PAD1SIZE - _SS_ALIGNSIZE)

struct sockaddr_storage {
#ifdef HAVE_SOCKADDR_SA_LEN
    unsigned char ss_len;               /* address length */
    unsigned char ss_family;            /* address family */
#else
    unsigned short ss_family;           /* address family */
#endif /* HAVE_SOCKADDR_SA_LEN */
    char        __ss_pad1[_SS_PAD1SIZE];
#ifdef HAVE_LONG_LONG
    PY_LONG_LONG __ss_align;            /* force desired structure storage alignment */
#else
    double __ss_align;          /* force desired structure storage alignment */
#endif /* HAVE_LONG_LONG */
    char        __ss_pad2[_SS_PAD2SIZE];
};
#endif /* !HAVE_SOCKADDR_STORAGE */

#ifdef __cplusplus
extern "C" {
#endif
extern void freehostent(struct hostent *);
#ifdef __cplusplus
}
#endif
