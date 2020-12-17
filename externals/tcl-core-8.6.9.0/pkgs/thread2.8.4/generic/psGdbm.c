/*
 * This file implements wrappers for persistent gdbm storage for the
 * shared variable arrays.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ----------------------------------------------------------------------------
 */

#ifdef HAVE_GDBM

#include "threadSvCmd.h"
#include <gdbm.h>
#include <stdlib.h> /* For free() */

/*
 * Functions implementing the persistent store interface
 */

static ps_open_proc   ps_gdbm_open;
static ps_close_proc  ps_gdbm_close;
static ps_get_proc    ps_gdbm_get;
static ps_put_proc    ps_gdbm_put;
static ps_first_proc  ps_gdbm_first;
static ps_next_proc   ps_gdbm_next;
static ps_delete_proc ps_gdbm_delete;
static ps_free_proc   ps_gdbm_free;
static ps_geterr_proc ps_gdbm_geterr;

/*
 * This structure collects all the various pointers
 * to the functions implementing the gdbm store.
 */

const PsStore GdbmStore = {
    "gdbm",
    NULL,
    ps_gdbm_open,
    ps_gdbm_get,
    ps_gdbm_put,
    ps_gdbm_first,
    ps_gdbm_next,
    ps_gdbm_delete,
    ps_gdbm_close,
    ps_gdbm_free,
    ps_gdbm_geterr,
    NULL
};

/*
 *-----------------------------------------------------------------------------
 *
 * Sv_RegisterGdbmStore --
 *
 *      Register the gdbm store with shared variable implementation.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
void
Sv_RegisterGdbmStore(void)
{
    Sv_RegisterPsStore(&GdbmStore);
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_gdbm_open --
 *
 *      Opens the dbm-based persistent storage.
 *
 * Results:
 *      Opaque handle of the opened dbm storage.
 *
 * Side effects:
 *      The gdbm file might be created if not found.
 *
 *-----------------------------------------------------------------------------
 */
static ClientData
ps_gdbm_open(
    const char *path)
{
    GDBM_FILE dbf;
    char *ext;
    Tcl_DString toext;

    Tcl_DStringInit(&toext);
    ext = Tcl_UtfToExternalDString(NULL, path, strlen(path), &toext);
    dbf = gdbm_open(ext, 512, GDBM_WRCREAT|GDBM_SYNC|GDBM_NOLOCK, 0666, NULL);
    Tcl_DStringFree(&toext);

    return (ClientData)dbf;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_gdbm_close --
 *
 *      Closes the gdbm-based persistent storage.
 *
 * Results:
 *      0 - ok
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static int
ps_gdbm_close(
    ClientData handle)
{
    gdbm_close((GDBM_FILE)handle);

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_gdbm_get --
 *
 *      Retrieves data for the key from the dbm storage.
 *
 * Results:
 *      1 - no such key
 *      0 - ok
 *
 * Side effects:
 *      Data returned must be freed by the caller.
 *
 *-----------------------------------------------------------------------------
 */
static int
ps_gdbm_get(
     ClientData handle,
     const char   *key,
     char **dataptrptr,
     size_t    *lenptr)
{
    GDBM_FILE dbf = (GDBM_FILE)handle;
    datum drec, dkey;

    dkey.dptr  = (char*)key;
    dkey.dsize = strlen(key) + 1;

    drec = gdbm_fetch(dbf, dkey);
    if (drec.dptr == NULL) {
        return 1;
    }

    *dataptrptr = drec.dptr;
    *lenptr = drec.dsize;

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_gdbm_first --
 *
 *      Starts the iterator over the dbm file and returns the first record.
 *
 * Results:
 *      1 - no more records in the iterator
 *      0 - ok
 *
 * Side effects:
 *      Data returned must be freed by the caller.
 *
 *-----------------------------------------------------------------------------
 */
static int
ps_gdbm_first(
    ClientData  handle,
    char   **keyptrptr,
    char  **dataptrptr,
    size_t     *lenptr)
{
    GDBM_FILE dbf = (GDBM_FILE)handle;
    datum drec, dkey;

    dkey = gdbm_firstkey(dbf);
    if (dkey.dptr == NULL) {
        return 1;
    }
    drec = gdbm_fetch(dbf, dkey);
    if (drec.dptr == NULL) {
        return 1;
    }

    *dataptrptr = drec.dptr;
    *lenptr = drec.dsize;
    *keyptrptr = dkey.dptr;

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_gdbm_next --
 *
 *      Uses the iterator over the dbm file and returns the next record.
 *
 * Results:
 *      1 - no more records in the iterator
 *      0 - ok
 *
 * Side effects:
 *      Data returned must be freed by the caller.
 *
 *-----------------------------------------------------------------------------
 */
static int ps_gdbm_next(
    ClientData  handle,
    char   **keyptrptr,
    char  **dataptrptr,
    size_t     *lenptr)
{
    GDBM_FILE dbf = (GDBM_FILE)handle;
    datum drec, dkey, dnext;

    dkey.dptr  = *keyptrptr;
    dkey.dsize = strlen(*keyptrptr) + 1;

    dnext = gdbm_nextkey(dbf, dkey);
    free(*keyptrptr), *keyptrptr = NULL;

    if (dnext.dptr == NULL) {
        return 1;
    }
    drec = gdbm_fetch(dbf, dnext);
    if (drec.dptr == NULL) {
        return 1;
    }

    *dataptrptr = drec.dptr;
    *lenptr = drec.dsize;
    *keyptrptr = dnext.dptr;

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_gdbm_put --
 *
 *      Stores used data bound to a key in dbm storage.
 *
 * Results:
 *      0 - ok
 *     -1 - error; use ps_dbm_geterr to retrieve the error message
 *
 * Side effects:
 *      If the key is already associated with some user data, this will
 *      be replaced by the new data chunk.
 *
 *-----------------------------------------------------------------------------
 */
static int
ps_gdbm_put(
    ClientData handle,
    const char   *key,
    char     *dataptr,
    size_t        len)
{
    GDBM_FILE dbf = (GDBM_FILE)handle;
    datum drec, dkey;
    int ret;

    dkey.dptr  = (char*)key;
    dkey.dsize = strlen(key) + 1;

    drec.dptr  = dataptr;
    drec.dsize = len;

    ret = gdbm_store(dbf, dkey, drec, GDBM_REPLACE);
    if (ret == -1) {
        return -1;
    }

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_gdbm_delete --
 *
 *      Deletes the key and associated data from the dbm storage.
 *
 * Results:
 *      0 - ok
 *     -1 - error; use ps_dbm_geterr to retrieve the error message
 *
 * Side effects:
 *      If the key is already associated with some user data, this will
 *      be replaced by the new data chunk.
 *
 *-----------------------------------------------------------------------------
 */
static int
ps_gdbm_delete(
    ClientData handle,
    const char   *key)
{
    GDBM_FILE dbf = (GDBM_FILE)handle;
    datum dkey;
    int ret;

    dkey.dptr  = (char*)key;
    dkey.dsize = strlen(key) + 1;

    ret = gdbm_delete(dbf, dkey);
    if (ret == -1) {
        return -1;
    }

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_gdbm_free --
 *
 *      Frees memory allocated by the gdbm implementation.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory gets reclaimed.
 *
 *-----------------------------------------------------------------------------
 */
static void
ps_gdbm_free(
    ClientData handle,
    void        *data)
{
    (void)handle;
    free(data);
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_gdbm_geterr --
 *
 *      Retrieves the textual representation of the error caused
 *      by the last dbm command.
 *
 * Results:
 *      Pointer to the strimg message.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static const char*
ps_gdbm_geterr(
    ClientData handle)
{
   /*
    * The problem with gdbm interface is that it uses the global
    * gdbm_errno variable which is not per-thread nor mutex
    * protected. This variable is used to reference array of gdbm
    * error text strings. It is very dangeours to use this in the
    * MT-program without proper locking. For this kind of app
    * we should not be concerned with that, since all ps_gdbm_xxx
    * operations are performed under shared variable lock anyway.
    */

    return gdbm_strerror(gdbm_errno);
}

#endif  /* HAVE_GDBM */

/* EOF $RCSfile*/

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */
