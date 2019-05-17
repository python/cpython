/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

/*
 * "#ifdef FAITH" part is local hack for supporting IPv4-v6 translator.
 *
 * Issues to be discussed:
 * - Thread safe-ness must be checked.
 * - Return values.  There are nonstandard return values defined and used
 *   in the source code.  This is because RFC2133 is silent about which error
 *   code must be returned for which situation.
 * - PF_UNSPEC case would be handled in getipnodebyname() with the AI_ALL flag.
 */

#if 0
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <resolv.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <unistd.h>

#include "addrinfo.h"
#endif

#if defined(__KAME__) && defined(ENABLE_IPV6)
# define FAITH
#endif

#define SUCCESS 0
#define GAI_ANY 0
#define YES 1
#define NO  0

#ifdef FAITH
static int translate = NO;
static struct in6_addr faith_prefix = IN6ADDR_GAI_ANY_INIT;
#endif

static const char in_addrany[] = { 0, 0, 0, 0 };
static const char in6_addrany[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static const char in_loopback[] = { 127, 0, 0, 1 };
static const char in6_loopback[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

struct sockinet {
    u_char      si_len;
    u_char      si_family;
    u_short     si_port;
};

static struct gai_afd {
    int a_af;
    int a_addrlen;
    int a_socklen;
    int a_off;
    const char *a_addrany;
    const char *a_loopback;
} gai_afdl [] = {
#ifdef ENABLE_IPV6
#define N_INET6 0
    {PF_INET6, sizeof(struct in6_addr),
     sizeof(struct sockaddr_in6),
     offsetof(struct sockaddr_in6, sin6_addr),
     in6_addrany, in6_loopback},
#define N_INET  1
#else
#define N_INET  0
#endif
    {PF_INET, sizeof(struct in_addr),
     sizeof(struct sockaddr_in),
     offsetof(struct sockaddr_in, sin_addr),
     in_addrany, in_loopback},
    {0, 0, 0, 0, NULL, NULL},
};

#ifdef ENABLE_IPV6
#define PTON_MAX        16
#else
#define PTON_MAX        4
#endif

#ifndef IN_MULTICAST
#define IN_MULTICAST(i)     (((i) & 0xf0000000U) == 0xe0000000U)
#endif

#ifndef IN_EXPERIMENTAL
#define IN_EXPERIMENTAL(i)  (((i) & 0xe0000000U) == 0xe0000000U)
#endif

#ifndef IN_LOOPBACKNET
#define IN_LOOPBACKNET      127
#endif

static int get_name(const char *, struct gai_afd *,
                          struct addrinfo **, char *, struct addrinfo *,
                          int);
static int get_addr(const char *, int, struct addrinfo **,
                        struct addrinfo *, int);
static int str_isnumber(const char *);

static const char * const ai_errlist[] = {
    "success.",
    "address family for hostname not supported.",       /* EAI_ADDRFAMILY */
    "temporary failure in name resolution.",            /* EAI_AGAIN      */
    "invalid value for ai_flags.",                      /* EAI_BADFLAGS   */
    "non-recoverable failure in name resolution.",      /* EAI_FAIL       */
    "ai_family not supported.",                         /* EAI_FAMILY     */
    "memory allocation failure.",                       /* EAI_MEMORY     */
    "no address associated with hostname.",             /* EAI_NODATA     */
    "hostname nor servname provided, or not known.",/* EAI_NONAME     */
    "servname not supported for ai_socktype.",          /* EAI_SERVICE    */
    "ai_socktype not supported.",                       /* EAI_SOCKTYPE   */
    "system error returned in errno.",                  /* EAI_SYSTEM     */
    "invalid value for hints.",                         /* EAI_BADHINTS   */
    "resolved protocol is unknown.",                    /* EAI_PROTOCOL   */
    "unknown error.",                                   /* EAI_MAX        */
};

#define GET_CANONNAME(ai, str) \
if (pai->ai_flags & AI_CANONNAME) {\
    if (((ai)->ai_canonname = (char *)malloc(strlen(str) + 1)) != NULL) {\
        strcpy((ai)->ai_canonname, (str));\
    } else {\
        error = EAI_MEMORY;\
        goto free;\
    }\
}

#ifdef HAVE_SOCKADDR_SA_LEN
#define GET_AI(ai, gai_afd, addr, port) {\
    char *p;\
    if (((ai) = (struct addrinfo *)malloc(sizeof(struct addrinfo) +\
                                          ((gai_afd)->a_socklen)))\
        == NULL) goto free;\
    memcpy(ai, pai, sizeof(struct addrinfo));\
    (ai)->ai_addr = (struct sockaddr *)((ai) + 1);\
    memset((ai)->ai_addr, 0, (gai_afd)->a_socklen);\
    (ai)->ai_addr->sa_len = (ai)->ai_addrlen = (gai_afd)->a_socklen;\
    (ai)->ai_addr->sa_family = (ai)->ai_family = (gai_afd)->a_af;\
    ((struct sockinet *)(ai)->ai_addr)->si_port = port;\
    p = (char *)((ai)->ai_addr);\
    memcpy(p + (gai_afd)->a_off, (addr), (gai_afd)->a_addrlen);\
}
#else
#define GET_AI(ai, gai_afd, addr, port) {\
    char *p;\
    if (((ai) = (struct addrinfo *)malloc(sizeof(struct addrinfo) +\
                                          ((gai_afd)->a_socklen)))\
        == NULL) goto free;\
    memcpy(ai, pai, sizeof(struct addrinfo));\
    (ai)->ai_addr = (struct sockaddr *)((ai) + 1);\
    memset((ai)->ai_addr, 0, (gai_afd)->a_socklen);\
    (ai)->ai_addrlen = (gai_afd)->a_socklen;\
    (ai)->ai_addr->sa_family = (ai)->ai_family = (gai_afd)->a_af;\
    ((struct sockinet *)(ai)->ai_addr)->si_port = port;\
    p = (char *)((ai)->ai_addr);\
    memcpy(p + (gai_afd)->a_off, (addr), (gai_afd)->a_addrlen);\
}
#endif

#define ERR(err) { error = (err); goto bad; }

const char *
gai_strerror(int ecode)
{
    if (ecode < 0 || ecode > EAI_MAX)
        ecode = EAI_MAX;
    return ai_errlist[ecode];
}

void
freeaddrinfo(struct addrinfo *ai)
{
    struct addrinfo *next;

    do {
        next = ai->ai_next;
        if (ai->ai_canonname)
            free(ai->ai_canonname);
        /* no need to free(ai->ai_addr) */
        free(ai);
    } while ((ai = next) != NULL);
}

static int
str_isnumber(const char *p)
{
    unsigned char *q = (unsigned char *)p;
    while (*q) {
        if (! isdigit(*q))
            return NO;
        q++;
    }
    return YES;
}

int
getaddrinfo(const char*hostname, const char*servname,
            const struct addrinfo *hints, struct addrinfo **res)
{
    struct addrinfo sentinel;
    struct addrinfo *top = NULL;
    struct addrinfo *cur;
    int i, error = 0;
    char pton[PTON_MAX];
    struct addrinfo ai;
    struct addrinfo *pai;
    u_short port;

#ifdef FAITH
    static int firsttime = 1;

    if (firsttime) {
        /* translator hack */
        {
            const char *q = getenv("GAI");
            if (q && inet_pton(AF_INET6, q, &faith_prefix) == 1)
                translate = YES;
        }
        firsttime = 0;
    }
#endif

    /* initialize file static vars */
    sentinel.ai_next = NULL;
    cur = &sentinel;
    pai = &ai;
    pai->ai_flags = 0;
    pai->ai_family = PF_UNSPEC;
    pai->ai_socktype = GAI_ANY;
    pai->ai_protocol = GAI_ANY;
    pai->ai_addrlen = 0;
    pai->ai_canonname = NULL;
    pai->ai_addr = NULL;
    pai->ai_next = NULL;
    port = GAI_ANY;

    if (hostname == NULL && servname == NULL)
        return EAI_NONAME;
    if (hints) {
        /* error check for hints */
        if (hints->ai_addrlen || hints->ai_canonname ||
            hints->ai_addr || hints->ai_next)
            ERR(EAI_BADHINTS); /* xxx */
        if (hints->ai_flags & ~AI_MASK)
            ERR(EAI_BADFLAGS);
        switch (hints->ai_family) {
        case PF_UNSPEC:
        case PF_INET:
#ifdef ENABLE_IPV6
        case PF_INET6:
#endif
            break;
        default:
            ERR(EAI_FAMILY);
        }
        memcpy(pai, hints, sizeof(*pai));
        switch (pai->ai_socktype) {
        case GAI_ANY:
            switch (pai->ai_protocol) {
            case GAI_ANY:
                break;
            case IPPROTO_UDP:
                pai->ai_socktype = SOCK_DGRAM;
                break;
            case IPPROTO_TCP:
                pai->ai_socktype = SOCK_STREAM;
                break;
            default:
                pai->ai_socktype = SOCK_RAW;
                break;
            }
            break;
        case SOCK_RAW:
            break;
        case SOCK_DGRAM:
            if (pai->ai_protocol != IPPROTO_UDP &&
                pai->ai_protocol != GAI_ANY)
                ERR(EAI_BADHINTS);                      /*xxx*/
            pai->ai_protocol = IPPROTO_UDP;
            break;
        case SOCK_STREAM:
            if (pai->ai_protocol != IPPROTO_TCP &&
                pai->ai_protocol != GAI_ANY)
                ERR(EAI_BADHINTS);                      /*xxx*/
            pai->ai_protocol = IPPROTO_TCP;
            break;
        default:
            ERR(EAI_SOCKTYPE);
            /* unreachable */
        }
    }

    /*
     * service port
     */
    if (servname) {
        if (str_isnumber(servname)) {
            if (pai->ai_socktype == GAI_ANY) {
                /* caller accept *GAI_ANY* socktype */
                pai->ai_socktype = SOCK_DGRAM;
                pai->ai_protocol = IPPROTO_UDP;
            }
            port = htons((u_short)atoi(servname));
        } else {
            struct servent *sp;
            const char *proto;

            proto = NULL;
            switch (pai->ai_socktype) {
            case GAI_ANY:
                proto = NULL;
                break;
            case SOCK_DGRAM:
                proto = "udp";
                break;
            case SOCK_STREAM:
                proto = "tcp";
                break;
            default:
                fprintf(stderr, "panic!\n");
                break;
            }
            if ((sp = getservbyname(servname, proto)) == NULL)
                ERR(EAI_SERVICE);
            port = sp->s_port;
            if (pai->ai_socktype == GAI_ANY) {
                if (strcmp(sp->s_proto, "udp") == 0) {
                    pai->ai_socktype = SOCK_DGRAM;
                    pai->ai_protocol = IPPROTO_UDP;
                } else if (strcmp(sp->s_proto, "tcp") == 0) {
                    pai->ai_socktype = SOCK_STREAM;
                    pai->ai_protocol = IPPROTO_TCP;
                } else
                    ERR(EAI_PROTOCOL);                          /*xxx*/
            }
        }
    }

    /*
     * hostname == NULL.
     * passive socket -> anyaddr (0.0.0.0 or ::)
     * non-passive socket -> localhost (127.0.0.1 or ::1)
     */
    if (hostname == NULL) {
        struct gai_afd *gai_afd;

        for (gai_afd = &gai_afdl[0]; gai_afd->a_af; gai_afd++) {
            if (!(pai->ai_family == PF_UNSPEC
               || pai->ai_family == gai_afd->a_af)) {
                continue;
            }

            if (pai->ai_flags & AI_PASSIVE) {
                GET_AI(cur->ai_next, gai_afd, gai_afd->a_addrany, port);
                /* xxx meaningless?
                 * GET_CANONNAME(cur->ai_next, "anyaddr");
                 */
            } else {
                GET_AI(cur->ai_next, gai_afd, gai_afd->a_loopback,
                    port);
                /* xxx meaningless?
                 * GET_CANONNAME(cur->ai_next, "localhost");
                 */
            }
            cur = cur->ai_next;
        }
        top = sentinel.ai_next;
        if (top)
            goto good;
        else
            ERR(EAI_FAMILY);
    }

    /* hostname as numeric name */
    for (i = 0; gai_afdl[i].a_af; i++) {
        if (inet_pton(gai_afdl[i].a_af, hostname, pton)) {
            u_long v4a;
#ifdef ENABLE_IPV6
            u_char pfx;
#endif

            switch (gai_afdl[i].a_af) {
            case AF_INET:
                v4a = ((struct in_addr *)pton)->s_addr;
                v4a = ntohl(v4a);
                if (IN_MULTICAST(v4a) || IN_EXPERIMENTAL(v4a))
                    pai->ai_flags &= ~AI_CANONNAME;
                v4a >>= IN_CLASSA_NSHIFT;
                if (v4a == 0 || v4a == IN_LOOPBACKNET)
                    pai->ai_flags &= ~AI_CANONNAME;
                break;
#ifdef ENABLE_IPV6
            case AF_INET6:
                pfx = ((struct in6_addr *)pton)->s6_addr[0];
                if (pfx == 0 || pfx == 0xfe || pfx == 0xff)
                    pai->ai_flags &= ~AI_CANONNAME;
                break;
#endif
            }

            if (pai->ai_family == gai_afdl[i].a_af ||
                pai->ai_family == PF_UNSPEC) {
                if (! (pai->ai_flags & AI_CANONNAME)) {
                    GET_AI(top, &gai_afdl[i], pton, port);
                    goto good;
                }
                /*
                 * if AI_CANONNAME and if reverse lookup
                 * fail, return ai anyway to pacify
                 * calling application.
                 *
                 * XXX getaddrinfo() is a name->address
                 * translation function, and it looks strange
                 * that we do addr->name translation here.
                 */
                get_name(pton, &gai_afdl[i], &top, pton, pai, port);
                goto good;
            } else
                ERR(EAI_FAMILY);                        /*xxx*/
        }
    }

    if (pai->ai_flags & AI_NUMERICHOST)
        ERR(EAI_NONAME);

    /* hostname as alphabetical name */
    error = get_addr(hostname, pai->ai_family, &top, pai, port);
    if (error == 0) {
        if (top) {
 good:
            *res = top;
            return SUCCESS;
        } else
            error = EAI_FAIL;
    }
 free:
    if (top)
        freeaddrinfo(top);
 bad:
    *res = NULL;
    return error;
}

static int
get_name(addr, gai_afd, res, numaddr, pai, port0)
    const char *addr;
    struct gai_afd *gai_afd;
    struct addrinfo **res;
    char *numaddr;
    struct addrinfo *pai;
    int port0;
{
    u_short port = port0 & 0xffff;
    struct hostent *hp;
    struct addrinfo *cur;
    int error = 0;
#ifdef ENABLE_IPV6
    int h_error;
#endif

#ifdef ENABLE_IPV6
    hp = getipnodebyaddr(addr, gai_afd->a_addrlen, gai_afd->a_af, &h_error);
#else
    hp = gethostbyaddr(addr, gai_afd->a_addrlen, AF_INET);
#endif
    if (hp && hp->h_name && hp->h_name[0] && hp->h_addr_list[0]) {
        GET_AI(cur, gai_afd, hp->h_addr_list[0], port);
        GET_CANONNAME(cur, hp->h_name);
    } else
        GET_AI(cur, gai_afd, numaddr, port);

#ifdef ENABLE_IPV6
    if (hp)
        freehostent(hp);
#endif
    *res = cur;
    return SUCCESS;
 free:
    if (cur)
        freeaddrinfo(cur);
#ifdef ENABLE_IPV6
    if (hp)
        freehostent(hp);
#endif
 /* bad: */
    *res = NULL;
    return error;
}

static int
get_addr(hostname, af, res, pai, port0)
    const char *hostname;
    int af;
    struct addrinfo **res;
    struct addrinfo *pai;
    int port0;
{
    u_short port = port0 & 0xffff;
    struct addrinfo sentinel;
    struct hostent *hp;
    struct addrinfo *top, *cur;
    struct gai_afd *gai_afd;
    int i, error = 0, h_error;
    char *ap;

    top = NULL;
    sentinel.ai_next = NULL;
    cur = &sentinel;
#ifdef ENABLE_IPV6
    if (af == AF_UNSPEC) {
        hp = getipnodebyname(hostname, AF_INET6,
                        AI_ADDRCONFIG|AI_ALL|AI_V4MAPPED, &h_error);
    } else
        hp = getipnodebyname(hostname, af, AI_ADDRCONFIG, &h_error);
#else
    hp = gethostbyname(hostname);
    h_error = h_errno;
#endif
    if (hp == NULL) {
        switch (h_error) {
        case HOST_NOT_FOUND:
        case NO_DATA:
            error = EAI_NODATA;
            break;
        case TRY_AGAIN:
            error = EAI_AGAIN;
            break;
        case NO_RECOVERY:
        default:
            error = EAI_FAIL;
            break;
        }
        goto free;
    }

    if ((hp->h_name == NULL) || (hp->h_name[0] == 0) ||
        (hp->h_addr_list[0] == NULL)) {
        error = EAI_FAIL;
        goto free;
    }

    for (i = 0; (ap = hp->h_addr_list[i]) != NULL; i++) {
        switch (af) {
#ifdef ENABLE_IPV6
        case AF_INET6:
            gai_afd = &gai_afdl[N_INET6];
            break;
#endif
#ifndef ENABLE_IPV6
        default:                /* AF_UNSPEC */
#endif
        case AF_INET:
            gai_afd = &gai_afdl[N_INET];
            break;
#ifdef ENABLE_IPV6
        default:                /* AF_UNSPEC */
            if (IN6_IS_ADDR_V4MAPPED((struct in6_addr *)ap)) {
                ap += sizeof(struct in6_addr) -
                    sizeof(struct in_addr);
                gai_afd = &gai_afdl[N_INET];
            } else
                gai_afd = &gai_afdl[N_INET6];
            break;
#endif
        }
#ifdef FAITH
        if (translate && gai_afd->a_af == AF_INET) {
            struct in6_addr *in6;

            GET_AI(cur->ai_next, &gai_afdl[N_INET6], ap, port);
            in6 = &((struct sockaddr_in6 *)cur->ai_next->ai_addr)->sin6_addr;
            memcpy(&in6->s6_addr32[0], &faith_prefix,
                sizeof(struct in6_addr) - sizeof(struct in_addr));
            memcpy(&in6->s6_addr32[3], ap, sizeof(struct in_addr));
        } else
#endif /* FAITH */
        GET_AI(cur->ai_next, gai_afd, ap, port);
        if (cur == &sentinel) {
            top = cur->ai_next;
            GET_CANONNAME(top, hp->h_name);
        }
        cur = cur->ai_next;
    }
#ifdef ENABLE_IPV6
    freehostent(hp);
#endif
    *res = top;
    return SUCCESS;
 free:
    if (top)
        freeaddrinfo(top);
#ifdef ENABLE_IPV6
    if (hp)
        freehostent(hp);
#endif
/* bad: */
    *res = NULL;
    return error;
}
