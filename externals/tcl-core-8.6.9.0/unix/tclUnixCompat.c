/*
 * tclUnixCompat.c
 *
 * Written by: Zoran Vasiljevic (vasiljevic@users.sourceforge.net).
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>

/*
 * See also: SC_BLOCKING_STYLE in unix/tcl.m4
 */

#ifdef	USE_FIONBIO
#   ifdef HAVE_SYS_FILIO_H
#	include	<sys/filio.h>	/* For FIONBIO. */
#   endif
#   ifdef HAVE_SYS_IOCTL_H
#	include	<sys/ioctl.h>
#   endif
#endif	/* USE_FIONBIO */

/*
 * Used to pad structures at size'd boundaries
 *
 * This macro assumes that the pointer 'buffer' was created from an aligned
 * pointer by adding the 'length'. If this 'length' was not a multiple of the
 * 'size' the result is unaligned and PadBuffer corrects both the pointer,
 * _and_ the 'length'. The latter means that future increments of 'buffer' by
 * 'length' stay aligned.
 */

#define PadBuffer(buffer, length, size)			\
    if (((length) % (size))) {				\
	(buffer) += ((size) - ((length) % (size)));	\
	(length) += ((size) - ((length) % (size)));	\
    }

/*
 * Per-thread private storage used to store values returned from MT-unsafe
 * library calls.
 */

#ifdef TCL_THREADS

typedef struct ThreadSpecificData {
    struct passwd pwd;
#if defined(HAVE_GETPWNAM_R_5) || defined(HAVE_GETPWUID_R_5)
#define NEED_PW_CLEANER 1
    char *pbuf;
    int pbuflen;
#else
    char pbuf[2048];
#endif

    struct group grp;
#if defined(HAVE_GETGRNAM_R_5) || defined(HAVE_GETGRGID_R_5)
#define NEED_GR_CLEANER 1
    char *gbuf;
    int gbuflen;
#else
    char gbuf[2048];
#endif

#if !defined(HAVE_MTSAFE_GETHOSTBYNAME) || !defined(HAVE_MTSAFE_GETHOSTBYADDR)
    struct hostent hent;
    char hbuf[2048];
#endif
}  ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

#if ((!defined(HAVE_GETHOSTBYNAME_R) || !defined(HAVE_GETHOSTBYADDR_R)) && \
     (!defined(HAVE_MTSAFE_GETHOSTBYNAME) || \
      !defined(HAVE_MTSAFE_GETHOSTBYADDR))) || \
      !defined(HAVE_GETPWNAM_R) || !defined(HAVE_GETPWUID_R) || \
      !defined(HAVE_GETGRNAM_R) || !defined(HAVE_GETGRGID_R)
/*
 * Mutex to lock access to MT-unsafe calls. This is just to protect our own
 * usage. It does not protect us from others calling the same functions
 * without (or using some different) lock.
 */

static Tcl_Mutex compatLock;

/*
 * Helper function declarations. Note that these are only used if needed and
 * only defined if used (via the NEED_* macros).
 */

#undef NEED_COPYARRAY
#undef NEED_COPYGRP
#undef NEED_COPYHOSTENT
#undef NEED_COPYPWD
#undef NEED_COPYSTRING

#if !defined(HAVE_GETGRNAM_R_5) && !defined(HAVE_GETGRNAM_R_4)
#define NEED_COPYGRP 1
static int		CopyGrp(struct group *tgtPtr, char *buf, int buflen);
#endif

#if !defined(HAVE_GETPWNAM_R_5) && !defined(HAVE_GETPWNAM_R_4)
#define NEED_COPYPWD 1
static int		CopyPwd(struct passwd *tgtPtr, char *buf, int buflen);
#endif

static int		CopyArray(char **src, int elsize, char *buf,
			    int buflen);
static int		CopyHostent(struct hostent *tgtPtr, char *buf,
			    int buflen);
static int		CopyString(const char *src, char *buf, int buflen);

#endif

#ifdef NEED_PW_CLEANER
static void		FreePwBuf(ClientData ignored);
#endif
#ifdef NEED_GR_CLEANER
static void		FreeGrBuf(ClientData ignored);
#endif
#endif /* TCL_THREADS */

/*
 *---------------------------------------------------------------------------
 *
 * TclUnixSetBlockingMode --
 *
 *	Set the blocking mode of a file descriptor.
 *
 * Results:
 *
 *	0 on success, -1 (with errno set) on error.
 *
 *---------------------------------------------------------------------------
 */

int
TclUnixSetBlockingMode(
    int fd,			/* File descriptor */
    int mode)			/* Either TCL_MODE_BLOCKING or
				 * TCL_MODE_NONBLOCKING. */
{
#ifndef USE_FIONBIO
    int flags = fcntl(fd, F_GETFL);

    if (mode == TCL_MODE_BLOCKING) {
	flags &= ~O_NONBLOCK;
    } else {
	flags |= O_NONBLOCK;
    }
    return fcntl(fd, F_SETFL, flags);
#else /* USE_FIONBIO */
    int state = (mode == TCL_MODE_NONBLOCKING);

    return ioctl(fd, FIONBIO, &state);
#endif /* !USE_FIONBIO */
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetPwNam --
 *
 *      Thread-safe wrappers for getpwnam(). See "man getpwnam" for more
 *      details.
 *
 * Results:
 *      Pointer to struct passwd on success or NULL on error.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

struct passwd *
TclpGetPwNam(
    const char *name)
{
#if !defined(TCL_THREADS)
    return getpwnam(name);
#else
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

#if defined(HAVE_GETPWNAM_R_5)
    struct passwd *pwPtr = NULL;

    /*
     * How to allocate a buffer of the right initial size. If you want the
     * gory detail, see http://www.opengroup.org/austin/docs/austin_328.txt
     * and weep.
     */

    if (tsdPtr->pbuf == NULL) {
	tsdPtr->pbuflen = (int) sysconf(_SC_GETPW_R_SIZE_MAX);
	if (tsdPtr->pbuflen < 1) {
	    tsdPtr->pbuflen = 1024;
	}
	tsdPtr->pbuf = ckalloc(tsdPtr->pbuflen);
	Tcl_CreateThreadExitHandler(FreePwBuf, NULL);
    }
    while (1) {
	int e = getpwnam_r(name, &tsdPtr->pwd, tsdPtr->pbuf, tsdPtr->pbuflen,
		&pwPtr);

	if (e == 0) {
	    break;
	} else if (e != ERANGE) {
	    return NULL;
	}
	tsdPtr->pbuflen *= 2;
	tsdPtr->pbuf = ckrealloc(tsdPtr->pbuf, tsdPtr->pbuflen);
    }
    return (pwPtr != NULL ? &tsdPtr->pwd : NULL);

#elif defined(HAVE_GETPWNAM_R_4)
    return getpwnam_r(name, &tsdPtr->pwd, tsdPtr->pbuf, sizeof(tsdPtr->pbuf));

#else
    struct passwd *pwPtr;

    Tcl_MutexLock(&compatLock);
    pwPtr = getpwnam(name);
    if (pwPtr != NULL) {
	tsdPtr->pwd = *pwPtr;
	pwPtr = &tsdPtr->pwd;
	if (CopyPwd(&tsdPtr->pwd, tsdPtr->pbuf, sizeof(tsdPtr->pbuf)) == -1) {
	    pwPtr = NULL;
	}
    }
    Tcl_MutexUnlock(&compatLock);
    return pwPtr;
#endif

    return NULL;		/* Not reached. */
#endif /* TCL_THREADS */
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetPwUid --
 *
 *      Thread-safe wrappers for getpwuid(). See "man getpwuid" for more
 *      details.
 *
 * Results:
 *      Pointer to struct passwd on success or NULL on error.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

struct passwd *
TclpGetPwUid(
    uid_t uid)
{
#if !defined(TCL_THREADS)
    return getpwuid(uid);
#else
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

#if defined(HAVE_GETPWUID_R_5)
    struct passwd *pwPtr = NULL;

    /*
     * How to allocate a buffer of the right initial size. If you want the
     * gory detail, see http://www.opengroup.org/austin/docs/austin_328.txt
     * and weep.
     */

    if (tsdPtr->pbuf == NULL) {
	tsdPtr->pbuflen = (int) sysconf(_SC_GETPW_R_SIZE_MAX);
	if (tsdPtr->pbuflen < 1) {
	    tsdPtr->pbuflen = 1024;
	}
	tsdPtr->pbuf = ckalloc(tsdPtr->pbuflen);
	Tcl_CreateThreadExitHandler(FreePwBuf, NULL);
    }
    while (1) {
	int e = getpwuid_r(uid, &tsdPtr->pwd, tsdPtr->pbuf, tsdPtr->pbuflen,
		&pwPtr);

	if (e == 0) {
	    break;
	} else if (e != ERANGE) {
	    return NULL;
	}
	tsdPtr->pbuflen *= 2;
	tsdPtr->pbuf = ckrealloc(tsdPtr->pbuf, tsdPtr->pbuflen);
    }
    return (pwPtr != NULL ? &tsdPtr->pwd : NULL);

#elif defined(HAVE_GETPWUID_R_4)
    return getpwuid_r(uid, &tsdPtr->pwd, tsdPtr->pbuf, sizeof(tsdPtr->pbuf));

#else
    struct passwd *pwPtr;

    Tcl_MutexLock(&compatLock);
    pwPtr = getpwuid(uid);
    if (pwPtr != NULL) {
	tsdPtr->pwd = *pwPtr;
	pwPtr = &tsdPtr->pwd;
	if (CopyPwd(&tsdPtr->pwd, tsdPtr->pbuf, sizeof(tsdPtr->pbuf)) == -1) {
	    pwPtr = NULL;
	}
    }
    Tcl_MutexUnlock(&compatLock);
    return pwPtr;
#endif

    return NULL;		/* Not reached. */
#endif /* TCL_THREADS */
}

/*
 *---------------------------------------------------------------------------
 *
 * FreePwBuf --
 *
 *	Helper that is used to dispose of space allocated and referenced from
 *	the ThreadSpecificData for user entries. (Darn that baroque POSIX
 *	reentrant interface.)
 *
 *---------------------------------------------------------------------------
 */

#ifdef NEED_PW_CLEANER
static void
FreePwBuf(
    ClientData ignored)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    ckfree(tsdPtr->pbuf);
}
#endif /* NEED_PW_CLEANER */

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetGrNam --
 *
 *      Thread-safe wrappers for getgrnam(). See "man getgrnam" for more
 *      details.
 *
 * Results:
 *      Pointer to struct group on success or NULL on error.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

struct group *
TclpGetGrNam(
    const char *name)
{
#if !defined(TCL_THREADS)
    return getgrnam(name);
#else
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

#if defined(HAVE_GETGRNAM_R_5)
    struct group *grPtr = NULL;

    /*
     * How to allocate a buffer of the right initial size. If you want the
     * gory detail, see http://www.opengroup.org/austin/docs/austin_328.txt
     * and weep.
     */

    if (tsdPtr->gbuf == NULL) {
	tsdPtr->gbuflen = (int) sysconf(_SC_GETGR_R_SIZE_MAX);
	if (tsdPtr->gbuflen < 1) {
	    tsdPtr->gbuflen = 1024;
	}
	tsdPtr->gbuf = ckalloc(tsdPtr->gbuflen);
	Tcl_CreateThreadExitHandler(FreeGrBuf, NULL);
    }
    while (1) {
	int e = getgrnam_r(name, &tsdPtr->grp, tsdPtr->gbuf, tsdPtr->gbuflen,
		&grPtr);

	if (e == 0) {
	    break;
	} else if (e != ERANGE) {
	    return NULL;
	}
	tsdPtr->gbuflen *= 2;
	tsdPtr->gbuf = ckrealloc(tsdPtr->gbuf, tsdPtr->gbuflen);
    }
    return (grPtr != NULL ? &tsdPtr->grp : NULL);

#elif defined(HAVE_GETGRNAM_R_4)
    return getgrnam_r(name, &tsdPtr->grp, tsdPtr->gbuf, sizeof(tsdPtr->gbuf));

#else
    struct group *grPtr;

    Tcl_MutexLock(&compatLock);
    grPtr = getgrnam(name);
    if (grPtr != NULL) {
	tsdPtr->grp = *grPtr;
	grPtr = &tsdPtr->grp;
	if (CopyGrp(&tsdPtr->grp, tsdPtr->gbuf, sizeof(tsdPtr->gbuf)) == -1) {
	    grPtr = NULL;
	}
    }
    Tcl_MutexUnlock(&compatLock);
    return grPtr;
#endif

    return NULL;		/* Not reached. */
#endif /* TCL_THREADS */
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetGrGid --
 *
 *      Thread-safe wrappers for getgrgid(). See "man getgrgid" for more
 *      details.
 *
 * Results:
 *      Pointer to struct group on success or NULL on error.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

struct group *
TclpGetGrGid(
    gid_t gid)
{
#if !defined(TCL_THREADS)
    return getgrgid(gid);
#else
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

#if defined(HAVE_GETGRGID_R_5)
    struct group *grPtr = NULL;

    /*
     * How to allocate a buffer of the right initial size. If you want the
     * gory detail, see http://www.opengroup.org/austin/docs/austin_328.txt
     * and weep.
     */

    if (tsdPtr->gbuf == NULL) {
	tsdPtr->gbuflen = (int) sysconf(_SC_GETGR_R_SIZE_MAX);
	if (tsdPtr->gbuflen < 1) {
	    tsdPtr->gbuflen = 1024;
	}
	tsdPtr->gbuf = ckalloc(tsdPtr->gbuflen);
	Tcl_CreateThreadExitHandler(FreeGrBuf, NULL);
    }
    while (1) {
	int e = getgrgid_r(gid, &tsdPtr->grp, tsdPtr->gbuf, tsdPtr->gbuflen,
		&grPtr);

	if (e == 0) {
	    break;
	} else if (e != ERANGE) {
	    return NULL;
	}
	tsdPtr->gbuflen *= 2;
	tsdPtr->gbuf = ckrealloc(tsdPtr->gbuf, tsdPtr->gbuflen);
    }
    return (grPtr != NULL ? &tsdPtr->grp : NULL);

#elif defined(HAVE_GETGRGID_R_4)
    return getgrgid_r(gid, &tsdPtr->grp, tsdPtr->gbuf, sizeof(tsdPtr->gbuf));

#else
    struct group *grPtr;

    Tcl_MutexLock(&compatLock);
    grPtr = getgrgid(gid);
    if (grPtr != NULL) {
	tsdPtr->grp = *grPtr;
	grPtr = &tsdPtr->grp;
	if (CopyGrp(&tsdPtr->grp, tsdPtr->gbuf, sizeof(tsdPtr->gbuf)) == -1) {
	    grPtr = NULL;
	}
    }
    Tcl_MutexUnlock(&compatLock);
    return grPtr;
#endif

    return NULL;		/* Not reached. */
#endif /* TCL_THREADS */
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeGrBuf --
 *
 *	Helper that is used to dispose of space allocated and referenced from
 *	the ThreadSpecificData for group entries. (Darn that baroque POSIX
 *	reentrant interface.)
 *
 *---------------------------------------------------------------------------
 */

#ifdef NEED_GR_CLEANER
static void
FreeGrBuf(
    ClientData ignored)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    ckfree(tsdPtr->gbuf);
}
#endif /* NEED_GR_CLEANER */

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetHostByName --
 *
 *      Thread-safe wrappers for gethostbyname(). See "man gethostbyname" for
 *      more details.
 *
 * Results:
 *      Pointer to struct hostent on success or NULL on error.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

struct hostent *
TclpGetHostByName(
    const char *name)
{
#if !defined(TCL_THREADS) || defined(HAVE_MTSAFE_GETHOSTBYNAME)
    return gethostbyname(name);
#else
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

#if defined(HAVE_GETHOSTBYNAME_R_5)
    int h_errno;

    return gethostbyname_r(name, &tsdPtr->hent, tsdPtr->hbuf,
			   sizeof(tsdPtr->hbuf), &h_errno);

#elif defined(HAVE_GETHOSTBYNAME_R_6)
    struct hostent *hePtr = NULL;
    int h_errno, result;

    result = gethostbyname_r(name, &tsdPtr->hent, tsdPtr->hbuf,
	    sizeof(tsdPtr->hbuf), &hePtr, &h_errno);
    return (result == 0) ? hePtr : NULL;

#elif defined(HAVE_GETHOSTBYNAME_R_3)
    struct hostent_data data;

    return (gethostbyname_r(name, &tsdPtr->hent, &data) == 0)
	    ? &tsdPtr->hent : NULL;

#else
#define NEED_COPYHOSTENT 1
    struct hostent *hePtr;

    Tcl_MutexLock(&compatLock);
    hePtr = gethostbyname(name);
    if (hePtr != NULL) {
	tsdPtr->hent = *hePtr;
	hePtr = &tsdPtr->hent;
	if (CopyHostent(&tsdPtr->hent, tsdPtr->hbuf,
		sizeof(tsdPtr->hbuf)) == -1) {
	    hePtr = NULL;
	}
    }
    Tcl_MutexUnlock(&compatLock);
    return hePtr;
#endif

    return NULL;		/* Not reached. */
#endif /* TCL_THREADS */
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetHostByAddr --
 *
 *      Thread-safe wrappers for gethostbyaddr(). See "man gethostbyaddr" for
 *      more details.
 *
 * Results:
 *      Pointer to struct hostent on success or NULL on error.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

struct hostent *
TclpGetHostByAddr(
    const char *addr,
    int length,
    int type)
{
#if !defined(TCL_THREADS) || defined(HAVE_MTSAFE_GETHOSTBYADDR)
    return gethostbyaddr(addr, length, type);
#else
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

#if defined(HAVE_GETHOSTBYADDR_R_7)
    int h_errno;

    return gethostbyaddr_r(addr, length, type, &tsdPtr->hent, tsdPtr->hbuf,
	    sizeof(tsdPtr->hbuf), &h_errno);

#elif defined(HAVE_GETHOSTBYADDR_R_8)
    struct hostent *hePtr;
    int h_errno;

    return (gethostbyaddr_r(addr, length, type, &tsdPtr->hent, tsdPtr->hbuf,
		sizeof(tsdPtr->hbuf), &hePtr, &h_errno) == 0)
	    ? &tsdPtr->hent : NULL;
#else
#define NEED_COPYHOSTENT 1
    struct hostent *hePtr;

    Tcl_MutexLock(&compatLock);
    hePtr = gethostbyaddr(addr, length, type);
    if (hePtr != NULL) {
	tsdPtr->hent = *hePtr;
	hePtr = &tsdPtr->hent;
	if (CopyHostent(&tsdPtr->hent, tsdPtr->hbuf,
		sizeof(tsdPtr->hbuf)) == -1) {
	    hePtr = NULL;
	}
    }
    Tcl_MutexUnlock(&compatLock);
    return hePtr;
#endif

    return NULL;		/* Not reached. */
#endif /* TCL_THREADS */
}

/*
 *---------------------------------------------------------------------------
 *
 * CopyGrp --
 *
 *      Copies string fields of the group structure to the private buffer,
 *      honouring the size of the buffer.
 *
 * Results:
 *      0 on success or -1 on error (errno = ERANGE).
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

#ifdef NEED_COPYGRP
#define NEED_COPYARRAY 1
#define NEED_COPYSTRING 1

static int
CopyGrp(
    struct group *tgtPtr,
    char *buf,
    int buflen)
{
    register char *p = buf;
    register int copied, len = 0;

    /*
     * Copy username.
     */

    copied = CopyString(tgtPtr->gr_name, p, buflen - len);
    if (copied == -1) {
	goto range;
    }
    tgtPtr->gr_name = (copied > 0) ? p : NULL;
    len += copied;
    p = buf + len;

    /*
     * Copy password.
     */

    copied = CopyString(tgtPtr->gr_passwd, p, buflen - len);
    if (copied == -1) {
	goto range;
    }
    tgtPtr->gr_passwd = (copied > 0) ? p : NULL;
    len += copied;
    p = buf + len;

    /*
     * Copy group members.
     */

    PadBuffer(p, len, sizeof(char *));
    copied = CopyArray((char **)tgtPtr->gr_mem, -1, p, buflen - len);
    if (copied == -1) {
	goto range;
    }
    tgtPtr->gr_mem = (copied > 0) ? (char **)p : NULL;

    return 0;

  range:
    errno = ERANGE;
    return -1;
}
#endif /* NEED_COPYGRP */

/*
 *---------------------------------------------------------------------------
 *
 * CopyHostent --
 *
 *      Copies string fields of the hostnent structure to the private buffer,
 *      honouring the size of the buffer.
 *
 * Results:
 *      Number of bytes copied on success or -1 on error (errno = ERANGE)
 *
 * Side effects:
 *      None
 *
 *---------------------------------------------------------------------------
 */

#ifdef NEED_COPYHOSTENT
#define NEED_COPYSTRING 1
#define NEED_COPYARRAY 1

static int
CopyHostent(
    struct hostent *tgtPtr,
    char *buf,
    int buflen)
{
    char *p = buf;
    int copied, len = 0;

    copied = CopyString(tgtPtr->h_name, p, buflen - len);
    if (copied == -1) {
	goto range;
    }
    tgtPtr->h_name = (copied > 0) ? p : NULL;
    len += copied;
    p = buf + len;

    PadBuffer(p, len, sizeof(char *));
    copied = CopyArray(tgtPtr->h_aliases, -1, p, buflen - len);
    if (copied == -1) {
	goto range;
    }
    tgtPtr->h_aliases = (copied > 0) ? (char **)p : NULL;
    len += copied;
    p += len;

    PadBuffer(p, len, sizeof(char *));
    copied = CopyArray(tgtPtr->h_addr_list, tgtPtr->h_length, p, buflen-len);
    if (copied == -1) {
	goto range;
    }
    tgtPtr->h_addr_list = (copied > 0) ? (char **)p : NULL;

    return 0;

  range:
    errno = ERANGE;
    return -1;
}
#endif /* NEED_COPYHOSTENT */

/*
 *---------------------------------------------------------------------------
 *
 * CopyPwd --
 *
 *      Copies string fields of the passwd structure to the private buffer,
 *      honouring the size of the buffer.
 *
 * Results:
 *      0 on success or -1 on error (errno = ERANGE).
 *
 * Side effects:
 *      We are not copying the gecos field as it may not be supported on all
 *      platforms.
 *
 *---------------------------------------------------------------------------
 */

#ifdef NEED_COPYPWD
#define NEED_COPYSTRING 1

static int
CopyPwd(
    struct passwd *tgtPtr,
    char *buf,
    int buflen)
{
    char *p = buf;
    int copied, len = 0;

    copied = CopyString(tgtPtr->pw_name, p, buflen - len);
    if (copied == -1) {
    range:
	errno = ERANGE;
	return -1;
    }
    tgtPtr->pw_name = (copied > 0) ? p : NULL;
    len += copied;
    p = buf + len;

    copied = CopyString(tgtPtr->pw_passwd, p, buflen - len);
    if (copied == -1) {
	goto range;
    }
    tgtPtr->pw_passwd = (copied > 0) ? p : NULL;
    len += copied;
    p = buf + len;

    copied = CopyString(tgtPtr->pw_dir, p, buflen - len);
    if (copied == -1) {
	goto range;
    }
    tgtPtr->pw_dir = (copied > 0) ? p : NULL;
    len += copied;
    p = buf + len;

    copied = CopyString(tgtPtr->pw_shell, p, buflen - len);
    if (copied == -1) {
	goto range;
    }
    tgtPtr->pw_shell = (copied > 0) ? p : NULL;

    return 0;
}
#endif /* NEED_COPYPWD */

/*
 *---------------------------------------------------------------------------
 *
 * CopyArray --
 *
 *      Copies array of NULL-terminated or fixed-length strings to the private
 *      buffer, honouring the size of the buffer.
 *
 * Results:
 *      Number of bytes copied on success or -1 on error (errno = ERANGE)
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

#ifdef NEED_COPYARRAY
static int
CopyArray(
    char **src,			/* Array of elements to copy. */
    int elsize,			/* Size of each element, or -1 to indicate
				 * that they are C strings of dynamic
				 * length. */
    char *buf,			/* Buffer to copy into. */
    int buflen)			/* Size of buffer. */
{
    int i, j, len = 0;
    char *p, **new;

    if (src == NULL) {
	return 0;
    }

    for (i = 0; src[i] != NULL; i++) {
	/*
	 * Empty loop to count how many.
	 */
    }
    len = sizeof(char *) * (i + 1);	/* Leave place for the array. */
    if (len >  buflen) {
	return -1;
    }

    new = (char **) buf;
    p = buf + len;

    for (j = 0; j < i; j++) {
	int sz = (elsize<0 ? (int) strlen(src[j]) + 1 : elsize);

	len += sz;
	if (len > buflen) {
	    return -1;
	}
	memcpy(p, src[j], sz);
	new[j] = p;
	p = buf + len;
    }
    new[j] = NULL;

    return len;
}
#endif /* NEED_COPYARRAY */

/*
 *---------------------------------------------------------------------------
 *
 * CopyString --
 *
 *      Copies a NULL-terminated string to the private buffer, honouring the
 *      size of the buffer
 *
 * Results:
 *      0 success or -1 on error (errno = ERANGE)
 *
 * Side effects:
 *      None
 *
 *---------------------------------------------------------------------------
 */

#ifdef NEED_COPYSTRING
static int
CopyString(
    const char *src,		/* String to copy. */
    char *buf,			/* Buffer to copy into. */
    int buflen)			/* Size of buffer. */
{
    int len = 0;

    if (src != NULL) {
	len = strlen(src) + 1;
	if (len > buflen) {
	    return -1;
	}
	memcpy(buf, src, len);
    }

    return len;
}
#endif /* NEED_COPYSTRING */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */

/*
 *------------------------------------------------------------------------
 *
 * TclWinCPUID --
 *
 *	Get CPU ID information on an Intel box under UNIX (either Linux or Cygwin)
 *
 * Results:
 *	Returns TCL_OK if successful, TCL_ERROR if CPUID is not supported.
 *
 * Side effects:
 *	If successful, stores EAX, EBX, ECX and EDX registers after the CPUID
 *	instruction in the four integers designated by 'regsPtr'
 *
 *----------------------------------------------------------------------
 */

int
TclWinCPUID(
    unsigned int index,		/* Which CPUID value to retrieve. */
    unsigned int *regsPtr)	/* Registers after the CPUID. */
{
    int status = TCL_ERROR;

    /* See: <http://en.wikipedia.org/wiki/CPUID> */
#if defined(HAVE_CPUID)
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
    __asm__ __volatile__("movq %%rbx, %%rsi     \n\t" /* save %rbx */
                 "cpuid            \n\t"
                 "xchgq %%rsi, %%rbx   \n\t" /* restore the old %rbx */
                 : "=a"(regsPtr[0]), "=S"(regsPtr[1]), "=c"(regsPtr[2]), "=d"(regsPtr[3])
                 : "a"(index));
#else
    __asm__ __volatile__("mov %%ebx, %%esi     \n\t" /* save %ebx */
                 "cpuid            \n\t"
                 "xchg %%esi, %%ebx   \n\t" /* restore the old %ebx */
                 : "=a"(regsPtr[0]), "=S"(regsPtr[1]), "=c"(regsPtr[2]), "=d"(regsPtr[3])
                 : "a"(index));
#endif
    status = TCL_OK;
#endif
    return status;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
