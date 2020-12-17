/*
 * This file implements wrappers for persistent lmdb storage for the
 * shared variable arrays.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ----------------------------------------------------------------------------
 */

#ifdef HAVE_LMDB

#include "threadSvCmd.h"
#include <lmdb.h>

/*
 * Structure keeping the lmdb environment context
 */
typedef struct {
    MDB_env    * env; // Environment
    MDB_txn    * txn; // Last active read transaction
    MDB_cursor * cur; // Cursor used for ps_lmdb_first and ps_lmdb_next
    MDB_dbi      dbi; // Open database (default db)
    int          err; // Last error (used in ps_lmdb_geterr)
} * LmdbCtx;

/*
 * Transaction and DB open mode
 */
enum LmdbOpenMode { LmdbRead, LmdbWrite };

// Initialize or renew a transaction.
static void LmdbTxnGet(LmdbCtx ctx, enum LmdbOpenMode mode);

// Commit a transaction.
static void LmdbTxnCommit(LmdbCtx ctx);

// Abort a transaction
static void LmdbTxnAbort(LmdbCtx ctx);

void LmdbTxnGet(LmdbCtx ctx, enum LmdbOpenMode mode)
{
    // Read transactions are reused, if possible
    if (ctx->txn && mode == LmdbRead)
    {
        ctx->err = mdb_txn_renew(ctx->txn);
        if (ctx->err)
        {
            ctx->txn = NULL;
        }
    }
    else if (ctx->txn && mode == LmdbWrite)
    {
        LmdbTxnAbort(ctx);
    }

    if (ctx->txn == NULL)
    {
        ctx->err = mdb_txn_begin(ctx->env, NULL, 0, &ctx->txn);
    }

    if (ctx->err)
    {
        ctx->txn = NULL;
        return;
    }

    // Given the setup above, and the arguments given, this won't fail.
    mdb_dbi_open(ctx->txn, NULL, 0, &ctx->dbi);
}

void LmdbTxnCommit(LmdbCtx ctx)
{
    ctx->err = mdb_txn_commit(ctx->txn);
    ctx->txn = NULL;
}

void LmdbTxnAbort(LmdbCtx ctx)
{
    mdb_txn_abort(ctx->txn);
    ctx->txn = NULL;
}

/*
 * Functions implementing the persistent store interface
 */

static ps_open_proc   ps_lmdb_open;
static ps_close_proc  ps_lmdb_close;
static ps_get_proc    ps_lmdb_get;
static ps_put_proc    ps_lmdb_put;
static ps_first_proc  ps_lmdb_first;
static ps_next_proc   ps_lmdb_next;
static ps_delete_proc ps_lmdb_delete;
static ps_free_proc   ps_lmdb_free;
static ps_geterr_proc ps_lmdb_geterr;

/*
 * This structure collects all the various pointers
 * to the functions implementing the lmdb store.
 */

const PsStore LmdbStore = {
    "lmdb",
    NULL,
    ps_lmdb_open,
    ps_lmdb_get,
    ps_lmdb_put,
    ps_lmdb_first,
    ps_lmdb_next,
    ps_lmdb_delete,
    ps_lmdb_close,
    ps_lmdb_free,
    ps_lmdb_geterr,
    NULL
};

/*
 *-----------------------------------------------------------------------------
 *
 * Sv_RegisterLmdbStore --
 *
 *      Register the lmdb store with shared variable implementation.
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
Sv_RegisterLmdbStore(void)
{
    Sv_RegisterPsStore(&LmdbStore);
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_lmdb_open --
 *
 *      Opens the lmdb-based persistent storage.
 *
 * Results:
 *      Opaque handle for LmdbCtx.
 *
 * Side effects:
 *      The lmdb file might be created if not found.
 *
 *-----------------------------------------------------------------------------
 */
static ClientData
ps_lmdb_open(
    const char *path)
{
    LmdbCtx ctx;

    char *ext;
    Tcl_DString toext;

    ctx = ckalloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        return NULL;
    }

    ctx->env = NULL;
    ctx->txn = NULL;
    ctx->cur = NULL;
    ctx->dbi = 0;

    ctx->err = mdb_env_create(&ctx->env);
    if (ctx->err)
    {
        ckfree(ctx);
        return NULL;
    }

    Tcl_DStringInit(&toext);
    ext = Tcl_UtfToExternalDString(NULL, path, strlen(path), &toext);
    ctx->err = mdb_env_open(ctx->env, ext, MDB_NOSUBDIR|MDB_NOLOCK, 0666);
    Tcl_DStringFree(&toext);

    if (ctx->err)
    {
        ckfree(ctx);
        return NULL;
    }

    return (ClientData)ctx;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_lmdb_close --
 *
 *      Closes the lmdb-based persistent storage.
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
ps_lmdb_close(
    ClientData handle)
{
    LmdbCtx ctx = (LmdbCtx)handle;
    if (ctx->cur)
    {
        mdb_cursor_close(ctx->cur);
    }
    if (ctx->txn)
    {
        LmdbTxnAbort(ctx);
    }

    mdb_env_close(ctx->env);
    ckfree(ctx);

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_lmdb_get --
 *
 *      Retrieves data for the key from the lmdb storage.
 *
 * Results:
 *      1 - no such key
 *      0 - ok
 *
 * Side effects:
 *      Data returned must be copied, then psFree must be called.
 *
 *-----------------------------------------------------------------------------
 */
static int
ps_lmdb_get(
     ClientData  handle,
     const char *keyptr,
     char  **dataptrptr,
     size_t     *lenptr)
{
    LmdbCtx ctx = (LmdbCtx)handle;
    MDB_val key, data;

    LmdbTxnGet(ctx, LmdbRead);
    if (ctx->err)
    {
        return 1;
    }

    key.mv_data = (void *)keyptr;
    key.mv_size = strlen(keyptr) + 1;

    ctx->err = mdb_get(ctx->txn, ctx->dbi, &key, &data);
    if (ctx->err)
    {
        mdb_txn_reset(ctx->txn);
        return 1;
    }

    *dataptrptr = data.mv_data;
    *lenptr = data.mv_size;

    /*
     * Transaction is left open at this point, so that the caller can get ahold
     * of the data and make a copy of it. Afterwards, it will call ps_lmdb_free
     * to free the data, and we'll catch the chance to reset the transaction
     * there.
     */

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_lmdb_first --
 *
 *      Starts the iterator over the lmdb file and returns the first record.
 *
 * Results:
 *      1 - no more records in the iterator
 *      0 - ok
 *
 * Side effects:
 *      Data returned must be copied, then psFree must be called.
 *
 *-----------------------------------------------------------------------------
 */
static int
ps_lmdb_first(
    ClientData  handle,
    char   **keyptrptr,
    char  **dataptrptr,
    size_t     *lenptr)
{
    LmdbCtx ctx = (LmdbCtx)handle;
    MDB_val key, data;

    LmdbTxnGet(ctx, LmdbRead);
    if (ctx->err)
    {
        return 1;
    }

    ctx->err = mdb_cursor_open(ctx->txn, ctx->dbi, &ctx->cur);
    if (ctx->err)
    {
        return 1;
    }

    ctx->err = mdb_cursor_get(ctx->cur, &key, &data, MDB_FIRST);
    if (ctx->err)
    {
        mdb_txn_reset(ctx->txn);
        mdb_cursor_close(ctx->cur);
        ctx->cur = NULL;
        return 1;
    }

    *dataptrptr = data.mv_data;
    *lenptr = data.mv_size;
    *keyptrptr = key.mv_data;

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_lmdb_next --
 *
 *      Uses the iterator over the lmdb file and returns the next record.
 *
 * Results:
 *      1 - no more records in the iterator
 *      0 - ok
 *
 * Side effects:
 *      Data returned must be copied, then psFree must be called.
 *
 *-----------------------------------------------------------------------------
 */
static int ps_lmdb_next(
    ClientData  handle,
    char   **keyptrptr,
    char  **dataptrptr,
    size_t     *lenptr)
{
    LmdbCtx ctx = (LmdbCtx)handle;
    MDB_val key, data;

    ctx->err = mdb_cursor_get(ctx->cur, &key, &data, MDB_NEXT);
    if (ctx->err)
    {
        mdb_txn_reset(ctx->txn);
        mdb_cursor_close(ctx->cur);
        ctx->cur = NULL;
        return 1;
    }

    *dataptrptr = data.mv_data;
    *lenptr = data.mv_size;
    *keyptrptr = key.mv_data;

    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_lmdb_put --
 *
 *      Stores used data bound to a key in lmdb storage.
 *
 * Results:
 *      0 - ok
 *     -1 - error; use ps_lmdb_geterr to retrieve the error message
 *
 * Side effects:
 *      If the key is already associated with some user data, this will
 *      be replaced by the new data chunk.
 *
 *-----------------------------------------------------------------------------
 */
static int
ps_lmdb_put(
    ClientData  handle,
    const char *keyptr,
    char      *dataptr,
    size_t         len)
{
    LmdbCtx ctx = (LmdbCtx)handle;
    MDB_val key, data;

    LmdbTxnGet(ctx, LmdbWrite);
    if (ctx->err)
    {
        return -1;
    }

    key.mv_data = (void*)keyptr;
    key.mv_size = strlen(keyptr) + 1;

    data.mv_data = dataptr;
    data.mv_size = len;

    ctx->err = mdb_put(ctx->txn, ctx->dbi, &key, &data, 0);
    if (ctx->err)
    {
        LmdbTxnAbort(ctx);
    }
    else
    {
        LmdbTxnCommit(ctx);
    }

    return ctx->err ? -1 : 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_lmdb_delete --
 *
 *      Deletes the key and associated data from the lmdb storage.
 *
 * Results:
 *      0 - ok
 *     -1 - error; use ps_lmdb_geterr to retrieve the error message
 *
 * Side effects:
 *      If the key is already associated with some user data, this will
 *      be replaced by the new data chunk.
 *
 *-----------------------------------------------------------------------------
 */
static int
ps_lmdb_delete(
    ClientData  handle,
    const char *keyptr)
{
    LmdbCtx ctx = (LmdbCtx)handle;
    MDB_val key;

    LmdbTxnGet(ctx, LmdbWrite);
    if (ctx->err)
    {
        return -1;
    }

    key.mv_data = (void*)keyptr;
    key.mv_size = strlen(keyptr) + 1;

    ctx->err = mdb_del(ctx->txn, ctx->dbi, &key, NULL);
    if (ctx->err)
    {
        LmdbTxnAbort(ctx);
    }
    else
    {
        LmdbTxnCommit(ctx);
    }

    ctx->txn = NULL;
    return ctx->err ? -1 : 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_lmdb_free --
 *
 *      This function is called to free data returned by the persistent store
 *      after calls to psFirst, psNext, or psGet. Lmdb doesn't need to free any
 *      data, as the data returned is owned by lmdb. On the other hand, this
 *      method is required to reset the read transaction. This is done only
 *      when iteration is over (ctx->cur == NULL).
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
ps_lmdb_free(
    ClientData handle,
    void        *data)
{
    LmdbCtx ctx = (LmdbCtx)handle;
    (void)data;

    if (ctx->cur == NULL)
    {
        mdb_txn_reset(ctx->txn);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * ps_lmdb_geterr --
 *
 *      Retrieves the textual representation of the error caused
 *      by the last lmdb command.
 *
 * Results:
 *      Pointer to the string message.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static const char*
ps_lmdb_geterr(
    ClientData handle)
{
    LmdbCtx ctx = (LmdbCtx)handle;
    return mdb_strerror(ctx->err);
}

#endif  /* HAVE_LMDB */

/* EOF $RCSfile*/

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */
