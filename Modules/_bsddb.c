/*----------------------------------------------------------------------
  Copyright (c) 1999-2001, Digital Creations, Fredericksburg, VA, USA
  and Andrew Kuchling. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    o Redistributions of source code must retain the above copyright
      notice, this list of conditions, and the disclaimer that follows.

    o Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions, and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

    o Neither the name of Digital Creations nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY DIGITAL CREATIONS AND CONTRIBUTORS *AS
  IS* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL DIGITAL
  CREATIONS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
  DAMAGE.
------------------------------------------------------------------------*/


/*
 * Handwritten code to wrap version 3.x of the Berkeley DB library,
 * written to replace a SWIG-generated file.  It has since been updated
 * to compile with Berkeley DB versions 3.2 through 4.2.
 *
 * This module was started by Andrew Kuchling to remove the dependency
 * on SWIG in a package by Gregory P. Smith who based his work on a
 * similar package by Robin Dunn <robin@alldunn.com> which wrapped
 * Berkeley DB 2.7.x.
 *
 * Development of this module then returned full circle back to Robin Dunn
 * who worked on behalf of Digital Creations to complete the wrapping of
 * the DB 3.x API and to build a solid unit test suite.  Robin has
 * since gone onto other projects (wxPython).
 *
 * Gregory P. Smith <greg@krypto.org> was once again the maintainer.
 *
 * Since January 2008, new maintainer is Jesus Cea <jcea@jcea.es>.
 * Jesus Cea licenses this code to PSF under a Contributor Agreement.
 *
 * Use the pybsddb-users@lists.sf.net mailing list for all questions.
 * Things can change faster than the header of this file is updated.  This
 * file is shared with the PyBSDDB project at SourceForge:
 *
 * http://pybsddb.sf.net
 *
 * This file should remain backward compatible with Python 2.1, but see PEP
 * 291 for the most current backward compatibility requirements:
 *
 * http://www.python.org/peps/pep-0291.html
 *
 * This module contains 7 types:
 *
 * DB           (Database)
 * DBCursor     (Database Cursor)
 * DBEnv        (database environment)
 * DBTxn        (An explicit database transaction)
 * DBLock       (A lock handle)
 * DBSequence   (Sequence)
 * DBSite       (Site)
 *
 * More datatypes added:
 *
 * DBLogCursor  (Log Cursor)
 *
 */

/* --------------------------------------------------------------------- */

/*
 * Portions of this module, associated unit tests and build scripts are the
 * result of a contract with The Written Word (http://thewrittenword.com/)
 * Many thanks go out to them for causing me to raise the bar on quality and
 * functionality, resulting in a better bsddb3 package for all of us to use.
 *
 * --Robin
 */

/* --------------------------------------------------------------------- */

#include <stddef.h>   /* for offsetof() */
#include <Python.h>

#define COMPILING_BSDDB_C
#include "bsddb.h"
#undef COMPILING_BSDDB_C

static char *rcs_id = "$Id$";

/* --------------------------------------------------------------------- */
/* Various macro definitions */

#if (PY_VERSION_HEX < 0x02050000)
typedef int Py_ssize_t;
#endif

#if (PY_VERSION_HEX < 0x02060000)  /* really: before python trunk r63675 */
/* This code now uses PyBytes* API function names instead of PyString*.
 * These #defines map to their equivalent on earlier python versions.    */
#define PyBytes_FromStringAndSize PyString_FromStringAndSize
#define PyBytes_FromString PyString_FromString
#define PyBytes_AsStringAndSize PyString_AsStringAndSize
#define PyBytes_Check PyString_Check
#define PyBytes_GET_SIZE PyString_GET_SIZE
#define PyBytes_AS_STRING PyString_AS_STRING
#endif

#if (PY_VERSION_HEX >= 0x03000000)
#define NUMBER_Check    PyLong_Check
#define NUMBER_AsLong   PyLong_AsLong
#define NUMBER_FromLong PyLong_FromLong
#else
#define NUMBER_Check    PyInt_Check
#define NUMBER_AsLong   PyInt_AsLong
#define NUMBER_FromLong PyInt_FromLong
#endif

#ifdef WITH_THREAD

/* These are for when calling Python --> C */
#define MYDB_BEGIN_ALLOW_THREADS Py_BEGIN_ALLOW_THREADS;
#define MYDB_END_ALLOW_THREADS Py_END_ALLOW_THREADS;

/* and these are for calling C --> Python */
#define MYDB_BEGIN_BLOCK_THREADS \
                PyGILState_STATE __savestate = PyGILState_Ensure();
#define MYDB_END_BLOCK_THREADS \
                PyGILState_Release(__savestate);

#else
/* Compiled without threads - avoid all this cruft */
#define MYDB_BEGIN_ALLOW_THREADS
#define MYDB_END_ALLOW_THREADS
#define MYDB_BEGIN_BLOCK_THREADS
#define MYDB_END_BLOCK_THREADS

#endif

/* --------------------------------------------------------------------- */
/* Exceptions */

static PyObject* DBError;               /* Base class, all others derive from this */
static PyObject* DBCursorClosedError;   /* raised when trying to use a closed cursor object */
static PyObject* DBKeyEmptyError;       /* DB_KEYEMPTY: also derives from KeyError */
static PyObject* DBKeyExistError;       /* DB_KEYEXIST */
static PyObject* DBLockDeadlockError;   /* DB_LOCK_DEADLOCK */
static PyObject* DBLockNotGrantedError; /* DB_LOCK_NOTGRANTED */
static PyObject* DBNotFoundError;       /* DB_NOTFOUND: also derives from KeyError */
static PyObject* DBOldVersionError;     /* DB_OLD_VERSION */
static PyObject* DBRunRecoveryError;    /* DB_RUNRECOVERY */
static PyObject* DBVerifyBadError;      /* DB_VERIFY_BAD */
static PyObject* DBNoServerError;       /* DB_NOSERVER */
#if (DBVER < 52)
static PyObject* DBNoServerHomeError;   /* DB_NOSERVER_HOME */
static PyObject* DBNoServerIDError;     /* DB_NOSERVER_ID */
#endif
static PyObject* DBPageNotFoundError;   /* DB_PAGE_NOTFOUND */
static PyObject* DBSecondaryBadError;   /* DB_SECONDARY_BAD */

static PyObject* DBInvalidArgError;     /* EINVAL */
static PyObject* DBAccessError;         /* EACCES */
static PyObject* DBNoSpaceError;        /* ENOSPC */
static PyObject* DBNoMemoryError;       /* DB_BUFFER_SMALL */
static PyObject* DBAgainError;          /* EAGAIN */
static PyObject* DBBusyError;           /* EBUSY  */
static PyObject* DBFileExistsError;     /* EEXIST */
static PyObject* DBNoSuchFileError;     /* ENOENT */
static PyObject* DBPermissionsError;    /* EPERM  */

static PyObject* DBRepHandleDeadError;  /* DB_REP_HANDLE_DEAD */
#if (DBVER >= 44)
static PyObject* DBRepLockoutError;     /* DB_REP_LOCKOUT */
#endif

#if (DBVER >= 46)
static PyObject* DBRepLeaseExpiredError; /* DB_REP_LEASE_EXPIRED */
#endif

#if (DBVER >= 47)
static PyObject* DBForeignConflictError; /* DB_FOREIGN_CONFLICT */
#endif


static PyObject* DBRepUnavailError;     /* DB_REP_UNAVAIL */

#if (DBVER < 48)
#define DB_GID_SIZE DB_XIDDATASIZE
#endif


/* --------------------------------------------------------------------- */
/* Structure definitions */

#if PYTHON_API_VERSION < 1010
#error "Python 2.1 or later required"
#endif


/* Defaults for moduleFlags in DBEnvObject and DBObject. */
#define DEFAULT_GET_RETURNS_NONE                1
#define DEFAULT_CURSOR_SET_RETURNS_NONE         1   /* 0 in pybsddb < 4.2, python < 2.4 */


/* See comment in Python 2.6 "object.h" */
#ifndef staticforward
#define staticforward static
#endif
#ifndef statichere
#define statichere static
#endif

staticforward PyTypeObject DB_Type, DBCursor_Type, DBEnv_Type, DBTxn_Type,
              DBLock_Type, DBLogCursor_Type;
staticforward PyTypeObject DBSequence_Type;
#if (DBVER >= 52)
staticforward PyTypeObject DBSite_Type;
#endif

#ifndef Py_TYPE
/* for compatibility with Python 2.5 and earlier */
#define Py_TYPE(ob)              (((PyObject*)(ob))->ob_type)
#endif

#define DBObject_Check(v)           (Py_TYPE(v) == &DB_Type)
#define DBCursorObject_Check(v)     (Py_TYPE(v) == &DBCursor_Type)
#define DBLogCursorObject_Check(v)  (Py_TYPE(v) == &DBLogCursor_Type)
#define DBEnvObject_Check(v)        (Py_TYPE(v) == &DBEnv_Type)
#define DBTxnObject_Check(v)        (Py_TYPE(v) == &DBTxn_Type)
#define DBLockObject_Check(v)       (Py_TYPE(v) == &DBLock_Type)
#define DBSequenceObject_Check(v)   (Py_TYPE(v) == &DBSequence_Type)
#if (DBVER >= 52)
#define DBSiteObject_Check(v)       (Py_TYPE(v) == &DBSite_Type)
#endif

#if (DBVER < 46)
  #define _DBC_close(dbc)           dbc->c_close(dbc)
  #define _DBC_count(dbc,a,b)       dbc->c_count(dbc,a,b)
  #define _DBC_del(dbc,a)           dbc->c_del(dbc,a)
  #define _DBC_dup(dbc,a,b)         dbc->c_dup(dbc,a,b)
  #define _DBC_get(dbc,a,b,c)       dbc->c_get(dbc,a,b,c)
  #define _DBC_pget(dbc,a,b,c,d)    dbc->c_pget(dbc,a,b,c,d)
  #define _DBC_put(dbc,a,b,c)       dbc->c_put(dbc,a,b,c)
#else
  #define _DBC_close(dbc)           dbc->close(dbc)
  #define _DBC_count(dbc,a,b)       dbc->count(dbc,a,b)
  #define _DBC_del(dbc,a)           dbc->del(dbc,a)
  #define _DBC_dup(dbc,a,b)         dbc->dup(dbc,a,b)
  #define _DBC_get(dbc,a,b,c)       dbc->get(dbc,a,b,c)
  #define _DBC_pget(dbc,a,b,c,d)    dbc->pget(dbc,a,b,c,d)
  #define _DBC_put(dbc,a,b,c)       dbc->put(dbc,a,b,c)
#endif


/* --------------------------------------------------------------------- */
/* Utility macros and functions */

#define INSERT_IN_DOUBLE_LINKED_LIST(backlink,object)                   \
    {                                                                   \
        object->sibling_next=backlink;                                  \
        object->sibling_prev_p=&(backlink);                             \
        backlink=object;                                                \
        if (object->sibling_next) {                                     \
          object->sibling_next->sibling_prev_p=&(object->sibling_next); \
        }                                                               \
    }

#define EXTRACT_FROM_DOUBLE_LINKED_LIST(object)                          \
    {                                                                    \
        if (object->sibling_next) {                                      \
            object->sibling_next->sibling_prev_p=object->sibling_prev_p; \
        }                                                                \
        *(object->sibling_prev_p)=object->sibling_next;                  \
    }

#define EXTRACT_FROM_DOUBLE_LINKED_LIST_MAYBE_NULL(object)               \
    {                                                                    \
        if (object->sibling_next) {                                      \
            object->sibling_next->sibling_prev_p=object->sibling_prev_p; \
        }                                                                \
        if (object->sibling_prev_p) {                                    \
            *(object->sibling_prev_p)=object->sibling_next;              \
        }                                                                \
    }

#define INSERT_IN_DOUBLE_LINKED_LIST_TXN(backlink,object)  \
    {                                                      \
        object->sibling_next_txn=backlink;                 \
        object->sibling_prev_p_txn=&(backlink);            \
        backlink=object;                                   \
        if (object->sibling_next_txn) {                    \
            object->sibling_next_txn->sibling_prev_p_txn=  \
                &(object->sibling_next_txn);               \
        }                                                  \
    }

#define EXTRACT_FROM_DOUBLE_LINKED_LIST_TXN(object)             \
    {                                                           \
        if (object->sibling_next_txn) {                         \
            object->sibling_next_txn->sibling_prev_p_txn=       \
                object->sibling_prev_p_txn;                     \
        }                                                       \
        *(object->sibling_prev_p_txn)=object->sibling_next_txn; \
    }


#define RETURN_IF_ERR()          \
    if (makeDBError(err)) {      \
        return NULL;             \
    }

#define RETURN_NONE()  Py_INCREF(Py_None); return Py_None;

#define _CHECK_OBJECT_NOT_CLOSED(nonNull, pyErrObj, name) \
    if ((nonNull) == NULL) {          \
        PyObject *errTuple = NULL;    \
        errTuple = Py_BuildValue("(is)", 0, #name " object has been closed"); \
        if (errTuple) { \
            PyErr_SetObject((pyErrObj), errTuple);  \
            Py_DECREF(errTuple);          \
        } \
        return NULL;                  \
    }

#define CHECK_DB_NOT_CLOSED(dbobj) \
        _CHECK_OBJECT_NOT_CLOSED(dbobj->db, DBError, DB)

#define CHECK_ENV_NOT_CLOSED(env) \
        _CHECK_OBJECT_NOT_CLOSED(env->db_env, DBError, DBEnv)

#define CHECK_CURSOR_NOT_CLOSED(curs) \
        _CHECK_OBJECT_NOT_CLOSED(curs->dbc, DBCursorClosedError, DBCursor)

#define CHECK_LOGCURSOR_NOT_CLOSED(logcurs) \
        _CHECK_OBJECT_NOT_CLOSED(logcurs->logc, DBCursorClosedError, DBLogCursor)

#define CHECK_SEQUENCE_NOT_CLOSED(curs) \
        _CHECK_OBJECT_NOT_CLOSED(curs->sequence, DBError, DBSequence)

#if (DBVER >= 52)
#define CHECK_SITE_NOT_CLOSED(db_site) \
         _CHECK_OBJECT_NOT_CLOSED(db_site->site, DBError, DBSite)
#endif

#define CHECK_DBFLAG(mydb, flag)    (((mydb)->flags & (flag)) || \
                                     (((mydb)->myenvobj != NULL) && ((mydb)->myenvobj->flags & (flag))))

#define CLEAR_DBT(dbt)              (memset(&(dbt), 0, sizeof(dbt)))

#define FREE_DBT(dbt)               if ((dbt.flags & (DB_DBT_MALLOC|DB_DBT_REALLOC)) && \
                                         dbt.data != NULL) { free(dbt.data); dbt.data = NULL; }


static int makeDBError(int err);


/* Return the access method type of the DBObject */
static int _DB_get_type(DBObject* self)
{
    DBTYPE type;
    int err;

    err = self->db->get_type(self->db, &type);
    if (makeDBError(err)) {
        return -1;
    }
    return type;
}


/* Create a DBT structure (containing key and data values) from Python
   strings.  Returns 1 on success, 0 on an error. */
static int make_dbt(PyObject* obj, DBT* dbt)
{
    CLEAR_DBT(*dbt);
    if (obj == Py_None) {
        /* no need to do anything, the structure has already been zeroed */
    }
    else if (!PyArg_Parse(obj, "s#", &dbt->data, &dbt->size)) {
        PyErr_SetString(PyExc_TypeError,
#if (PY_VERSION_HEX < 0x03000000)
                        "Data values must be of type string or None.");
#else
                        "Data values must be of type bytes or None.");
#endif
        return 0;
    }
    return 1;
}


/* Recno and Queue DBs can have integer keys.  This function figures out
   what's been given, verifies that it's allowed, and then makes the DBT.

   Caller MUST call FREE_DBT(key) when done. */
static int
make_key_dbt(DBObject* self, PyObject* keyobj, DBT* key, int* pflags)
{
    db_recno_t recno;
    int type;

    CLEAR_DBT(*key);
    if (keyobj == Py_None) {
        type = _DB_get_type(self);
        if (type == -1)
            return 0;
        if (type == DB_RECNO || type == DB_QUEUE) {
            PyErr_SetString(
                PyExc_TypeError,
                "None keys not allowed for Recno and Queue DB's");
            return 0;
        }
        /* no need to do anything, the structure has already been zeroed */
    }

    else if (PyBytes_Check(keyobj)) {
        /* verify access method type */
        type = _DB_get_type(self);
        if (type == -1)
            return 0;
        if (type == DB_RECNO || type == DB_QUEUE) {
            PyErr_SetString(
                PyExc_TypeError,
#if (PY_VERSION_HEX < 0x03000000)
                "String keys not allowed for Recno and Queue DB's");
#else
                "Bytes keys not allowed for Recno and Queue DB's");
#endif
            return 0;
        }

        /*
         * NOTE(gps): I don't like doing a data copy here, it seems
         * wasteful.  But without a clean way to tell FREE_DBT if it
         * should free key->data or not we have to.  Other places in
         * the code check for DB_THREAD and forceably set DBT_MALLOC
         * when we otherwise would leave flags 0 to indicate that.
         */
        key->data = malloc(PyBytes_GET_SIZE(keyobj));
        if (key->data == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Key memory allocation failed");
            return 0;
        }
        memcpy(key->data, PyBytes_AS_STRING(keyobj),
               PyBytes_GET_SIZE(keyobj));
        key->flags = DB_DBT_REALLOC;
        key->size = PyBytes_GET_SIZE(keyobj);
    }

    else if (NUMBER_Check(keyobj)) {
        /* verify access method type */
        type = _DB_get_type(self);
        if (type == -1)
            return 0;
        if (type == DB_BTREE && pflags != NULL) {
            /* if BTREE then an Integer key is allowed with the
             * DB_SET_RECNO flag */
            *pflags |= DB_SET_RECNO;
        }
        else if (type != DB_RECNO && type != DB_QUEUE) {
            PyErr_SetString(
                PyExc_TypeError,
                "Integer keys only allowed for Recno and Queue DB's");
            return 0;
        }

        /* Make a key out of the requested recno, use allocated space so DB
         * will be able to realloc room for the real key if needed. */
        recno = NUMBER_AsLong(keyobj);
        key->data = malloc(sizeof(db_recno_t));
        if (key->data == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Key memory allocation failed");
            return 0;
        }
        key->ulen = key->size = sizeof(db_recno_t);
        memcpy(key->data, &recno, sizeof(db_recno_t));
        key->flags = DB_DBT_REALLOC;
    }
    else {
        PyErr_Format(PyExc_TypeError,
#if (PY_VERSION_HEX < 0x03000000)
                     "String or Integer object expected for key, %s found",
#else
                     "Bytes or Integer object expected for key, %s found",
#endif
                     Py_TYPE(keyobj)->tp_name);
        return 0;
    }

    return 1;
}


/* Add partial record access to an existing DBT data struct.
   If dlen and doff are set, then the DB_DBT_PARTIAL flag will be set
   and the data storage/retrieval will be done using dlen and doff. */
static int add_partial_dbt(DBT* d, int dlen, int doff) {
    /* if neither were set we do nothing (-1 is the default value) */
    if ((dlen == -1) && (doff == -1)) {
        return 1;
    }

    if ((dlen < 0) || (doff < 0)) {
        PyErr_SetString(PyExc_TypeError, "dlen and doff must both be >= 0");
        return 0;
    }

    d->flags = d->flags | DB_DBT_PARTIAL;
    d->dlen = (unsigned int) dlen;
    d->doff = (unsigned int) doff;
    return 1;
}

/* a safe strcpy() without the zeroing behaviour and semantics of strncpy. */
/* TODO: make this use the native libc strlcpy() when available (BSD)      */
unsigned int our_strlcpy(char* dest, const char* src, unsigned int n)
{
    unsigned int srclen, copylen;

    srclen = strlen(src);
    if (n <= 0)
        return srclen;
    copylen = (srclen > n-1) ? n-1 : srclen;
    /* populate dest[0] thru dest[copylen-1] */
    memcpy(dest, src, copylen);
    /* guarantee null termination */
    dest[copylen] = 0;

    return srclen;
}

/* Callback used to save away more information about errors from the DB
 * library. */
static char _db_errmsg[1024];
static void _db_errorCallback(const DB_ENV *db_env,
        const char* prefix, const char* msg)
{
    our_strlcpy(_db_errmsg, msg, sizeof(_db_errmsg));
}


/*
** We need these functions because some results
** are undefined if pointer is NULL. Some other
** give None instead of "".
**
** This functions are static and will be
** -I hope- inlined.
*/
static const char *DummyString = "This string is a simple placeholder";
static PyObject *Build_PyString(const char *p,int s)
{
  if (!p) {
    p=DummyString;
    assert(s==0);
  }
  return PyBytes_FromStringAndSize(p,s);
}

static PyObject *BuildValue_S(const void *p,int s)
{
  if (!p) {
    p=DummyString;
    assert(s==0);
  }
  return PyBytes_FromStringAndSize(p, s);
}

static PyObject *BuildValue_SS(const void *p1,int s1,const void *p2,int s2)
{
PyObject *a, *b, *r;

  if (!p1) {
    p1=DummyString;
    assert(s1==0);
  }
  if (!p2) {
    p2=DummyString;
    assert(s2==0);
  }

  if (!(a = PyBytes_FromStringAndSize(p1, s1))) {
      return NULL;
  }
  if (!(b = PyBytes_FromStringAndSize(p2, s2))) {
      Py_DECREF(a);
      return NULL;
  }

  r = PyTuple_Pack(2, a, b) ;
  Py_DECREF(a);
  Py_DECREF(b);
  return r;
}

static PyObject *BuildValue_IS(int i,const void *p,int s)
{
  PyObject *a, *r;

  if (!p) {
    p=DummyString;
    assert(s==0);
  }

  if (!(a = PyBytes_FromStringAndSize(p, s))) {
      return NULL;
  }

  r = Py_BuildValue("iO", i, a);
  Py_DECREF(a);
  return r;
}

static PyObject *BuildValue_LS(long l,const void *p,int s)
{
  PyObject *a, *r;

  if (!p) {
    p=DummyString;
    assert(s==0);
  }

  if (!(a = PyBytes_FromStringAndSize(p, s))) {
      return NULL;
  }

  r = Py_BuildValue("lO", l, a);
  Py_DECREF(a);
  return r;
}



/* make a nice exception object to raise for errors. */
static int makeDBError(int err)
{
    char errTxt[2048];  /* really big, just in case... */
    PyObject *errObj = NULL;
    PyObject *errTuple = NULL;
    int exceptionRaised = 0;
    unsigned int bytes_left;

    switch (err) {
        case 0:                     /* successful, no error */
            return 0;

        case DB_KEYEMPTY:           errObj = DBKeyEmptyError;       break;
        case DB_KEYEXIST:           errObj = DBKeyExistError;       break;
        case DB_LOCK_DEADLOCK:      errObj = DBLockDeadlockError;   break;
        case DB_LOCK_NOTGRANTED:    errObj = DBLockNotGrantedError; break;
        case DB_NOTFOUND:           errObj = DBNotFoundError;       break;
        case DB_OLD_VERSION:        errObj = DBOldVersionError;     break;
        case DB_RUNRECOVERY:        errObj = DBRunRecoveryError;    break;
        case DB_VERIFY_BAD:         errObj = DBVerifyBadError;      break;
        case DB_NOSERVER:           errObj = DBNoServerError;       break;
#if (DBVER < 52)
        case DB_NOSERVER_HOME:      errObj = DBNoServerHomeError;   break;
        case DB_NOSERVER_ID:        errObj = DBNoServerIDError;     break;
#endif
        case DB_PAGE_NOTFOUND:      errObj = DBPageNotFoundError;   break;
        case DB_SECONDARY_BAD:      errObj = DBSecondaryBadError;   break;
        case DB_BUFFER_SMALL:       errObj = DBNoMemoryError;       break;

        case ENOMEM:  errObj = PyExc_MemoryError;   break;
        case EINVAL:  errObj = DBInvalidArgError;   break;
        case EACCES:  errObj = DBAccessError;       break;
        case ENOSPC:  errObj = DBNoSpaceError;      break;
        case EAGAIN:  errObj = DBAgainError;        break;
        case EBUSY :  errObj = DBBusyError;         break;
        case EEXIST:  errObj = DBFileExistsError;   break;
        case ENOENT:  errObj = DBNoSuchFileError;   break;
        case EPERM :  errObj = DBPermissionsError;  break;

        case DB_REP_HANDLE_DEAD : errObj = DBRepHandleDeadError; break;
#if (DBVER >= 44)
        case DB_REP_LOCKOUT : errObj = DBRepLockoutError; break;
#endif

#if (DBVER >= 46)
        case DB_REP_LEASE_EXPIRED : errObj = DBRepLeaseExpiredError; break;
#endif

#if (DBVER >= 47)
        case DB_FOREIGN_CONFLICT : errObj = DBForeignConflictError; break;
#endif

        case DB_REP_UNAVAIL : errObj = DBRepUnavailError; break;

        default:      errObj = DBError;             break;
    }

    if (errObj != NULL) {
        bytes_left = our_strlcpy(errTxt, db_strerror(err), sizeof(errTxt));
        /* Ensure that bytes_left never goes negative */
        if (_db_errmsg[0] && bytes_left < (sizeof(errTxt) - 4)) {
            bytes_left = sizeof(errTxt) - bytes_left - 4 - 1;
            assert(bytes_left >= 0);
            strcat(errTxt, " -- ");
            strncat(errTxt, _db_errmsg, bytes_left);
        }
        _db_errmsg[0] = 0;

        errTuple = Py_BuildValue("(is)", err, errTxt);
        if (errTuple == NULL) {
            Py_DECREF(errObj);
            return !0;
        }
        PyErr_SetObject(errObj, errTuple);
        Py_DECREF(errTuple);
    }

    return ((errObj != NULL) || exceptionRaised);
}



/* set a type exception */
static void makeTypeError(char* expected, PyObject* found)
{
    PyErr_Format(PyExc_TypeError, "Expected %s argument, %s found.",
                 expected, Py_TYPE(found)->tp_name);
}


/* verify that an obj is either None or a DBTxn, and set the txn pointer */
static int checkTxnObj(PyObject* txnobj, DB_TXN** txn)
{
    if (txnobj == Py_None || txnobj == NULL) {
        *txn = NULL;
        return 1;
    }
    if (DBTxnObject_Check(txnobj)) {
        *txn = ((DBTxnObject*)txnobj)->txn;
        return 1;
    }
    else
        makeTypeError("DBTxn", txnobj);
    return 0;
}


/* Delete a key from a database
  Returns 0 on success, -1 on an error.  */
static int _DB_delete(DBObject* self, DB_TXN *txn, DBT *key, int flags)
{
    int err;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->del(self->db, txn, key, 0);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        return -1;
    }
    return 0;
}


/* Store a key into a database
   Returns 0 on success, -1 on an error.  */
static int _DB_put(DBObject* self, DB_TXN *txn, DBT *key, DBT *data, int flags)
{
    int err;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->put(self->db, txn, key, data, flags);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        return -1;
    }
    return 0;
}

/* Get a key/data pair from a cursor */
static PyObject* _DBCursor_get(DBCursorObject* self, int extra_flags,
                               PyObject *args, PyObject *kwargs, char *format)
{
    int err;
    PyObject* retval = NULL;
    DBT key, data;
    int dlen = -1;
    int doff = -1;
    int flags = 0;
    static char* kwnames[] = { "flags", "dlen", "doff", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, format, kwnames,
                                     &flags, &dlen, &doff))
      return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    flags |= extra_flags;
    CLEAR_DBT(key);
    CLEAR_DBT(data);
    if (!add_partial_dbt(&data, dlen, doff))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_get(self->dbc, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
            && self->mydb->moduleFlags.getReturnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {  /* otherwise, success! */

        /* if Recno or Queue, return the key as an Int */
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;

        case DB_RECNO:
        case DB_QUEUE:
            retval = BuildValue_IS(*((db_recno_t*)key.data), data.data, data.size);
            break;
        case DB_HASH:
        case DB_BTREE:
        default:
            retval = BuildValue_SS(key.data, key.size, data.data, data.size);
            break;
        }
    }
    return retval;
}


/* add an integer to a dictionary using the given name as a key */
static void _addIntToDict(PyObject* dict, char *name, int value)
{
    PyObject* v = NUMBER_FromLong((long) value);
    if (!v || PyDict_SetItemString(dict, name, v))
        PyErr_Clear();

    Py_XDECREF(v);
}

/* The same, when the value is a time_t */
static void _addTimeTToDict(PyObject* dict, char *name, time_t value)
{
    PyObject* v;
        /* if the value fits in regular int, use that. */
#ifdef PY_LONG_LONG
        if (sizeof(time_t) > sizeof(long))
                v = PyLong_FromLongLong((PY_LONG_LONG) value);
        else
#endif
                v = NUMBER_FromLong((long) value);
    if (!v || PyDict_SetItemString(dict, name, v))
        PyErr_Clear();

    Py_XDECREF(v);
}

/* add an db_seq_t to a dictionary using the given name as a key */
static void _addDb_seq_tToDict(PyObject* dict, char *name, db_seq_t value)
{
    PyObject* v = PyLong_FromLongLong(value);
    if (!v || PyDict_SetItemString(dict, name, v))
        PyErr_Clear();

    Py_XDECREF(v);
}

static void _addDB_lsnToDict(PyObject* dict, char *name, DB_LSN value)
{
    PyObject *v = Py_BuildValue("(ll)",value.file,value.offset);
    if (!v || PyDict_SetItemString(dict, name, v))
        PyErr_Clear();

    Py_XDECREF(v);
}

/* --------------------------------------------------------------------- */
/* Allocators and deallocators */

static DBObject*
newDBObject(DBEnvObject* arg, int flags)
{
    DBObject* self;
    DB_ENV* db_env = NULL;
    int err;

    self = PyObject_New(DBObject, &DB_Type);
    if (self == NULL)
        return NULL;

    self->flags = 0;
    self->setflags = 0;
    self->myenvobj = NULL;
    self->db = NULL;
    self->children_cursors = NULL;
    self->children_sequences = NULL;
    self->associateCallback = NULL;
    self->btCompareCallback = NULL;
    self->dupCompareCallback = NULL;
    self->primaryDBType = 0;
    Py_INCREF(Py_None);
    self->private_obj = Py_None;
    self->in_weakreflist = NULL;

    /* keep a reference to our python DBEnv object */
    if (arg) {
        Py_INCREF(arg);
        self->myenvobj = arg;
        db_env = arg->db_env;
        INSERT_IN_DOUBLE_LINKED_LIST(self->myenvobj->children_dbs,self);
    } else {
      self->sibling_prev_p=NULL;
      self->sibling_next=NULL;
    }
    self->txn=NULL;
    self->sibling_prev_p_txn=NULL;
    self->sibling_next_txn=NULL;

    if (self->myenvobj)
        self->moduleFlags = self->myenvobj->moduleFlags;
    else
        self->moduleFlags.getReturnsNone = DEFAULT_GET_RETURNS_NONE;
        self->moduleFlags.cursorSetReturnsNone = DEFAULT_CURSOR_SET_RETURNS_NONE;

    MYDB_BEGIN_ALLOW_THREADS;
    err = db_create(&self->db, db_env, flags);
    if (self->db != NULL) {
        self->db->set_errcall(self->db, _db_errorCallback);
        self->db->app_private = (void*)self;
    }
    MYDB_END_ALLOW_THREADS;
    /* TODO add a weakref(self) to the self->myenvobj->open_child_weakrefs
     * list so that a DBEnv can refuse to close without aborting any open
     * DBTxns and closing any open DBs first. */
    if (makeDBError(err)) {
        if (self->myenvobj) {
            Py_CLEAR(self->myenvobj);
        }
        Py_DECREF(self);
        self = NULL;
    }
    return self;
}


/* Forward declaration */
static PyObject *DB_close_internal(DBObject* self, int flags, int do_not_close);

static void
DB_dealloc(DBObject* self)
{
  PyObject *dummy;

    if (self->db != NULL) {
        dummy=DB_close_internal(self, 0, 0);
        /*
        ** Raising exceptions while doing
        ** garbage collection is a fatal error.
        */
        if (dummy)
            Py_DECREF(dummy);
        else
            PyErr_Clear();
    }
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }
    if (self->myenvobj) {
        Py_CLEAR(self->myenvobj);
    }
    if (self->associateCallback != NULL) {
        Py_CLEAR(self->associateCallback);
    }
    if (self->btCompareCallback != NULL) {
        Py_CLEAR(self->btCompareCallback);
    }
    if (self->dupCompareCallback != NULL) {
        Py_CLEAR(self->dupCompareCallback);
    }
    Py_DECREF(self->private_obj);
    PyObject_Del(self);
}

static DBCursorObject*
newDBCursorObject(DBC* dbc, DBTxnObject *txn, DBObject* db)
{
    DBCursorObject* self = PyObject_New(DBCursorObject, &DBCursor_Type);
    if (self == NULL)
        return NULL;

    self->dbc = dbc;
    self->mydb = db;

    INSERT_IN_DOUBLE_LINKED_LIST(self->mydb->children_cursors,self);
    if (txn && ((PyObject *)txn!=Py_None)) {
            INSERT_IN_DOUBLE_LINKED_LIST_TXN(txn->children_cursors,self);
            self->txn=txn;
    } else {
            self->txn=NULL;
    }

    self->in_weakreflist = NULL;
    Py_INCREF(self->mydb);
    return self;
}


/* Forward declaration */
static PyObject *DBC_close_internal(DBCursorObject* self);

static void
DBCursor_dealloc(DBCursorObject* self)
{
    PyObject *dummy;

    if (self->dbc != NULL) {
        dummy=DBC_close_internal(self);
        /*
        ** Raising exceptions while doing
        ** garbage collection is a fatal error.
        */
        if (dummy)
            Py_DECREF(dummy);
        else
            PyErr_Clear();
    }
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }
    Py_DECREF(self->mydb);
    PyObject_Del(self);
}


static DBLogCursorObject*
newDBLogCursorObject(DB_LOGC* dblogc, DBEnvObject* env)
{
    DBLogCursorObject* self;

    self = PyObject_New(DBLogCursorObject, &DBLogCursor_Type);

    if (self == NULL)
        return NULL;

    self->logc = dblogc;
    self->env = env;

    INSERT_IN_DOUBLE_LINKED_LIST(self->env->children_logcursors, self);

    self->in_weakreflist = NULL;
    Py_INCREF(self->env);
    return self;
}


/* Forward declaration */
static PyObject *DBLogCursor_close_internal(DBLogCursorObject* self);

static void
DBLogCursor_dealloc(DBLogCursorObject* self)
{
    PyObject *dummy;

    if (self->logc != NULL) {
        dummy = DBLogCursor_close_internal(self);
        /*
        ** Raising exceptions while doing
        ** garbage collection is a fatal error.
        */
        if (dummy)
            Py_DECREF(dummy);
        else
            PyErr_Clear();
    }
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }
    Py_DECREF(self->env);
    PyObject_Del(self);
}


static DBEnvObject*
newDBEnvObject(int flags)
{
    int err;
    DBEnvObject* self = PyObject_New(DBEnvObject, &DBEnv_Type);
    if (self == NULL)
        return NULL;

    self->db_env = NULL;
    self->closed = 1;
    self->flags = flags;
    self->moduleFlags.getReturnsNone = DEFAULT_GET_RETURNS_NONE;
    self->moduleFlags.cursorSetReturnsNone = DEFAULT_CURSOR_SET_RETURNS_NONE;
    self->children_dbs = NULL;
    self->children_txns = NULL;
    self->children_logcursors = NULL ;
#if (DBVER >= 52)
    self->children_sites = NULL;
#endif
    Py_INCREF(Py_None);
    self->private_obj = Py_None;
    Py_INCREF(Py_None);
    self->rep_transport = Py_None;
    self->in_weakreflist = NULL;
    self->event_notifyCallback = NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = db_env_create(&self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        Py_DECREF(self);
        self = NULL;
    }
    else {
        self->db_env->set_errcall(self->db_env, _db_errorCallback);
        self->db_env->app_private = self;
    }
    return self;
}

/* Forward declaration */
static PyObject *DBEnv_close_internal(DBEnvObject* self, int flags);

static void
DBEnv_dealloc(DBEnvObject* self)
{
  PyObject *dummy;

    if (self->db_env) {
        dummy=DBEnv_close_internal(self, 0);
        /*
        ** Raising exceptions while doing
        ** garbage collection is a fatal error.
        */
        if (dummy)
            Py_DECREF(dummy);
        else
            PyErr_Clear();
    }

    Py_CLEAR(self->event_notifyCallback);

    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }
    Py_DECREF(self->private_obj);
    Py_DECREF(self->rep_transport);
    PyObject_Del(self);
}


static DBTxnObject*
newDBTxnObject(DBEnvObject* myenv, DBTxnObject *parent, DB_TXN *txn, int flags)
{
    int err;
    DB_TXN *parent_txn = NULL;

    DBTxnObject* self = PyObject_New(DBTxnObject, &DBTxn_Type);
    if (self == NULL)
        return NULL;

    self->in_weakreflist = NULL;
    self->children_txns = NULL;
    self->children_dbs = NULL;
    self->children_cursors = NULL;
    self->children_sequences = NULL;
    self->flag_prepare = 0;
    self->parent_txn = NULL;
    self->env = NULL;
    /* We initialize just in case "txn_begin" fails */
    self->txn = NULL;

    if (parent && ((PyObject *)parent!=Py_None)) {
        parent_txn = parent->txn;
    }

    if (txn) {
        self->txn = txn;
    } else {
        MYDB_BEGIN_ALLOW_THREADS;
        err = myenv->db_env->txn_begin(myenv->db_env, parent_txn, &(self->txn), flags);
        MYDB_END_ALLOW_THREADS;

        if (makeDBError(err)) {
            /* Free object half initialized */
            Py_DECREF(self);
            return NULL;
        }
    }

    /* Can't use 'parent' because could be 'parent==Py_None' */
    if (parent_txn) {
        self->parent_txn = parent;
        Py_INCREF(parent);
        self->env = NULL;
        INSERT_IN_DOUBLE_LINKED_LIST(parent->children_txns, self);
    } else {
        self->parent_txn = NULL;
        Py_INCREF(myenv);
        self->env = myenv;
        INSERT_IN_DOUBLE_LINKED_LIST(myenv->children_txns, self);
    }

    return self;
}

/* Forward declaration */
static PyObject *
DBTxn_abort_discard_internal(DBTxnObject* self, int discard);

static void
DBTxn_dealloc(DBTxnObject* self)
{
  PyObject *dummy;

    if (self->txn) {
        int flag_prepare = self->flag_prepare;

        dummy=DBTxn_abort_discard_internal(self, 0);
        /*
        ** Raising exceptions while doing
        ** garbage collection is a fatal error.
        */
        if (dummy)
            Py_DECREF(dummy);
        else
            PyErr_Clear();

        if (!flag_prepare) {
            PyErr_Warn(PyExc_RuntimeWarning,
              "DBTxn aborted in destructor.  No prior commit() or abort().");
        }
    }

    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }

    if (self->env) {
        Py_DECREF(self->env);
    } else {
        /*
        ** We can have "self->env==NULL" and "self->parent_txn==NULL"
        ** if something happens when creating the transaction object
        ** and we abort the object while half done.
        */
        Py_XDECREF(self->parent_txn);
    }
    PyObject_Del(self);
}


static DBLockObject*
newDBLockObject(DBEnvObject* myenv, u_int32_t locker, DBT* obj,
                db_lockmode_t lock_mode, int flags)
{
    int err;
    DBLockObject* self = PyObject_New(DBLockObject, &DBLock_Type);
    if (self == NULL)
        return NULL;
    self->in_weakreflist = NULL;
    self->lock_initialized = 0;  /* Just in case the call fails */

    MYDB_BEGIN_ALLOW_THREADS;
    err = myenv->db_env->lock_get(myenv->db_env, locker, flags, obj, lock_mode,
                                  &self->lock);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        Py_DECREF(self);
        self = NULL;
    } else {
        self->lock_initialized = 1;
    }

    return self;
}


static void
DBLock_dealloc(DBLockObject* self)
{
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }
    /* TODO: is this lock held? should we release it? */
    /* CAUTION: The lock can be not initialized if the creation has failed */

    PyObject_Del(self);
}


static DBSequenceObject*
newDBSequenceObject(DBObject* mydb,  int flags)
{
    int err;
    DBSequenceObject* self = PyObject_New(DBSequenceObject, &DBSequence_Type);
    if (self == NULL)
        return NULL;
    Py_INCREF(mydb);
    self->mydb = mydb;

    INSERT_IN_DOUBLE_LINKED_LIST(self->mydb->children_sequences,self);
    self->txn = NULL;

    self->in_weakreflist = NULL;
    self->sequence = NULL;  /* Just in case the call fails */

    MYDB_BEGIN_ALLOW_THREADS;
    err = db_sequence_create(&self->sequence, self->mydb->db, flags);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        Py_DECREF(self);
        self = NULL;
    }

    return self;
}

/* Forward declaration */
static PyObject
*DBSequence_close_internal(DBSequenceObject* self, int flags, int do_not_close);

static void
DBSequence_dealloc(DBSequenceObject* self)
{
    PyObject *dummy;

    if (self->sequence != NULL) {
        dummy=DBSequence_close_internal(self,0,0);
        /*
        ** Raising exceptions while doing
        ** garbage collection is a fatal error.
        */
        if (dummy)
            Py_DECREF(dummy);
        else
            PyErr_Clear();
    }

    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }

    Py_DECREF(self->mydb);
    PyObject_Del(self);
}

#if (DBVER >= 52)
static DBSiteObject*
newDBSiteObject(DB_SITE* sitep, DBEnvObject* env)
{
    DBSiteObject* self;

    self = PyObject_New(DBSiteObject, &DBSite_Type);

    if (self == NULL)
        return NULL;

    self->site = sitep;
    self->env = env;

    INSERT_IN_DOUBLE_LINKED_LIST(self->env->children_sites, self);

    self->in_weakreflist = NULL;
    Py_INCREF(self->env);
    return self;
}

/* Forward declaration */
static PyObject *DBSite_close_internal(DBSiteObject* self);

static void
DBSite_dealloc(DBSiteObject* self)
{
    PyObject *dummy;

    if (self->site != NULL) {
        dummy = DBSite_close_internal(self);
        /*
        ** Raising exceptions while doing
        ** garbage collection is a fatal error.
        */
        if (dummy)
            Py_DECREF(dummy);
        else
            PyErr_Clear();
    }
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }
    Py_DECREF(self->env);
    PyObject_Del(self);
}
#endif

/* --------------------------------------------------------------------- */
/* DB methods */

static PyObject*
DB_append(DBObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* txnobj = NULL;
    PyObject* dataobj;
    db_recno_t recno;
    DBT key, data;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "data", "txn", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:append", kwnames,
                                     &dataobj, &txnobj))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);

    /* make a dummy key out of a recno */
    recno = 0;
    CLEAR_DBT(key);
    key.data = &recno;
    key.size = sizeof(recno);
    key.ulen = key.size;
    key.flags = DB_DBT_USERMEM;

    if (!make_dbt(dataobj, &data)) return NULL;
    if (!checkTxnObj(txnobj, &txn)) return NULL;

    if (-1 == _DB_put(self, txn, &key, &data, DB_APPEND))
        return NULL;

    return NUMBER_FromLong(recno);
}


static int
_db_associateCallback(DB* db, const DBT* priKey, const DBT* priData,
                      DBT* secKey)
{
    int       retval = DB_DONOTINDEX;
    DBObject* secondaryDB = (DBObject*)db->app_private;
    PyObject* callback = secondaryDB->associateCallback;
    int       type = secondaryDB->primaryDBType;
    PyObject* args;
    PyObject* result = NULL;


    if (callback != NULL) {
        MYDB_BEGIN_BLOCK_THREADS;

        if (type == DB_RECNO || type == DB_QUEUE)
            args = BuildValue_LS(*((db_recno_t*)priKey->data), priData->data, priData->size);
        else
            args = BuildValue_SS(priKey->data, priKey->size, priData->data, priData->size);
        if (args != NULL) {
                result = PyEval_CallObject(callback, args);
        }
        if (args == NULL || result == NULL) {
            PyErr_Print();
        }
        else if (result == Py_None) {
            retval = DB_DONOTINDEX;
        }
        else if (NUMBER_Check(result)) {
            retval = NUMBER_AsLong(result);
        }
        else if (PyBytes_Check(result)) {
            char* data;
            Py_ssize_t size;

            CLEAR_DBT(*secKey);
            PyBytes_AsStringAndSize(result, &data, &size);
            secKey->flags = DB_DBT_APPMALLOC;   /* DB will free */
            secKey->data = malloc(size);        /* TODO, check this */
            if (secKey->data) {
                memcpy(secKey->data, data, size);
                secKey->size = size;
                retval = 0;
            }
            else {
                PyErr_SetString(PyExc_MemoryError,
                                "malloc failed in _db_associateCallback");
                PyErr_Print();
            }
        }
#if (DBVER >= 46)
        else if (PyList_Check(result))
        {
            char* data;
            Py_ssize_t size;
            int i, listlen;
            DBT* dbts;

            listlen = PyList_Size(result);

            dbts = (DBT *)malloc(sizeof(DBT) * listlen);

            for (i=0; i<listlen; i++)
            {
                if (!PyBytes_Check(PyList_GetItem(result, i)))
                {
                    PyErr_SetString(
                       PyExc_TypeError,
#if (PY_VERSION_HEX < 0x03000000)
"The list returned by DB->associate callback should be a list of strings.");
#else
"The list returned by DB->associate callback should be a list of bytes.");
#endif
                    PyErr_Print();
                }

                PyBytes_AsStringAndSize(
                    PyList_GetItem(result, i),
                    &data, &size);

                CLEAR_DBT(dbts[i]);
                dbts[i].data = malloc(size);          /* TODO, check this */

                if (dbts[i].data)
                {
                    memcpy(dbts[i].data, data, size);
                    dbts[i].size = size;
                    dbts[i].ulen = dbts[i].size;
                    dbts[i].flags = DB_DBT_APPMALLOC;  /* DB will free */
                }
                else
                {
                    PyErr_SetString(PyExc_MemoryError,
                        "malloc failed in _db_associateCallback (list)");
                    PyErr_Print();
                }
            }

            CLEAR_DBT(*secKey);

            secKey->data = dbts;
            secKey->size = listlen;
            secKey->flags = DB_DBT_APPMALLOC | DB_DBT_MULTIPLE;
            retval = 0;
        }
#endif
        else {
            PyErr_SetString(
               PyExc_TypeError,
#if (PY_VERSION_HEX < 0x03000000)
"DB associate callback should return DB_DONOTINDEX/string/list of strings.");
#else
"DB associate callback should return DB_DONOTINDEX/bytes/list of bytes.");
#endif
            PyErr_Print();
        }

        Py_XDECREF(args);
        Py_XDECREF(result);

        MYDB_END_BLOCK_THREADS;
    }
    return retval;
}


static PyObject*
DB_associate(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    DBObject* secondaryDB;
    PyObject* callback;
    PyObject *txnobj = NULL;
    DB_TXN *txn = NULL;
    static char* kwnames[] = {"secondaryDB", "callback", "flags", "txn",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|iO:associate", kwnames,
                                     &secondaryDB, &callback, &flags,
                                     &txnobj)) {
        return NULL;
    }

    if (!checkTxnObj(txnobj, &txn)) return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!DBObject_Check(secondaryDB)) {
        makeTypeError("DB", (PyObject*)secondaryDB);
        return NULL;
    }
    CHECK_DB_NOT_CLOSED(secondaryDB);
    if (callback == Py_None) {
        callback = NULL;
    }
    else if (!PyCallable_Check(callback)) {
        makeTypeError("Callable", callback);
        return NULL;
    }

    /* Save a reference to the callback in the secondary DB. */
    Py_XINCREF(callback);
    Py_SETREF(secondaryDB->associateCallback, callback);
    secondaryDB->primaryDBType = _DB_get_type(self);

    /* PyEval_InitThreads is called here due to a quirk in python 1.5
     * - 2.2.1 (at least) according to Russell Williamson <merel@wt.net>:
     * The global interepreter lock is not initialized until the first
     * thread is created using thread.start_new_thread() or fork() is
     * called.  that would cause the ALLOW_THREADS here to segfault due
     * to a null pointer reference if no threads or child processes
     * have been created.  This works around that and is a no-op if
     * threads have already been initialized.
     *  (see pybsddb-users mailing list post on 2002-08-07)
     */
#ifdef WITH_THREAD
    PyEval_InitThreads();
#endif
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->associate(self->db,
                              txn,
                              secondaryDB->db,
                              _db_associateCallback,
                              flags);
    MYDB_END_ALLOW_THREADS;

    if (err) {
        Py_CLEAR(secondaryDB->associateCallback);
        secondaryDB->primaryDBType = 0;
    }

    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_close_internal(DBObject* self, int flags, int do_not_close)
{
    PyObject *dummy;
    int err = 0;

    if (self->db != NULL) {
        /* Can be NULL if db is not in an environment */
        EXTRACT_FROM_DOUBLE_LINKED_LIST_MAYBE_NULL(self);

        if (self->txn) {
            EXTRACT_FROM_DOUBLE_LINKED_LIST_TXN(self);
            self->txn=NULL;
        }

        while(self->children_cursors) {
          dummy=DBC_close_internal(self->children_cursors);
          Py_XDECREF(dummy);
        }

        while(self->children_sequences) {
            dummy=DBSequence_close_internal(self->children_sequences,0,0);
            Py_XDECREF(dummy);
        }

        /*
        ** "do_not_close" is used to dispose all related objects in the
        ** tree, without actually releasing the "root" object.
        ** This is done, for example, because function calls like
        ** "DB.verify()" implicitly close the underlying handle. So
        ** the handle doesn't need to be closed, but related objects
        ** must be cleaned up.
        */
        if (!do_not_close) {
            MYDB_BEGIN_ALLOW_THREADS;
            err = self->db->close(self->db, flags);
            MYDB_END_ALLOW_THREADS;
            self->db = NULL;
        }
        RETURN_IF_ERR();
    }
    RETURN_NONE();
}

static PyObject*
DB_close(DBObject* self, PyObject* args)
{
    int flags=0;
    if (!PyArg_ParseTuple(args,"|i:close", &flags))
        return NULL;
    return DB_close_internal(self, flags, 0);
}


static PyObject*
_DB_consume(DBObject* self, PyObject* args, PyObject* kwargs, int consume_flag)
{
    int err, flags=0, type;
    PyObject* txnobj = NULL;
    PyObject* retval = NULL;
    DBT key, data;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:consume", kwnames,
                                     &txnobj, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    type = _DB_get_type(self);
    if (type == -1)
        return NULL;
    if (type != DB_QUEUE) {
        PyErr_SetString(PyExc_TypeError,
                        "Consume methods only allowed for Queue DB's");
        return NULL;
    }
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    CLEAR_DBT(key);
    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self, DB_THREAD)) {
        /* Tell Berkeley DB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
        key.flags = DB_DBT_MALLOC;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, txn, &key, &data, flags|consume_flag);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
            && self->moduleFlags.getReturnsNone) {
        err = 0;
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (!err) {
        retval = BuildValue_SS(key.data, key.size, data.data, data.size);
        FREE_DBT(key);
        FREE_DBT(data);
    }

    RETURN_IF_ERR();
    return retval;
}

static PyObject*
DB_consume(DBObject* self, PyObject* args, PyObject* kwargs, int consume_flag)
{
    return _DB_consume(self, args, kwargs, DB_CONSUME);
}

static PyObject*
DB_consume_wait(DBObject* self, PyObject* args, PyObject* kwargs,
                int consume_flag)
{
    return _DB_consume(self, args, kwargs, DB_CONSUME_WAIT);
}


static PyObject*
DB_cursor(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    DBC* dbc;
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:cursor", kwnames,
                                     &txnobj, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->cursor(self->db, txn, &dbc, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return (PyObject*) newDBCursorObject(dbc, (DBTxnObject *)txnobj, self);
}


static PyObject*
DB_delete(DBObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* txnobj = NULL;
    int flags = 0;
    PyObject* keyobj;
    DBT key;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "key", "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oi:delete", kwnames,
                                     &keyobj, &txnobj, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;
    if (!checkTxnObj(txnobj, &txn)) {
        FREE_DBT(key);
        return NULL;
    }

    if (-1 == _DB_delete(self, txn, &key, 0)) {
        FREE_DBT(key);
        return NULL;
    }

    FREE_DBT(key);
    RETURN_NONE();
}


#if (DBVER >= 47)
/*
** This function is available since Berkeley DB 4.4,
** but 4.6 version is so buggy that we only support
** it from BDB 4.7 and newer.
*/
static PyObject*
DB_compact(DBObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* txnobj = NULL;
    PyObject *startobj = NULL, *stopobj = NULL;
    int flags = 0;
    DB_TXN *txn = NULL;
    DBT *start_p = NULL, *stop_p = NULL;
    DBT start, stop;
    int err;
    DB_COMPACT c_data = { 0 };
    static char* kwnames[] = { "txn", "start", "stop", "flags",
                               "compact_fillpercent", "compact_pages",
                               "compact_timeout", NULL };


    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOOiiiI:compact", kwnames,
                                     &txnobj, &startobj, &stopobj, &flags,
                                     &c_data.compact_fillpercent,
                                     &c_data.compact_pages,
                                     &c_data.compact_timeout))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!checkTxnObj(txnobj, &txn)) {
        return NULL;
    }

    if (startobj && make_key_dbt(self, startobj, &start, NULL)) {
        start_p = &start;
    }
    if (stopobj && make_key_dbt(self, stopobj, &stop, NULL)) {
        stop_p = &stop;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->compact(self->db, txn, start_p, stop_p, &c_data,
                            flags, NULL);
    MYDB_END_ALLOW_THREADS;

    if (startobj)
        FREE_DBT(start);
    if (stopobj)
        FREE_DBT(stop);

    RETURN_IF_ERR();

    return PyLong_FromUnsignedLong(c_data.compact_pages_truncated);
}
#endif


static PyObject*
DB_fd(DBObject* self)
{
    int err, the_fd;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->fd(self->db, &the_fd);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(the_fd);
}


#if (DBVER >= 46)
static PyObject*
DB_exists(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    DBT key;
    DB_TXN *txn;

    static char* kwnames[] = {"key", "txn", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oi:exists", kwnames,
                &keyobj, &txnobj, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;
    if (!checkTxnObj(txnobj, &txn)) {
        FREE_DBT(key);
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->exists(self->db, txn, &key, flags);
    MYDB_END_ALLOW_THREADS;

    FREE_DBT(key);

    if (!err) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)) {
        Py_INCREF(Py_False);
        return Py_False;
    }

    /*
    ** If we reach there, there was an error. The
    ** "return" should be unreachable.
    */
    RETURN_IF_ERR();
    assert(0);  /* This coude SHOULD be unreachable */
    return NULL;
}
#endif

static PyObject*
DB_get(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    PyObject* dfltobj = NULL;
    PyObject* retval = NULL;
    int dlen = -1;
    int doff = -1;
    DBT key, data;
    DB_TXN *txn = NULL;
    static char* kwnames[] = {"key", "default", "txn", "flags", "dlen",
                                    "doff", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOiii:get", kwnames,
                                     &keyobj, &dfltobj, &txnobj, &flags, &dlen,
                                     &doff))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, &flags))
        return NULL;
    if (!checkTxnObj(txnobj, &txn)) {
        FREE_DBT(key);
        return NULL;
    }

    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self, DB_THREAD)) {
        /* Tell Berkeley DB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
    }
    if (!add_partial_dbt(&data, dlen, doff)) {
        FREE_DBT(key);
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, txn, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY) && (dfltobj != NULL)) {
        err = 0;
        Py_INCREF(dfltobj);
        retval = dfltobj;
    }
    else if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
             && self->moduleFlags.getReturnsNone) {
        err = 0;
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (!err) {
        if (flags & DB_SET_RECNO) /* return both key and data */
            retval = BuildValue_SS(key.data, key.size, data.data, data.size);
        else /* return just the data */
            retval = Build_PyString(data.data, data.size);
        FREE_DBT(data);
    }
    FREE_DBT(key);

    RETURN_IF_ERR();
    return retval;
}

static PyObject*
DB_pget(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    PyObject* dfltobj = NULL;
    PyObject* retval = NULL;
    int dlen = -1;
    int doff = -1;
    DBT key, pkey, data;
    DB_TXN *txn = NULL;
    static char* kwnames[] = {"key", "default", "txn", "flags", "dlen",
                                    "doff", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOiii:pget", kwnames,
                                     &keyobj, &dfltobj, &txnobj, &flags, &dlen,
                                     &doff))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, &flags))
        return NULL;
    if (!checkTxnObj(txnobj, &txn)) {
        FREE_DBT(key);
        return NULL;
    }

    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self, DB_THREAD)) {
        /* Tell Berkeley DB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
    }
    if (!add_partial_dbt(&data, dlen, doff)) {
        FREE_DBT(key);
        return NULL;
    }

    CLEAR_DBT(pkey);
    pkey.flags = DB_DBT_MALLOC;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->pget(self->db, txn, &key, &pkey, &data, flags);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY) && (dfltobj != NULL)) {
        err = 0;
        Py_INCREF(dfltobj);
        retval = dfltobj;
    }
    else if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
             && self->moduleFlags.getReturnsNone) {
        err = 0;
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (!err) {
        PyObject *pkeyObj;
        PyObject *dataObj;
        dataObj = Build_PyString(data.data, data.size);

        if (self->primaryDBType == DB_RECNO ||
            self->primaryDBType == DB_QUEUE)
            pkeyObj = NUMBER_FromLong(*(int *)pkey.data);
        else
            pkeyObj = Build_PyString(pkey.data, pkey.size);

        if (flags & DB_SET_RECNO) /* return key , pkey and data */
        {
            PyObject *keyObj;
            int type = _DB_get_type(self);
            if (type == DB_RECNO || type == DB_QUEUE)
                keyObj = NUMBER_FromLong(*(int *)key.data);
            else
                keyObj = Build_PyString(key.data, key.size);
            retval = PyTuple_Pack(3, keyObj, pkeyObj, dataObj);
            Py_DECREF(keyObj);
        }
        else /* return just the pkey and data */
        {
            retval = PyTuple_Pack(2, pkeyObj, dataObj);
        }
        Py_DECREF(dataObj);
        Py_DECREF(pkeyObj);
        FREE_DBT(pkey);
        FREE_DBT(data);
    }
    FREE_DBT(key);

    RETURN_IF_ERR();
    return retval;
}


/* Return size of entry */
static PyObject*
DB_get_size(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    PyObject* retval = NULL;
    DBT key, data;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "key", "txn", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get_size", kwnames,
                                     &keyobj, &txnobj))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, &flags))
        return NULL;
    if (!checkTxnObj(txnobj, &txn)) {
        FREE_DBT(key);
        return NULL;
    }
    CLEAR_DBT(data);

    /* We don't allocate any memory, forcing a DB_BUFFER_SMALL error and
       thus getting the record size. */
    data.flags = DB_DBT_USERMEM;
    data.ulen = 0;
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, txn, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;
    if ((err == DB_BUFFER_SMALL) || (err == 0)) {
        retval = NUMBER_FromLong((long)data.size);
        err = 0;
    }

    FREE_DBT(key);
    FREE_DBT(data);
    RETURN_IF_ERR();
    return retval;
}


static PyObject*
DB_get_both(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    PyObject* dataobj;
    PyObject* retval = NULL;
    DBT key, data;
    void *orig_data;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "key", "data", "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|Oi:get_both", kwnames,
                                     &keyobj, &dataobj, &txnobj, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;
    if ( !make_dbt(dataobj, &data) ||
         !checkTxnObj(txnobj, &txn) )
    {
        FREE_DBT(key);
        return NULL;
    }

    flags |= DB_GET_BOTH;
    orig_data = data.data;

    if (CHECK_DBFLAG(self, DB_THREAD)) {
        /* Tell Berkeley DB to malloc the return value (thread safe) */
        /* XXX(nnorwitz): At least 4.4.20 and 4.5.20 require this flag. */
        data.flags = DB_DBT_MALLOC;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, txn, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
            && self->moduleFlags.getReturnsNone) {
        err = 0;
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (!err) {
        /* XXX(nnorwitz): can we do: retval = dataobj; Py_INCREF(retval); */
        retval = Build_PyString(data.data, data.size);

        /* Even though the flags require DB_DBT_MALLOC, data is not always
           allocated.  4.4: allocated, 4.5: *not* allocated. :-( */
        if (data.data != orig_data)
            FREE_DBT(data);
    }

    FREE_DBT(key);
    RETURN_IF_ERR();
    return retval;
}


static PyObject*
DB_get_byteswapped(DBObject* self)
{
    int err = 0;
    int retval = -1;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_byteswapped(self->db, &retval);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(retval);
}


static PyObject*
DB_get_type(DBObject* self)
{
    int type;

    CHECK_DB_NOT_CLOSED(self);

    type = _DB_get_type(self);
    if (type == -1)
        return NULL;
    return NUMBER_FromLong(type);
}


static PyObject*
DB_join(DBObject* self, PyObject* args)
{
    int err, flags=0;
    int length, x;
    PyObject* cursorsObj;
    DBC** cursors;
    DBC*  dbc;

    if (!PyArg_ParseTuple(args,"O|i:join", &cursorsObj, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);

    if (!PySequence_Check(cursorsObj)) {
        PyErr_SetString(PyExc_TypeError,
                        "Sequence of DBCursor objects expected");
        return NULL;
    }

    length = PyObject_Length(cursorsObj);
    cursors = malloc((length+1) * sizeof(DBC*));
    if (!cursors) {
        PyErr_NoMemory();
        return NULL;
    }

    cursors[length] = NULL;
    for (x=0; x<length; x++) {
        PyObject* item = PySequence_GetItem(cursorsObj, x);
        if (item == NULL) {
            free(cursors);
            return NULL;
        }
        if (!DBCursorObject_Check(item)) {
            PyErr_SetString(PyExc_TypeError,
                            "Sequence of DBCursor objects expected");
            free(cursors);
            return NULL;
        }
        cursors[x] = ((DBCursorObject*)item)->dbc;
        Py_DECREF(item);
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->join(self->db, cursors, &dbc, flags);
    MYDB_END_ALLOW_THREADS;
    free(cursors);
    RETURN_IF_ERR();

    /* FIXME: this is a buggy interface.  The returned cursor
       contains internal references to the passed in cursors
       but does not hold python references to them or prevent
       them from being closed prematurely.  This can cause
       python to crash when things are done in the wrong order. */
    return (PyObject*) newDBCursorObject(dbc, NULL, self);
}


static PyObject*
DB_key_range(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    PyObject* txnobj = NULL;
    PyObject* keyobj;
    DBT key;
    DB_TXN *txn = NULL;
    DB_KEY_RANGE range;
    static char* kwnames[] = { "key", "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oi:key_range", kwnames,
                                     &keyobj, &txnobj, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!make_dbt(keyobj, &key))
        /* BTree only, don't need to allow for an int key */
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->key_range(self->db, txn, &key, &range, flags);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    return Py_BuildValue("ddd", range.less, range.equal, range.greater);
}


static PyObject*
DB_open(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, type = DB_UNKNOWN, flags=0, mode=0660;
    char* filename = NULL;
    char* dbname = NULL;
    PyObject *txnobj = NULL;
    DB_TXN *txn = NULL;
    /* with dbname */
    static char* kwnames[] = {
        "filename", "dbname", "dbtype", "flags", "mode", "txn", NULL};
    /* without dbname */
    static char* kwnames_basic[] = {
        "filename", "dbtype", "flags", "mode", "txn", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "z|ziiiO:open", kwnames,
                                     &filename, &dbname, &type, &flags, &mode,
                                     &txnobj))
    {
        PyErr_Clear();
        type = DB_UNKNOWN; flags = 0; mode = 0660;
        filename = NULL; dbname = NULL;
        if (!PyArg_ParseTupleAndKeywords(args, kwargs,"z|iiiO:open",
                                         kwnames_basic,
                                         &filename, &type, &flags, &mode,
                                         &txnobj))
            return NULL;
    }

    if (!checkTxnObj(txnobj, &txn)) return NULL;

    if (NULL == self->db) {
        PyObject *t = Py_BuildValue("(is)", 0,
                                "Cannot call open() twice for DB object");
        if (t) {
            PyErr_SetObject(DBError, t);
            Py_DECREF(t);
        }
        return NULL;
    }

    if (txn) {  /* Can't use 'txnobj' because could be 'txnobj==Py_None' */
        INSERT_IN_DOUBLE_LINKED_LIST_TXN(((DBTxnObject *)txnobj)->children_dbs,self);
        self->txn=(DBTxnObject *)txnobj;
    } else {
        self->txn=NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->open(self->db, txn, filename, dbname, type, flags, mode);
    MYDB_END_ALLOW_THREADS;

    if (makeDBError(err)) {
        PyObject *dummy;

        dummy=DB_close_internal(self, 0, 0);
        Py_XDECREF(dummy);
        return NULL;
    }

    self->db->get_flags(self->db, &self->setflags);

    self->flags = flags;

    RETURN_NONE();
}


static PyObject*
DB_put(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int flags=0;
    PyObject* txnobj = NULL;
    int dlen = -1;
    int doff = -1;
    PyObject* keyobj, *dataobj, *retval;
    DBT key, data;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "key", "data", "txn", "flags", "dlen",
                                     "doff", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|Oiii:put", kwnames,
                         &keyobj, &dataobj, &txnobj, &flags, &dlen, &doff))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;
    if ( !make_dbt(dataobj, &data) ||
         !add_partial_dbt(&data, dlen, doff) ||
         !checkTxnObj(txnobj, &txn) )
    {
        FREE_DBT(key);
        return NULL;
    }

    if (-1 == _DB_put(self, txn, &key, &data, flags)) {
        FREE_DBT(key);
        return NULL;
    }

    if (flags & DB_APPEND)
        retval = NUMBER_FromLong(*((db_recno_t*)key.data));
    else {
        retval = Py_None;
        Py_INCREF(retval);
    }
    FREE_DBT(key);
    return retval;
}



static PyObject*
DB_remove(DBObject* self, PyObject* args, PyObject* kwargs)
{
    char* filename;
    char* database = NULL;
    int err, flags=0;
    static char* kwnames[] = { "filename", "dbname", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|zi:remove", kwnames,
                                     &filename, &database, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    EXTRACT_FROM_DOUBLE_LINKED_LIST_MAYBE_NULL(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->remove(self->db, filename, database, flags);
    MYDB_END_ALLOW_THREADS;

    self->db = NULL;
    RETURN_IF_ERR();
    RETURN_NONE();
}



static PyObject*
DB_rename(DBObject* self, PyObject* args)
{
    char* filename;
    char* database;
    char* newname;
    int err, flags=0;

    if (!PyArg_ParseTuple(args, "sss|i:rename", &filename, &database, &newname,
                          &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->rename(self->db, filename, database, newname, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_get_private(DBObject* self)
{
    /* We can give out the private field even if db is closed */
    Py_INCREF(self->private_obj);
    return self->private_obj;
}

static PyObject*
DB_set_private(DBObject* self, PyObject* private_obj)
{
    /* We can set the private field even if db is closed */
    Py_INCREF(private_obj);
    Py_SETREF(self->private_obj, private_obj);
    RETURN_NONE();
}

#if (DBVER >= 46)
static PyObject*
DB_set_priority(DBObject* self, PyObject* args)
{
    int err, priority;

    if (!PyArg_ParseTuple(args,"i:set_priority", &priority))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_priority(self->db, priority);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_priority(DBObject* self)
{
    int err = 0;
    DB_CACHE_PRIORITY priority;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_priority(self->db, &priority);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(priority);
}
#endif

static PyObject*
DB_get_dbname(DBObject* self)
{
    int err;
    const char *filename, *dbname;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_dbname(self->db, &filename, &dbname);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    /* If "dbname==NULL", it is correctly converted to "None" */
    return Py_BuildValue("(ss)", filename, dbname);
}

static PyObject*
DB_get_open_flags(DBObject* self)
{
    int err;
    unsigned int flags;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_open_flags(self->db, &flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(flags);
}

static PyObject*
DB_set_q_extentsize(DBObject* self, PyObject* args)
{
    int err;
    u_int32_t extentsize;

    if (!PyArg_ParseTuple(args,"i:set_q_extentsize", &extentsize))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_q_extentsize(self->db, extentsize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_q_extentsize(DBObject* self)
{
    int err = 0;
    u_int32_t extentsize;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_q_extentsize(self->db, &extentsize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(extentsize);
}

static PyObject*
DB_set_bt_minkey(DBObject* self, PyObject* args)
{
    int err, minkey;

    if (!PyArg_ParseTuple(args,"i:set_bt_minkey", &minkey))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_bt_minkey(self->db, minkey);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_bt_minkey(DBObject* self)
{
    int err;
    u_int32_t bt_minkey;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_bt_minkey(self->db, &bt_minkey);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(bt_minkey);
}

static int
_default_cmp(const DBT *leftKey,
             const DBT *rightKey)
{
  int res;
  int lsize = leftKey->size, rsize = rightKey->size;

  res = memcmp(leftKey->data, rightKey->data,
               lsize < rsize ? lsize : rsize);

  if (res == 0) {
      if (lsize < rsize) {
          res = -1;
      }
      else if (lsize > rsize) {
          res = 1;
      }
  }
  return res;
}

static int
_db_compareCallback(DB* db,
                    const DBT *leftKey,
                    const DBT *rightKey)
{
    int res = 0;
    PyObject *args;
    PyObject *result = NULL;
    DBObject *self = (DBObject *)db->app_private;

    if (self == NULL || self->btCompareCallback == NULL) {
        MYDB_BEGIN_BLOCK_THREADS;
        PyErr_SetString(PyExc_TypeError,
                        (self == 0
                         ? "DB_bt_compare db is NULL."
                         : "DB_bt_compare callback is NULL."));
        /* we're in a callback within the DB code, we can't raise */
        PyErr_Print();
        res = _default_cmp(leftKey, rightKey);
        MYDB_END_BLOCK_THREADS;
    } else {
        MYDB_BEGIN_BLOCK_THREADS;

        args = BuildValue_SS(leftKey->data, leftKey->size, rightKey->data, rightKey->size);
        if (args != NULL) {
                result = PyEval_CallObject(self->btCompareCallback, args);
        }
        if (args == NULL || result == NULL) {
            /* we're in a callback within the DB code, we can't raise */
            PyErr_Print();
            res = _default_cmp(leftKey, rightKey);
        } else if (NUMBER_Check(result)) {
            res = NUMBER_AsLong(result);
        } else {
            PyErr_SetString(PyExc_TypeError,
                            "DB_bt_compare callback MUST return an int.");
            /* we're in a callback within the DB code, we can't raise */
            PyErr_Print();
            res = _default_cmp(leftKey, rightKey);
        }

        Py_XDECREF(args);
        Py_XDECREF(result);

        MYDB_END_BLOCK_THREADS;
    }
    return res;
}

static PyObject*
DB_set_bt_compare(DBObject* self, PyObject* comparator)
{
    int err;
    PyObject *tuple, *result;

    CHECK_DB_NOT_CLOSED(self);

    if (!PyCallable_Check(comparator)) {
        makeTypeError("Callable", comparator);
        return NULL;
    }

    /*
     * Perform a test call of the comparator function with two empty
     * string objects here.  verify that it returns an int (0).
     * err if not.
     */
    tuple = Py_BuildValue("(ss)", "", "");
    result = PyEval_CallObject(comparator, tuple);
    Py_DECREF(tuple);
    if (result == NULL)
        return NULL;
    if (!NUMBER_Check(result)) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_TypeError,
                        "callback MUST return an int");
        return NULL;
    } else if (NUMBER_AsLong(result) != 0) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_TypeError,
                        "callback failed to return 0 on two empty strings");
        return NULL;
    }
    Py_DECREF(result);

    /* We don't accept multiple set_bt_compare operations, in order to
     * simplify the code. This would have no real use, as one cannot
     * change the function once the db is opened anyway */
    if (self->btCompareCallback != NULL) {
        PyErr_SetString(PyExc_RuntimeError, "set_bt_compare() cannot be called more than once");
        return NULL;
    }

    Py_INCREF(comparator);
    self->btCompareCallback = comparator;

    /* This is to workaround a problem with un-initialized threads (see
       comment in DB_associate) */
#ifdef WITH_THREAD
    PyEval_InitThreads();
#endif

    err = self->db->set_bt_compare(self->db, _db_compareCallback);

    if (err) {
        /* restore the old state in case of error */
        Py_DECREF(comparator);
        self->btCompareCallback = NULL;
    }

    RETURN_IF_ERR();
    RETURN_NONE();
}

static int
_db_dupCompareCallback(DB* db,
		    const DBT *leftKey,
		    const DBT *rightKey)
{
    int res = 0;
    PyObject *args;
    PyObject *result = NULL;
    DBObject *self = (DBObject *)db->app_private;

    if (self == NULL || self->dupCompareCallback == NULL) {
	MYDB_BEGIN_BLOCK_THREADS;
	PyErr_SetString(PyExc_TypeError,
			(self == 0
			 ? "DB_dup_compare db is NULL."
			 : "DB_dup_compare callback is NULL."));
	/* we're in a callback within the DB code, we can't raise */
	PyErr_Print();
	res = _default_cmp(leftKey, rightKey);
	MYDB_END_BLOCK_THREADS;
    } else {
	MYDB_BEGIN_BLOCK_THREADS;

	args = BuildValue_SS(leftKey->data, leftKey->size, rightKey->data, rightKey->size);
	if (args != NULL) {
		result = PyEval_CallObject(self->dupCompareCallback, args);
	}
	if (args == NULL || result == NULL) {
	    /* we're in a callback within the DB code, we can't raise */
	    PyErr_Print();
	    res = _default_cmp(leftKey, rightKey);
	} else if (NUMBER_Check(result)) {
	    res = NUMBER_AsLong(result);
	} else {
	    PyErr_SetString(PyExc_TypeError,
			    "DB_dup_compare callback MUST return an int.");
	    /* we're in a callback within the DB code, we can't raise */
	    PyErr_Print();
	    res = _default_cmp(leftKey, rightKey);
	}

	Py_XDECREF(args);
	Py_XDECREF(result);

	MYDB_END_BLOCK_THREADS;
    }
    return res;
}

static PyObject*
DB_set_dup_compare(DBObject* self, PyObject* comparator)
{
    int err;
    PyObject *tuple, *result;

    CHECK_DB_NOT_CLOSED(self);

    if (!PyCallable_Check(comparator)) {
	makeTypeError("Callable", comparator);
	return NULL;
    }

    /*
     * Perform a test call of the comparator function with two empty
     * string objects here.  verify that it returns an int (0).
     * err if not.
     */
    tuple = Py_BuildValue("(ss)", "", "");
    result = PyEval_CallObject(comparator, tuple);
    Py_DECREF(tuple);
    if (result == NULL)
        return NULL;
    if (!NUMBER_Check(result)) {
	Py_DECREF(result);
	PyErr_SetString(PyExc_TypeError,
		        "callback MUST return an int");
	return NULL;
    } else if (NUMBER_AsLong(result) != 0) {
	Py_DECREF(result);
	PyErr_SetString(PyExc_TypeError,
		        "callback failed to return 0 on two empty strings");
	return NULL;
    }
    Py_DECREF(result);

    /* We don't accept multiple set_dup_compare operations, in order to
     * simplify the code. This would have no real use, as one cannot
     * change the function once the db is opened anyway */
    if (self->dupCompareCallback != NULL) {
	PyErr_SetString(PyExc_RuntimeError, "set_dup_compare() cannot be called more than once");
	return NULL;
    }

    Py_INCREF(comparator);
    self->dupCompareCallback = comparator;

    /* This is to workaround a problem with un-initialized threads (see
       comment in DB_associate) */
#ifdef WITH_THREAD
    PyEval_InitThreads();
#endif

    err = self->db->set_dup_compare(self->db, _db_dupCompareCallback);

    if (err) {
	/* restore the old state in case of error */
	Py_DECREF(comparator);
	self->dupCompareCallback = NULL;
    }

    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_cachesize(DBObject* self, PyObject* args)
{
    int err;
    int gbytes = 0, bytes = 0, ncache = 0;

    if (!PyArg_ParseTuple(args,"ii|i:set_cachesize",
                          &gbytes,&bytes,&ncache))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_cachesize(self->db, gbytes, bytes, ncache);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_cachesize(DBObject* self)
{
    int err;
    u_int32_t gbytes, bytes;
    int ncache;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_cachesize(self->db, &gbytes, &bytes, &ncache);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return Py_BuildValue("(iii)", gbytes, bytes, ncache);
}

static PyObject*
DB_set_flags(DBObject* self, PyObject* args)
{
    int err, flags;

    if (!PyArg_ParseTuple(args,"i:set_flags", &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_flags(self->db, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    self->setflags |= flags;
    RETURN_NONE();
}

static PyObject*
DB_get_flags(DBObject* self)
{
    int err;
    u_int32_t flags;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_flags(self->db, &flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(flags);
}

static PyObject*
DB_get_transactional(DBObject* self)
{
    int err;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_transactional(self->db);
    MYDB_END_ALLOW_THREADS;

    if(err == 0) {
        Py_INCREF(Py_False);
        return Py_False;
    } else if(err == 1) {
        Py_INCREF(Py_True);
        return Py_True;
    }

    /*
    ** If we reach there, there was an error. The
    ** "return" should be unreachable.
    */
    RETURN_IF_ERR();
    assert(0);  /* This code SHOULD be unreachable */
    return NULL;
}

static PyObject*
DB_set_h_ffactor(DBObject* self, PyObject* args)
{
    int err, ffactor;

    if (!PyArg_ParseTuple(args,"i:set_h_ffactor", &ffactor))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_h_ffactor(self->db, ffactor);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_h_ffactor(DBObject* self)
{
    int err;
    u_int32_t ffactor;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_h_ffactor(self->db, &ffactor);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(ffactor);
}

static PyObject*
DB_set_h_nelem(DBObject* self, PyObject* args)
{
    int err, nelem;

    if (!PyArg_ParseTuple(args,"i:set_h_nelem", &nelem))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_h_nelem(self->db, nelem);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_h_nelem(DBObject* self)
{
    int err;
    u_int32_t nelem;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_h_nelem(self->db, &nelem);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(nelem);
}

static PyObject*
DB_set_lorder(DBObject* self, PyObject* args)
{
    int err, lorder;

    if (!PyArg_ParseTuple(args,"i:set_lorder", &lorder))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_lorder(self->db, lorder);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_lorder(DBObject* self)
{
    int err;
    int lorder;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_lorder(self->db, &lorder);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(lorder);
}

static PyObject*
DB_set_pagesize(DBObject* self, PyObject* args)
{
    int err, pagesize;

    if (!PyArg_ParseTuple(args,"i:set_pagesize", &pagesize))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_pagesize(self->db, pagesize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_pagesize(DBObject* self)
{
    int err;
    u_int32_t pagesize;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_pagesize(self->db, &pagesize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(pagesize);
}

static PyObject*
DB_set_re_delim(DBObject* self, PyObject* args)
{
    int err;
    char delim;

    if (!PyArg_ParseTuple(args,"b:set_re_delim", &delim)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args,"c:set_re_delim", &delim))
            return NULL;
    }

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_re_delim(self->db, delim);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_re_delim(DBObject* self)
{
    int err, re_delim;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_re_delim(self->db, &re_delim);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(re_delim);
}

static PyObject*
DB_set_re_len(DBObject* self, PyObject* args)
{
    int err, len;

    if (!PyArg_ParseTuple(args,"i:set_re_len", &len))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_re_len(self->db, len);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_re_len(DBObject* self)
{
    int err;
    u_int32_t re_len;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_re_len(self->db, &re_len);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(re_len);
}

static PyObject*
DB_set_re_pad(DBObject* self, PyObject* args)
{
    int err;
    char pad;

    if (!PyArg_ParseTuple(args,"b:set_re_pad", &pad)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args,"c:set_re_pad", &pad))
            return NULL;
    }
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_re_pad(self->db, pad);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_re_pad(DBObject* self)
{
    int err, re_pad;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_re_pad(self->db, &re_pad);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(re_pad);
}

static PyObject*
DB_set_re_source(DBObject* self, PyObject* args)
{
    int err;
    char *source;

    if (!PyArg_ParseTuple(args,"s:set_re_source", &source))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_re_source(self->db, source);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_re_source(DBObject* self)
{
    int err;
    const char *source;

    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_re_source(self->db, &source);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return PyBytes_FromString(source);
}

static PyObject*
DB_stat(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags = 0, type;
    void* sp;
    PyObject* d;
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "flags", "txn", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iO:stat", kwnames,
                                     &flags, &txnobj))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->stat(self->db, txn, &sp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    /* Turn the stat structure into a dictionary */
    type = _DB_get_type(self);
    if ((type == -1) || ((d = PyDict_New()) == NULL)) {
        free(sp);
        return NULL;
    }

#define MAKE_HASH_ENTRY(name)  _addIntToDict(d, #name, ((DB_HASH_STAT*)sp)->hash_##name)
#define MAKE_BT_ENTRY(name)    _addIntToDict(d, #name, ((DB_BTREE_STAT*)sp)->bt_##name)
#define MAKE_QUEUE_ENTRY(name) _addIntToDict(d, #name, ((DB_QUEUE_STAT*)sp)->qs_##name)

    switch (type) {
    case DB_HASH:
        MAKE_HASH_ENTRY(magic);
        MAKE_HASH_ENTRY(version);
        MAKE_HASH_ENTRY(nkeys);
        MAKE_HASH_ENTRY(ndata);
#if (DBVER >= 46)
        MAKE_HASH_ENTRY(pagecnt);
#endif
        MAKE_HASH_ENTRY(pagesize);
        MAKE_HASH_ENTRY(ffactor);
        MAKE_HASH_ENTRY(buckets);
        MAKE_HASH_ENTRY(free);
        MAKE_HASH_ENTRY(bfree);
        MAKE_HASH_ENTRY(bigpages);
        MAKE_HASH_ENTRY(big_bfree);
        MAKE_HASH_ENTRY(overflows);
        MAKE_HASH_ENTRY(ovfl_free);
        MAKE_HASH_ENTRY(dup);
        MAKE_HASH_ENTRY(dup_free);
        break;

    case DB_BTREE:
    case DB_RECNO:
        MAKE_BT_ENTRY(magic);
        MAKE_BT_ENTRY(version);
        MAKE_BT_ENTRY(nkeys);
        MAKE_BT_ENTRY(ndata);
#if (DBVER >= 46)
        MAKE_BT_ENTRY(pagecnt);
#endif
        MAKE_BT_ENTRY(pagesize);
        MAKE_BT_ENTRY(minkey);
        MAKE_BT_ENTRY(re_len);
        MAKE_BT_ENTRY(re_pad);
        MAKE_BT_ENTRY(levels);
        MAKE_BT_ENTRY(int_pg);
        MAKE_BT_ENTRY(leaf_pg);
        MAKE_BT_ENTRY(dup_pg);
        MAKE_BT_ENTRY(over_pg);
        MAKE_BT_ENTRY(empty_pg);
        MAKE_BT_ENTRY(free);
        MAKE_BT_ENTRY(int_pgfree);
        MAKE_BT_ENTRY(leaf_pgfree);
        MAKE_BT_ENTRY(dup_pgfree);
        MAKE_BT_ENTRY(over_pgfree);
        break;

    case DB_QUEUE:
        MAKE_QUEUE_ENTRY(magic);
        MAKE_QUEUE_ENTRY(version);
        MAKE_QUEUE_ENTRY(nkeys);
        MAKE_QUEUE_ENTRY(ndata);
        MAKE_QUEUE_ENTRY(pagesize);
        MAKE_QUEUE_ENTRY(extentsize);
        MAKE_QUEUE_ENTRY(pages);
        MAKE_QUEUE_ENTRY(re_len);
        MAKE_QUEUE_ENTRY(re_pad);
        MAKE_QUEUE_ENTRY(pgfree);
#if (DBVER == 31)
        MAKE_QUEUE_ENTRY(start);
#endif
        MAKE_QUEUE_ENTRY(first_recno);
        MAKE_QUEUE_ENTRY(cur_recno);
        break;

    default:
        PyErr_SetString(PyExc_TypeError, "Unknown DB type, unable to stat");
        Py_DECREF(d);
        d = NULL;
    }

#undef MAKE_HASH_ENTRY
#undef MAKE_BT_ENTRY
#undef MAKE_QUEUE_ENTRY

    free(sp);
    return d;
}

static PyObject*
DB_stat_print(DBObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:stat_print",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_DB_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->stat_print(self->db, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_sync(DBObject* self, PyObject* args)
{
    int err;
    int flags = 0;

    if (!PyArg_ParseTuple(args,"|i:sync", &flags ))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->sync(self->db, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_truncate(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    u_int32_t count=0;
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "txn", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:cursor", kwnames,
                                     &txnobj, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);
    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->truncate(self->db, txn, &count, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(count);
}


static PyObject*
DB_upgrade(DBObject* self, PyObject* args)
{
    int err, flags=0;
    char *filename;

    if (!PyArg_ParseTuple(args,"s|i:upgrade", &filename, &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->upgrade(self->db, filename, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_verify(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags=0;
    char* fileName;
    char* dbName=NULL;
    char* outFileName=NULL;
    FILE* outFile=NULL;
    static char* kwnames[] = { "filename", "dbname", "outfile", "flags",
                                     NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|zzi:verify", kwnames,
                                     &fileName, &dbName, &outFileName, &flags))
        return NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (outFileName)
        outFile = fopen(outFileName, "w");
        /* XXX(nnorwitz): it should probably be an exception if outFile
           can't be opened. */

    {  /* DB.verify acts as a DB handle destructor (like close) */
        PyObject *error;

        error=DB_close_internal(self, 0, 1);
        if (error) {
            if (outFile)
                fclose(outFile);
            return error;
        }
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->verify(self->db, fileName, dbName, outFile, flags);
    MYDB_END_ALLOW_THREADS;

    self->db = NULL;  /* Implicit close; related objects already released */

    if (outFile)
        fclose(outFile);

    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DB_set_get_returns_none(DBObject* self, PyObject* args)
{
    int flags=0;
    int oldValue=0;

    if (!PyArg_ParseTuple(args,"i:set_get_returns_none", &flags))
        return NULL;
    CHECK_DB_NOT_CLOSED(self);

    if (self->moduleFlags.getReturnsNone)
        ++oldValue;
    if (self->moduleFlags.cursorSetReturnsNone)
        ++oldValue;
    self->moduleFlags.getReturnsNone = (flags >= 1);
    self->moduleFlags.cursorSetReturnsNone = (flags >= 2);
    return NUMBER_FromLong(oldValue);
}

static PyObject*
DB_set_encrypt(DBObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    u_int32_t flags=0;
    char *passwd = NULL;
    static char* kwnames[] = { "passwd", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i:set_encrypt", kwnames,
                &passwd, &flags)) {
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->set_encrypt(self->db, passwd, flags);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DB_get_encrypt_flags(DBObject* self)
{
    int err;
    u_int32_t flags;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get_encrypt_flags(self->db, &flags);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return NUMBER_FromLong(flags);
}



/*-------------------------------------------------------------- */
/* Mapping and Dictionary-like access routines */

Py_ssize_t DB_length(PyObject* _self)
{
    int err;
    Py_ssize_t size = 0;
    void* sp;
    DBObject* self = (DBObject*)_self;

    if (self->db == NULL) {
        PyObject *t = Py_BuildValue("(is)", 0, "DB object has been closed");
        if (t) {
            PyErr_SetObject(DBError, t);
            Py_DECREF(t);
        }
        return -1;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->stat(self->db, /*txnid*/ NULL, &sp, 0);
    MYDB_END_ALLOW_THREADS;

    /* All the stat structures have matching fields upto the ndata field,
       so we can use any of them for the type cast */
    size = ((DB_BTREE_STAT*)sp)->bt_ndata;

    if (err)
        return -1;

    free(sp);
    return size;
}


PyObject* DB_subscript(DBObject* self, PyObject* keyobj)
{
    int err;
    PyObject* retval;
    DBT key;
    DBT data;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;

    CLEAR_DBT(data);
    if (CHECK_DBFLAG(self, DB_THREAD)) {
        /* Tell Berkeley DB to malloc the return value (thread safe) */
        data.flags = DB_DBT_MALLOC;
    }
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->get(self->db, NULL, &key, &data, 0);
    MYDB_END_ALLOW_THREADS;
    if (err == DB_NOTFOUND || err == DB_KEYEMPTY) {
        PyErr_SetObject(PyExc_KeyError, keyobj);
        retval = NULL;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        retval = Build_PyString(data.data, data.size);
        FREE_DBT(data);
    }

    FREE_DBT(key);
    return retval;
}


static int
DB_ass_sub(DBObject* self, PyObject* keyobj, PyObject* dataobj)
{
    DBT key, data;
    int retval;
    int flags = 0;

    if (self->db == NULL) {
        PyObject *t = Py_BuildValue("(is)", 0, "DB object has been closed");
        if (t) {
            PyErr_SetObject(DBError, t);
            Py_DECREF(t);
        }
        return -1;
    }

    if (!make_key_dbt(self, keyobj, &key, NULL))
        return -1;

    if (dataobj != NULL) {
        if (!make_dbt(dataobj, &data))
            retval =  -1;
        else {
            if (self->setflags & (DB_DUP|DB_DUPSORT))
                /* dictionaries shouldn't have duplicate keys */
                flags = DB_NOOVERWRITE;
            retval = _DB_put(self, NULL, &key, &data, flags);

            if ((retval == -1) &&  (self->setflags & (DB_DUP|DB_DUPSORT))) {
                /* try deleting any old record that matches and then PUT it
                 * again... */
                _DB_delete(self, NULL, &key, 0);
                PyErr_Clear();
                retval = _DB_put(self, NULL, &key, &data, flags);
            }
        }
    }
    else {
        /* dataobj == NULL, so delete the key */
        retval = _DB_delete(self, NULL, &key, 0);
    }
    FREE_DBT(key);
    return retval;
}


static PyObject*
_DB_has_key(DBObject* self, PyObject* keyobj, PyObject* txnobj)
{
    int err;
    DBT key;
    DB_TXN *txn = NULL;

    CHECK_DB_NOT_CLOSED(self);
    if (!make_key_dbt(self, keyobj, &key, NULL))
        return NULL;
    if (!checkTxnObj(txnobj, &txn)) {
        FREE_DBT(key);
        return NULL;
    }

#if (DBVER < 46)
    /* This causes DB_BUFFER_SMALL to be returned when the db has the key because
       it has a record but can't allocate a buffer for the data.  This saves
       having to deal with data we won't be using.
     */
    {
        DBT data ;
        CLEAR_DBT(data);
        data.flags = DB_DBT_USERMEM;

        MYDB_BEGIN_ALLOW_THREADS;
        err = self->db->get(self->db, txn, &key, &data, 0);
        MYDB_END_ALLOW_THREADS;
    }
#else
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->exists(self->db, txn, &key, 0);
    MYDB_END_ALLOW_THREADS;
#endif

    FREE_DBT(key);

    /*
    ** DB_BUFFER_SMALL is only used if we use "get".
    ** We can drop it when we only use "exists",
    ** when we drop suport for Berkeley DB < 4.6.
    */
    if (err == DB_BUFFER_SMALL || err == 0) {
        Py_INCREF(Py_True);
        return Py_True;
    } else if (err == DB_NOTFOUND || err == DB_KEYEMPTY) {
        Py_INCREF(Py_False);
        return Py_False;
    }

    makeDBError(err);
    return NULL;
}

static PyObject*
DB_has_key(DBObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* keyobj;
    PyObject* txnobj = NULL;
    static char* kwnames[] = {"key","txn", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:has_key", kwnames,
                &keyobj, &txnobj))
        return NULL;

    return _DB_has_key(self, keyobj, txnobj);
}


static int DB_contains(DBObject* self, PyObject* keyobj)
{
    PyObject* result;
    int result2 = 0;

    result = _DB_has_key(self, keyobj, NULL) ;
    if (result == NULL) {
        return -1; /* Propague exception */
    }
    if (result != Py_False) {
        result2 = 1;
    }

    Py_DECREF(result);
    return result2;
}


#define _KEYS_LIST      1
#define _VALUES_LIST    2
#define _ITEMS_LIST     3

static PyObject*
_DB_make_list(DBObject* self, DB_TXN* txn, int type)
{
    int err, dbtype;
    DBT key;
    DBT data;
    DBC *cursor;
    PyObject* list;
    PyObject* item = NULL;

    CHECK_DB_NOT_CLOSED(self);
    CLEAR_DBT(key);
    CLEAR_DBT(data);

    dbtype = _DB_get_type(self);
    if (dbtype == -1)
        return NULL;

    list = PyList_New(0);
    if (list == NULL)
        return NULL;

    /* get a cursor */
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db->cursor(self->db, txn, &cursor, 0);
    MYDB_END_ALLOW_THREADS;
    if (makeDBError(err)) {
        Py_DECREF(list);
        return NULL;
    }

    while (1) { /* use the cursor to traverse the DB, collecting items */
        MYDB_BEGIN_ALLOW_THREADS;
        err = _DBC_get(cursor, &key, &data, DB_NEXT);
        MYDB_END_ALLOW_THREADS;

        if (err) {
            /* for any error, break out of the loop */
            break;
        }

        switch (type) {
        case _KEYS_LIST:
            switch(dbtype) {
            case DB_BTREE:
            case DB_HASH:
            default:
                item = Build_PyString(key.data, key.size);
                break;
            case DB_RECNO:
            case DB_QUEUE:
                item = NUMBER_FromLong(*((db_recno_t*)key.data));
                break;
            }
            break;

        case _VALUES_LIST:
            item = Build_PyString(data.data, data.size);
            break;

        case _ITEMS_LIST:
            switch(dbtype) {
            case DB_BTREE:
            case DB_HASH:
            default:
                item = BuildValue_SS(key.data, key.size, data.data, data.size);
                break;
            case DB_RECNO:
            case DB_QUEUE:
                item = BuildValue_IS(*((db_recno_t*)key.data), data.data, data.size);
                break;
            }
            break;
        default:
            PyErr_Format(PyExc_ValueError, "Unknown key type 0x%x", type);
            item = NULL;
            break;
        }
        if (item == NULL) {
            Py_DECREF(list);
            list = NULL;
            goto done;
        }
        if (PyList_Append(list, item)) {
            Py_DECREF(list);
            Py_DECREF(item);
            list = NULL;
            goto done;
        }
        Py_DECREF(item);
    }

    /* DB_NOTFOUND || DB_KEYEMPTY is okay, it means we got to the end */
    if (err != DB_NOTFOUND && err != DB_KEYEMPTY && makeDBError(err)) {
        Py_DECREF(list);
        list = NULL;
    }

 done:
    MYDB_BEGIN_ALLOW_THREADS;
    _DBC_close(cursor);
    MYDB_END_ALLOW_THREADS;
    return list;
}


static PyObject*
DB_keys(DBObject* self, PyObject* args)
{
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;

    if (!PyArg_UnpackTuple(args, "keys", 0, 1, &txnobj))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    return _DB_make_list(self, txn, _KEYS_LIST);
}


static PyObject*
DB_items(DBObject* self, PyObject* args)
{
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;

    if (!PyArg_UnpackTuple(args, "items", 0, 1, &txnobj))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    return _DB_make_list(self, txn, _ITEMS_LIST);
}


static PyObject*
DB_values(DBObject* self, PyObject* args)
{
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;

    if (!PyArg_UnpackTuple(args, "values", 0, 1, &txnobj))
        return NULL;
    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    return _DB_make_list(self, txn, _VALUES_LIST);
}

/* --------------------------------------------------------------------- */
/* DBLogCursor methods */


static PyObject*
DBLogCursor_close_internal(DBLogCursorObject* self)
{
    int err = 0;

    if (self->logc != NULL) {
        EXTRACT_FROM_DOUBLE_LINKED_LIST(self);

        MYDB_BEGIN_ALLOW_THREADS;
        err = self->logc->close(self->logc, 0);
        MYDB_END_ALLOW_THREADS;
        self->logc = NULL;
    }
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBLogCursor_close(DBLogCursorObject* self)
{
    return DBLogCursor_close_internal(self);
}


static PyObject*
_DBLogCursor_get(DBLogCursorObject* self, int flag, DB_LSN *lsn2)
{
    int err;
    DBT data;
    DB_LSN lsn = {0, 0};
    PyObject *dummy, *retval;

    CLEAR_DBT(data);
    data.flags = DB_DBT_MALLOC; /* Berkeley DB must do the malloc */

    CHECK_LOGCURSOR_NOT_CLOSED(self);

    if (lsn2)
        lsn = *lsn2;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->logc->get(self->logc, &lsn, &data, flag);
    MYDB_END_ALLOW_THREADS;

    if (err == DB_NOTFOUND) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        retval = dummy = BuildValue_S(data.data, data.size);
        if (dummy) {
            retval = Py_BuildValue("(ii)O", lsn.file, lsn.offset, dummy);
            Py_DECREF(dummy);
        }
    }

    FREE_DBT(data);
    return retval;
}

static PyObject*
DBLogCursor_current(DBLogCursorObject* self)
{
    return _DBLogCursor_get(self, DB_CURRENT, NULL);
}

static PyObject*
DBLogCursor_first(DBLogCursorObject* self)
{
    return _DBLogCursor_get(self, DB_FIRST, NULL);
}

static PyObject*
DBLogCursor_last(DBLogCursorObject* self)
{
    return _DBLogCursor_get(self, DB_LAST, NULL);
}

static PyObject*
DBLogCursor_next(DBLogCursorObject* self)
{
    return _DBLogCursor_get(self, DB_NEXT, NULL);
}

static PyObject*
DBLogCursor_prev(DBLogCursorObject* self)
{
    return _DBLogCursor_get(self, DB_PREV, NULL);
}

static PyObject*
DBLogCursor_set(DBLogCursorObject* self, PyObject* args)
{
    DB_LSN lsn;

    if (!PyArg_ParseTuple(args, "(ii):set", &lsn.file, &lsn.offset))
        return NULL;

    return _DBLogCursor_get(self, DB_SET, &lsn);
}


/* --------------------------------------------------------------------- */
/* DBSite methods */


#if (DBVER >= 52)
static PyObject*
DBSite_close_internal(DBSiteObject* self)
{
    int err = 0;

    if (self->site != NULL) {
        EXTRACT_FROM_DOUBLE_LINKED_LIST(self);

        MYDB_BEGIN_ALLOW_THREADS;
        err = self->site->close(self->site);
        MYDB_END_ALLOW_THREADS;
        self->site = NULL;
    }
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBSite_close(DBSiteObject* self)
{
    return DBSite_close_internal(self);
}

static PyObject*
DBSite_remove(DBSiteObject* self)
{
    int err = 0;

    CHECK_SITE_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->site->remove(self->site);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBSite_get_eid(DBSiteObject* self)
{
    int err = 0;
    int eid;

    CHECK_SITE_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->site->get_eid(self->site, &eid);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    return NUMBER_FromLong(eid);
}

static PyObject*
DBSite_get_address(DBSiteObject* self)
{
    int err = 0;
    const char *host;
    u_int port;

    CHECK_SITE_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->site->get_address(self->site, &host, &port);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return Py_BuildValue("(sI)", host, port);
}

static PyObject*
DBSite_get_config(DBSiteObject* self, PyObject* args, PyObject* kwargs)
{
    int err = 0;
    u_int32_t which, value;
    static char* kwnames[] = { "which", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:get_config", kwnames,
                                     &which))
        return NULL;

    CHECK_SITE_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->site->get_config(self->site, which, &value);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    if (value) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static PyObject*
DBSite_set_config(DBSiteObject* self, PyObject* args, PyObject* kwargs)
{
    int err = 0;
    u_int32_t which, value;
    PyObject *valueO;
    static char* kwnames[] = { "which", "value", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO:set_config", kwnames,
                                     &which, &valueO))
        return NULL;

    CHECK_SITE_NOT_CLOSED(self);

    value = PyObject_IsTrue(valueO);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->site->set_config(self->site, which, value);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif


/* --------------------------------------------------------------------- */
/* DBCursor methods */


static PyObject*
DBC_close_internal(DBCursorObject* self)
{
    int err = 0;

    if (self->dbc != NULL) {
        EXTRACT_FROM_DOUBLE_LINKED_LIST(self);
        if (self->txn) {
            EXTRACT_FROM_DOUBLE_LINKED_LIST_TXN(self);
            self->txn=NULL;
        }

        MYDB_BEGIN_ALLOW_THREADS;
        err = _DBC_close(self->dbc);
        MYDB_END_ALLOW_THREADS;
        self->dbc = NULL;
    }
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBC_close(DBCursorObject* self)
{
    return DBC_close_internal(self);
}


static PyObject*
DBC_count(DBCursorObject* self, PyObject* args)
{
    int err = 0;
    db_recno_t count;
    int flags = 0;

    if (!PyArg_ParseTuple(args, "|i:count", &flags))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_count(self->dbc, &count, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    return NUMBER_FromLong(count);
}


static PyObject*
DBC_current(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_CURRENT,args,kwargs,"|iii:current");
}


static PyObject*
DBC_delete(DBCursorObject* self, PyObject* args)
{
    int err, flags=0;

    if (!PyArg_ParseTuple(args, "|i:delete", &flags))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_del(self->dbc, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    RETURN_NONE();
}


static PyObject*
DBC_dup(DBCursorObject* self, PyObject* args)
{
    int err, flags =0;
    DBC* dbc = NULL;

    if (!PyArg_ParseTuple(args, "|i:dup", &flags))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_dup(self->dbc, &dbc, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    return (PyObject*) newDBCursorObject(dbc, self->txn, self->mydb);
}

static PyObject*
DBC_first(DBCursorObject* self, PyObject* args, PyObject* kwargs)
{
    return _DBCursor_get(self,DB_FIRST,args,kwargs,"|iii:first");
}


static PyObject*
DBC_get(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    int err, flags=0;
    PyObject* keyobj = NULL;
    PyObject* dataobj = NULL;
    PyObject* retval = NULL;
    int dlen = -1;
    int doff = -1;
    DBT key, data;
    static char* kwnames[] = { "key","data", "flags", "dlen", "doff",
                                     NULL };

    CLEAR_DBT(key);
    CLEAR_DBT(data);
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|ii:get", &kwnames[2],
                                     &flags, &dlen, &doff))
    {
        PyErr_Clear();
        if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|ii:get",
                                         &kwnames[1],
                                         &keyobj, &flags, &dlen, &doff))
        {
            PyErr_Clear();
            if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOi|ii:get",
                                             kwnames, &keyobj, &dataobj,
                                             &flags, &dlen, &doff))
            {
                return NULL;
            }
        }
    }

    CHECK_CURSOR_NOT_CLOSED(self);

    if (keyobj && !make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;
    if ( (dataobj && !make_dbt(dataobj, &data)) ||
         (!add_partial_dbt(&data, dlen, doff)) )
    {
        FREE_DBT(key); /* 'make_key_dbt' could do a 'malloc' */
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_get(self->dbc, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
            && self->mydb->moduleFlags.getReturnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;
        case DB_BTREE:
        case DB_HASH:
        default:
            retval = BuildValue_SS(key.data, key.size, data.data, data.size);
            break;
        case DB_RECNO:
        case DB_QUEUE:
            retval = BuildValue_IS(*((db_recno_t*)key.data), data.data, data.size);
            break;
        }
    }
    FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
    return retval;
}

static PyObject*
DBC_pget(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    int err, flags=0;
    PyObject* keyobj = NULL;
    PyObject* dataobj = NULL;
    PyObject* retval = NULL;
    int dlen = -1;
    int doff = -1;
    DBT key, pkey, data;
    static char* kwnames_keyOnly[] = { "key", "flags", "dlen", "doff", NULL };
    static char* kwnames[] = { "key", "data", "flags", "dlen", "doff", NULL };

    CLEAR_DBT(key);
    CLEAR_DBT(data);
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|ii:pget", &kwnames[2],
                                     &flags, &dlen, &doff))
    {
        PyErr_Clear();
        if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|ii:pget",
                                         kwnames_keyOnly,
                                         &keyobj, &flags, &dlen, &doff))
        {
            PyErr_Clear();
            if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOi|ii:pget",
                                             kwnames, &keyobj, &dataobj,
                                             &flags, &dlen, &doff))
            {
                return NULL;
            }
        }
    }

    CHECK_CURSOR_NOT_CLOSED(self);

    if (keyobj && !make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;
    if ( (dataobj && !make_dbt(dataobj, &data)) ||
         (!add_partial_dbt(&data, dlen, doff)) ) {
        FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
        return NULL;
    }

    CLEAR_DBT(pkey);
    pkey.flags = DB_DBT_MALLOC;

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_pget(self->dbc, &key, &pkey, &data, flags);
    MYDB_END_ALLOW_THREADS;

    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
            && self->mydb->moduleFlags.getReturnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        PyObject *pkeyObj;
        PyObject *dataObj;
        dataObj = Build_PyString(data.data, data.size);

        if (self->mydb->primaryDBType == DB_RECNO ||
            self->mydb->primaryDBType == DB_QUEUE)
            pkeyObj = NUMBER_FromLong(*(int *)pkey.data);
        else
            pkeyObj = Build_PyString(pkey.data, pkey.size);

        if (key.data && key.size) /* return key, pkey and data */
        {
            PyObject *keyObj;
            int type = _DB_get_type(self->mydb);
            if (type == DB_RECNO || type == DB_QUEUE)
                keyObj = NUMBER_FromLong(*(int *)key.data);
            else
                keyObj = Build_PyString(key.data, key.size);
            retval = PyTuple_Pack(3, keyObj, pkeyObj, dataObj);
            Py_DECREF(keyObj);
            FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
        }
        else /* return just the pkey and data */
        {
            retval = PyTuple_Pack(2, pkeyObj, dataObj);
        }
        Py_DECREF(dataObj);
        Py_DECREF(pkeyObj);
        FREE_DBT(pkey);
    }
    /* the only time REALLOC should be set is if we used an integer
     * key that make_key_dbt malloc'd for us.  always free these. */
    if (key.flags & DB_DBT_REALLOC) {  /* 'make_key_dbt' could do a 'malloc' */
        FREE_DBT(key);
    }
    return retval;
}


static PyObject*
DBC_get_recno(DBCursorObject* self)
{
    int err;
    db_recno_t recno;
    DBT key;
    DBT data;

    CHECK_CURSOR_NOT_CLOSED(self);

    CLEAR_DBT(key);
    CLEAR_DBT(data);

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_get(self->dbc, &key, &data, DB_GET_RECNO);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    recno = *((db_recno_t*)data.data);
    return NUMBER_FromLong(recno);
}


static PyObject*
DBC_last(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_LAST,args,kwargs,"|iii:last");
}


static PyObject*
DBC_next(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_NEXT,args,kwargs,"|iii:next");
}


static PyObject*
DBC_prev(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_PREV,args,kwargs,"|iii:prev");
}


static PyObject*
DBC_put(DBCursorObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags = 0;
    PyObject* keyobj, *dataobj;
    DBT key, data;
    static char* kwnames[] = { "key", "data", "flags", "dlen", "doff",
                                     NULL };
    int dlen = -1;
    int doff = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|iii:put", kwnames,
                                     &keyobj, &dataobj, &flags, &dlen, &doff))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    if (!make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;
    if (!make_dbt(dataobj, &data) ||
        !add_partial_dbt(&data, dlen, doff) )
    {
        FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_put(self->dbc, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;
    FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBC_set(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    int err, flags = 0;
    DBT key, data;
    PyObject* retval, *keyobj;
    static char* kwnames[] = { "key", "flags", "dlen", "doff", NULL };
    int dlen = -1;
    int doff = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iii:set", kwnames,
                                     &keyobj, &flags, &dlen, &doff))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    if (!make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;

    CLEAR_DBT(data);
    if (!add_partial_dbt(&data, dlen, doff)) {
        FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_get(self->dbc, &key, &data, flags|DB_SET);
    MYDB_END_ALLOW_THREADS;
    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
            && self->mydb->moduleFlags.cursorSetReturnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;
        case DB_BTREE:
        case DB_HASH:
        default:
            retval = BuildValue_SS(key.data, key.size, data.data, data.size);
            break;
        case DB_RECNO:
        case DB_QUEUE:
            retval = BuildValue_IS(*((db_recno_t*)key.data), data.data, data.size);
            break;
        }
        FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
    }
    /* the only time REALLOC should be set is if we used an integer
     * key that make_key_dbt malloc'd for us.  always free these. */
    if (key.flags & DB_DBT_REALLOC) {
        FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
    }

    return retval;
}


static PyObject*
DBC_set_range(DBCursorObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags = 0;
    DBT key, data;
    PyObject* retval, *keyobj;
    static char* kwnames[] = { "key", "flags", "dlen", "doff", NULL };
    int dlen = -1;
    int doff = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iii:set_range", kwnames,
                                     &keyobj, &flags, &dlen, &doff))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    if (!make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;

    CLEAR_DBT(data);
    if (!add_partial_dbt(&data, dlen, doff)) {
        FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
        return NULL;
    }
    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_get(self->dbc, &key, &data, flags|DB_SET_RANGE);
    MYDB_END_ALLOW_THREADS;
    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
            && self->mydb->moduleFlags.cursorSetReturnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;
        case DB_BTREE:
        case DB_HASH:
        default:
            retval = BuildValue_SS(key.data, key.size, data.data, data.size);
            break;
        case DB_RECNO:
        case DB_QUEUE:
            retval = BuildValue_IS(*((db_recno_t*)key.data), data.data, data.size);
            break;
        }
        FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
    }
    /* the only time REALLOC should be set is if we used an integer
     * key that make_key_dbt malloc'd for us.  always free these. */
    if (key.flags & DB_DBT_REALLOC) {
        FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
    }

    return retval;
}

static PyObject*
_DBC_get_set_both(DBCursorObject* self, PyObject* keyobj, PyObject* dataobj,
                  int flags, unsigned int returnsNone)
{
    int err;
    DBT key, data;
    PyObject* retval;

    /* the caller did this:  CHECK_CURSOR_NOT_CLOSED(self); */
    if (!make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;
    if (!make_dbt(dataobj, &data)) {
        FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_get(self->dbc, &key, &data, flags|DB_GET_BOTH);
    MYDB_END_ALLOW_THREADS;
    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY) && returnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        switch (_DB_get_type(self->mydb)) {
        case -1:
            retval = NULL;
            break;
        case DB_BTREE:
        case DB_HASH:
        default:
            retval = BuildValue_SS(key.data, key.size, data.data, data.size);
            break;
        case DB_RECNO:
        case DB_QUEUE:
            retval = BuildValue_IS(*((db_recno_t*)key.data), data.data, data.size);
            break;
        }
    }

    FREE_DBT(key);  /* 'make_key_dbt' could do a 'malloc' */
    return retval;
}

static PyObject*
DBC_get_both(DBCursorObject* self, PyObject* args)
{
    int flags=0;
    PyObject *keyobj, *dataobj;

    if (!PyArg_ParseTuple(args, "OO|i:get_both", &keyobj, &dataobj, &flags))
        return NULL;

    /* if the cursor is closed, self->mydb may be invalid */
    CHECK_CURSOR_NOT_CLOSED(self);

    return _DBC_get_set_both(self, keyobj, dataobj, flags,
                self->mydb->moduleFlags.getReturnsNone);
}

/* Return size of entry */
static PyObject*
DBC_get_current_size(DBCursorObject* self)
{
    int err, flags=DB_CURRENT;
    PyObject* retval = NULL;
    DBT key, data;

    CHECK_CURSOR_NOT_CLOSED(self);
    CLEAR_DBT(key);
    CLEAR_DBT(data);

    /* We don't allocate any memory, forcing a DB_BUFFER_SMALL error and thus
       getting the record size. */
    data.flags = DB_DBT_USERMEM;
    data.ulen = 0;
    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_get(self->dbc, &key, &data, flags);
    MYDB_END_ALLOW_THREADS;
    if (err == DB_BUFFER_SMALL || !err) {
        /* DB_BUFFER_SMALL means positive size, !err means zero length value */
        retval = NUMBER_FromLong((long)data.size);
        err = 0;
    }

    RETURN_IF_ERR();
    return retval;
}

static PyObject*
DBC_set_both(DBCursorObject* self, PyObject* args)
{
    int flags=0;
    PyObject *keyobj, *dataobj;

    if (!PyArg_ParseTuple(args, "OO|i:set_both", &keyobj, &dataobj, &flags))
        return NULL;

    /* if the cursor is closed, self->mydb may be invalid */
    CHECK_CURSOR_NOT_CLOSED(self);

    return _DBC_get_set_both(self, keyobj, dataobj, flags,
                self->mydb->moduleFlags.cursorSetReturnsNone);
}


static PyObject*
DBC_set_recno(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    int err, irecno, flags=0;
    db_recno_t recno;
    DBT key, data;
    PyObject* retval;
    int dlen = -1;
    int doff = -1;
    static char* kwnames[] = { "recno","flags", "dlen", "doff", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|iii:set_recno", kwnames,
                                     &irecno, &flags, &dlen, &doff))
      return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    CLEAR_DBT(key);
    recno = (db_recno_t) irecno;
    /* use allocated space so DB will be able to realloc room for the real
     * key */
    key.data = malloc(sizeof(db_recno_t));
    if (key.data == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Key memory allocation failed");
        return NULL;
    }
    key.size = sizeof(db_recno_t);
    key.ulen = key.size;
    memcpy(key.data, &recno, sizeof(db_recno_t));
    key.flags = DB_DBT_REALLOC;

    CLEAR_DBT(data);
    if (!add_partial_dbt(&data, dlen, doff)) {
        FREE_DBT(key);
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_get(self->dbc, &key, &data, flags|DB_SET_RECNO);
    MYDB_END_ALLOW_THREADS;
    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
            && self->mydb->moduleFlags.cursorSetReturnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {  /* Can only be used for BTrees, so no need to return int key */
        retval = BuildValue_SS(key.data, key.size, data.data, data.size);
    }
    FREE_DBT(key);

    return retval;
}


static PyObject*
DBC_consume(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_CONSUME,args,kwargs,"|iii:consume");
}


static PyObject*
DBC_next_dup(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_NEXT_DUP,args,kwargs,"|iii:next_dup");
}


static PyObject*
DBC_next_nodup(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_NEXT_NODUP,args,kwargs,"|iii:next_nodup");
}

#if (DBVER >= 46)
static PyObject*
DBC_prev_dup(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_PREV_DUP,args,kwargs,"|iii:prev_dup");
}
#endif

static PyObject*
DBC_prev_nodup(DBCursorObject* self, PyObject* args, PyObject *kwargs)
{
    return _DBCursor_get(self,DB_PREV_NODUP,args,kwargs,"|iii:prev_nodup");
}


static PyObject*
DBC_join_item(DBCursorObject* self, PyObject* args)
{
    int err, flags=0;
    DBT key, data;
    PyObject* retval;

    if (!PyArg_ParseTuple(args, "|i:join_item", &flags))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    CLEAR_DBT(key);
    CLEAR_DBT(data);

    MYDB_BEGIN_ALLOW_THREADS;
    err = _DBC_get(self->dbc, &key, &data, flags | DB_JOIN_ITEM);
    MYDB_END_ALLOW_THREADS;
    if ((err == DB_NOTFOUND || err == DB_KEYEMPTY)
            && self->mydb->moduleFlags.getReturnsNone) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (makeDBError(err)) {
        retval = NULL;
    }
    else {
        retval = BuildValue_S(key.data, key.size);
    }

    return retval;
}


#if (DBVER >= 46)
static PyObject*
DBC_set_priority(DBCursorObject* self, PyObject* args, PyObject* kwargs)
{
    int err, priority;
    static char* kwnames[] = { "priority", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:set_priority", kwnames,
                                     &priority))
        return NULL;

    CHECK_CURSOR_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->set_priority(self->dbc, priority);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBC_get_priority(DBCursorObject* self)
{
    int err;
    DB_CACHE_PRIORITY priority;

    CHECK_CURSOR_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->dbc->get_priority(self->dbc, &priority);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(priority);
}
#endif



/* --------------------------------------------------------------------- */
/* DBEnv methods */


static PyObject*
DBEnv_close_internal(DBEnvObject* self, int flags)
{
    PyObject *dummy;
    int err;

    if (!self->closed) {      /* Don't close more than once */
        while(self->children_txns) {
            dummy = DBTxn_abort_discard_internal(self->children_txns, 0);
            Py_XDECREF(dummy);
        }
        while(self->children_dbs) {
            dummy = DB_close_internal(self->children_dbs, 0, 0);
            Py_XDECREF(dummy);
        }
        while(self->children_logcursors) {
            dummy = DBLogCursor_close_internal(self->children_logcursors);
            Py_XDECREF(dummy);
        }
#if (DBVER >= 52)
        while(self->children_sites) {
            dummy = DBSite_close_internal(self->children_sites);
            Py_XDECREF(dummy);
        }
#endif
    }

    self->closed = 1;
    if (self->db_env) {
        MYDB_BEGIN_ALLOW_THREADS;
        err = self->db_env->close(self->db_env, flags);
        MYDB_END_ALLOW_THREADS;
        /* after calling DBEnv->close, regardless of error, this DBEnv
         * may not be accessed again (Berkeley DB docs). */
        self->db_env = NULL;
        RETURN_IF_ERR();
    }
    RETURN_NONE();
}

static PyObject*
DBEnv_close(DBEnvObject* self, PyObject* args)
{
    int flags = 0;

    if (!PyArg_ParseTuple(args, "|i:close", &flags))
        return NULL;
    return DBEnv_close_internal(self, flags);
}


static PyObject*
DBEnv_open(DBEnvObject* self, PyObject* args)
{
    int err, flags=0, mode=0660;
    char *db_home;

    if (!PyArg_ParseTuple(args, "z|ii:open", &db_home, &flags, &mode))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->open(self->db_env, db_home, flags, mode);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    self->closed = 0;
    self->flags = flags;
    RETURN_NONE();
}


static PyObject*
DBEnv_memp_stat(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    DB_MPOOL_STAT *gsp;
    DB_MPOOL_FSTAT **fsp, **fsp2;
    PyObject* d = NULL, *d2, *d3, *r;
    u_int32_t flags = 0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:memp_stat",
                kwnames, &flags))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->memp_stat(self->db_env, &gsp, &fsp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    /* Turn the stat structure into a dictionary */
    d = PyDict_New();
    if (d == NULL) {
        if (gsp)
            free(gsp);
        return NULL;
    }

#define MAKE_ENTRY(name)  _addIntToDict(d, #name, gsp->st_##name)

    MAKE_ENTRY(gbytes);
    MAKE_ENTRY(bytes);
    MAKE_ENTRY(ncache);
#if (DBVER >= 46)
    MAKE_ENTRY(max_ncache);
#endif
    MAKE_ENTRY(regsize);
    MAKE_ENTRY(mmapsize);
    MAKE_ENTRY(maxopenfd);
    MAKE_ENTRY(maxwrite);
    MAKE_ENTRY(maxwrite_sleep);
    MAKE_ENTRY(map);
    MAKE_ENTRY(cache_hit);
    MAKE_ENTRY(cache_miss);
    MAKE_ENTRY(page_create);
    MAKE_ENTRY(page_in);
    MAKE_ENTRY(page_out);
    MAKE_ENTRY(ro_evict);
    MAKE_ENTRY(rw_evict);
    MAKE_ENTRY(page_trickle);
    MAKE_ENTRY(pages);
    MAKE_ENTRY(page_clean);
    MAKE_ENTRY(page_dirty);
    MAKE_ENTRY(hash_buckets);
    MAKE_ENTRY(hash_searches);
    MAKE_ENTRY(hash_longest);
    MAKE_ENTRY(hash_examined);
    MAKE_ENTRY(hash_nowait);
    MAKE_ENTRY(hash_wait);
#if (DBVER >= 45)
    MAKE_ENTRY(hash_max_nowait);
#endif
    MAKE_ENTRY(hash_max_wait);
    MAKE_ENTRY(region_wait);
    MAKE_ENTRY(region_nowait);
#if (DBVER >= 45)
    MAKE_ENTRY(mvcc_frozen);
    MAKE_ENTRY(mvcc_thawed);
    MAKE_ENTRY(mvcc_freed);
#endif
    MAKE_ENTRY(alloc);
    MAKE_ENTRY(alloc_buckets);
    MAKE_ENTRY(alloc_max_buckets);
    MAKE_ENTRY(alloc_pages);
    MAKE_ENTRY(alloc_max_pages);
#if (DBVER >= 45)
    MAKE_ENTRY(io_wait);
#endif
#if (DBVER >= 48)
    MAKE_ENTRY(sync_interrupted);
#endif

#undef MAKE_ENTRY
    free(gsp);

    d2 = PyDict_New();
    if (d2 == NULL) {
        Py_DECREF(d);
        if (fsp)
            free(fsp);
        return NULL;
    }
#define MAKE_ENTRY(name)  _addIntToDict(d3, #name, (*fsp2)->st_##name)
    for(fsp2=fsp;*fsp2; fsp2++) {
        d3 = PyDict_New();
        if (d3 == NULL) {
            Py_DECREF(d);
            Py_DECREF(d2);
            if (fsp)
                free(fsp);
            return NULL;
        }
        MAKE_ENTRY(pagesize);
        MAKE_ENTRY(cache_hit);
        MAKE_ENTRY(cache_miss);
        MAKE_ENTRY(map);
        MAKE_ENTRY(page_create);
        MAKE_ENTRY(page_in);
        MAKE_ENTRY(page_out);
        if(PyDict_SetItemString(d2, (*fsp2)->file_name, d3)) {
            Py_DECREF(d);
            Py_DECREF(d2);
            Py_DECREF(d3);
            if (fsp)
                free(fsp);
            return NULL;
        }
        Py_DECREF(d3);
    }

#undef MAKE_ENTRY
    free(fsp);

    r = PyTuple_Pack(2, d, d2);
    Py_DECREF(d);
    Py_DECREF(d2);
    return r;
}

static PyObject*
DBEnv_memp_stat_print(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:memp_stat_print",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->memp_stat_print(self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_memp_trickle(DBEnvObject* self, PyObject* args)
{
    int err, percent, nwrotep;

    if (!PyArg_ParseTuple(args, "i:memp_trickle", &percent))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->memp_trickle(self->db_env, percent, &nwrotep);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(nwrotep);
}

static PyObject*
DBEnv_memp_sync(DBEnvObject* self, PyObject* args)
{
    int err;
    DB_LSN lsn = {0, 0};
    DB_LSN *lsn_p = NULL;

    if (!PyArg_ParseTuple(args, "|(ii):memp_sync", &lsn.file, &lsn.offset))
        return NULL;
    if ((lsn.file!=0) || (lsn.offset!=0)) {
        lsn_p = &lsn;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->memp_sync(self->db_env, lsn_p);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_remove(DBEnvObject* self, PyObject* args)
{
    int err, flags=0;
    char *db_home;

    if (!PyArg_ParseTuple(args, "s|i:remove", &db_home, &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->remove(self->db_env, db_home, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_dbremove(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    u_int32_t flags=0;
    char *file = NULL;
    char *database = NULL;
    PyObject *txnobj = NULL;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "file", "database", "txn", "flags",
                                     NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|zOi:dbremove", kwnames,
                &file, &database, &txnobj, &flags)) {
        return NULL;
    }
    if (!checkTxnObj(txnobj, &txn)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->dbremove(self->db_env, txn, file, database, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_dbrename(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    u_int32_t flags=0;
    char *file = NULL;
    char *database = NULL;
    char *newname = NULL;
    PyObject *txnobj = NULL;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "file", "database", "newname", "txn",
                                     "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "szs|Oi:dbrename", kwnames,
                &file, &database, &newname, &txnobj, &flags)) {
        return NULL;
    }
    if (!checkTxnObj(txnobj, &txn)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->dbrename(self->db_env, txn, file, database, newname,
                                 flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}



static PyObject*
DBEnv_set_encrypt(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    u_int32_t flags=0;
    char *passwd = NULL;
    static char* kwnames[] = { "passwd", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i:set_encrypt", kwnames,
                &passwd, &flags)) {
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_encrypt(self->db_env, passwd, flags);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_encrypt_flags(DBEnvObject* self)
{
    int err;
    u_int32_t flags;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_encrypt_flags(self->db_env, &flags);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return NUMBER_FromLong(flags);
}

static PyObject*
DBEnv_get_timeout(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    int flag;
    u_int32_t timeout;
    static char* kwnames[] = {"flag", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:get_timeout", kwnames,
                &flag)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_timeout(self->db_env, &timeout, flag);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(timeout);
}


static PyObject*
DBEnv_set_timeout(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    u_int32_t flags=0;
    u_int32_t timeout = 0;
    static char* kwnames[] = { "timeout", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii:set_timeout", kwnames,
                &timeout, &flags)) {
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_timeout(self->db_env, (db_timeout_t)timeout, flags);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_set_shm_key(DBEnvObject* self, PyObject* args)
{
    int err;
    long shm_key = 0;

    if (!PyArg_ParseTuple(args, "l:set_shm_key", &shm_key))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    err = self->db_env->set_shm_key(self->db_env, shm_key);
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_shm_key(DBEnvObject* self)
{
    int err;
    long shm_key;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_shm_key(self->db_env, &shm_key);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return NUMBER_FromLong(shm_key);
}

#if (DBVER >= 46)
static PyObject*
DBEnv_set_cache_max(DBEnvObject* self, PyObject* args)
{
    int err, gbytes, bytes;

    if (!PyArg_ParseTuple(args, "ii:set_cache_max",
                          &gbytes, &bytes))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_cache_max(self->db_env, gbytes, bytes);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_cache_max(DBEnvObject* self)
{
    int err;
    u_int32_t gbytes, bytes;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_cache_max(self->db_env, &gbytes, &bytes);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return Py_BuildValue("(ii)", gbytes, bytes);
}
#endif

#if (DBVER >= 46)
static PyObject*
DBEnv_set_thread_count(DBEnvObject* self, PyObject* args)
{
    int err;
    u_int32_t count;

    if (!PyArg_ParseTuple(args, "i:set_thread_count", &count))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_thread_count(self->db_env, count);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_thread_count(DBEnvObject* self)
{
    int err;
    u_int32_t count;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_thread_count(self->db_env, &count);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(count);
}
#endif

static PyObject*
DBEnv_set_cachesize(DBEnvObject* self, PyObject* args)
{
    int err, gbytes=0, bytes=0, ncache=0;

    if (!PyArg_ParseTuple(args, "ii|i:set_cachesize",
                          &gbytes, &bytes, &ncache))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_cachesize(self->db_env, gbytes, bytes, ncache);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_cachesize(DBEnvObject* self)
{
    int err;
    u_int32_t gbytes, bytes;
    int ncache;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_cachesize(self->db_env, &gbytes, &bytes, &ncache);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return Py_BuildValue("(iii)", gbytes, bytes, ncache);
}


static PyObject*
DBEnv_set_flags(DBEnvObject* self, PyObject* args)
{
    int err, flags=0, onoff=0;

    if (!PyArg_ParseTuple(args, "ii:set_flags",
                          &flags, &onoff))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_flags(self->db_env, flags, onoff);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_flags(DBEnvObject* self)
{
    int err;
    u_int32_t flags;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_flags(self->db_env, &flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(flags);
}

#if (DBVER >= 47)
static PyObject*
DBEnv_log_set_config(DBEnvObject* self, PyObject* args)
{
    int err, flags, onoff;

    if (!PyArg_ParseTuple(args, "ii:log_set_config",
                          &flags, &onoff))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->log_set_config(self->db_env, flags, onoff);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_log_get_config(DBEnvObject* self, PyObject* args)
{
    int err, flag, onoff;

    if (!PyArg_ParseTuple(args, "i:log_get_config", &flag))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->log_get_config(self->db_env, flag, &onoff);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return PyBool_FromLong(onoff);
}
#endif /* DBVER >= 47 */

#if (DBVER >= 44)
static PyObject*
DBEnv_mutex_set_max(DBEnvObject* self, PyObject* args)
{
    int err;
    int value;

    if (!PyArg_ParseTuple(args, "i:mutex_set_max", &value))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_set_max(self->db_env, value);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_mutex_get_max(DBEnvObject* self)
{
    int err;
    u_int32_t value;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_get_max(self->db_env, &value);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return NUMBER_FromLong(value);
}

static PyObject*
DBEnv_mutex_set_align(DBEnvObject* self, PyObject* args)
{
    int err;
    int align;

    if (!PyArg_ParseTuple(args, "i:mutex_set_align", &align))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_set_align(self->db_env, align);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_mutex_get_align(DBEnvObject* self)
{
    int err;
    u_int32_t align;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_get_align(self->db_env, &align);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return NUMBER_FromLong(align);
}

static PyObject*
DBEnv_mutex_set_increment(DBEnvObject* self, PyObject* args)
{
    int err;
    int increment;

    if (!PyArg_ParseTuple(args, "i:mutex_set_increment", &increment))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_set_increment(self->db_env, increment);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_mutex_get_increment(DBEnvObject* self)
{
    int err;
    u_int32_t increment;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_get_increment(self->db_env, &increment);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return NUMBER_FromLong(increment);
}

static PyObject*
DBEnv_mutex_set_tas_spins(DBEnvObject* self, PyObject* args)
{
    int err;
    int tas_spins;

    if (!PyArg_ParseTuple(args, "i:mutex_set_tas_spins", &tas_spins))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_set_tas_spins(self->db_env, tas_spins);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_mutex_get_tas_spins(DBEnvObject* self)
{
    int err;
    u_int32_t tas_spins;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_get_tas_spins(self->db_env, &tas_spins);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return NUMBER_FromLong(tas_spins);
}
#endif

static PyObject*
DBEnv_set_data_dir(DBEnvObject* self, PyObject* args)
{
    int err;
    char *dir;

    if (!PyArg_ParseTuple(args, "s:set_data_dir", &dir))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_data_dir(self->db_env, dir);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_data_dirs(DBEnvObject* self)
{
    int err;
    PyObject *tuple;
    PyObject *item;
    const char **dirpp;
    int size, i;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_data_dirs(self->db_env, &dirpp);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    /*
    ** Calculate size. Python C API
    ** actually allows for tuple resizing,
    ** but this is simple enough.
    */
    for (size=0; *(dirpp+size) ; size++);

    tuple = PyTuple_New(size);
    if (!tuple)
        return NULL;

    for (i=0; i<size; i++) {
        item = PyBytes_FromString (*(dirpp+i));
        if (item == NULL) {
            Py_DECREF(tuple);
            tuple = NULL;
            break;
        }
        PyTuple_SET_ITEM(tuple, i, item);
    }
    return tuple;
}

#if (DBVER >= 44)
static PyObject*
DBEnv_set_lg_filemode(DBEnvObject* self, PyObject* args)
{
    int err, filemode;

    if (!PyArg_ParseTuple(args, "i:set_lg_filemode", &filemode))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lg_filemode(self->db_env, filemode);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lg_filemode(DBEnvObject* self)
{
    int err, filemode;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lg_filemode(self->db_env, &filemode);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(filemode);
}
#endif

static PyObject*
DBEnv_set_lg_bsize(DBEnvObject* self, PyObject* args)
{
    int err, lg_bsize;

    if (!PyArg_ParseTuple(args, "i:set_lg_bsize", &lg_bsize))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lg_bsize(self->db_env, lg_bsize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lg_bsize(DBEnvObject* self)
{
    int err;
    u_int32_t lg_bsize;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lg_bsize(self->db_env, &lg_bsize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(lg_bsize);
}

static PyObject*
DBEnv_set_lg_dir(DBEnvObject* self, PyObject* args)
{
    int err;
    char *dir;

    if (!PyArg_ParseTuple(args, "s:set_lg_dir", &dir))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lg_dir(self->db_env, dir);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lg_dir(DBEnvObject* self)
{
    int err;
    const char *dirp;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lg_dir(self->db_env, &dirp);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return PyBytes_FromString(dirp);
}

static PyObject*
DBEnv_set_lg_max(DBEnvObject* self, PyObject* args)
{
    int err, lg_max;

    if (!PyArg_ParseTuple(args, "i:set_lg_max", &lg_max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lg_max(self->db_env, lg_max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lg_max(DBEnvObject* self)
{
    int err;
    u_int32_t lg_max;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lg_max(self->db_env, &lg_max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(lg_max);
}

static PyObject*
DBEnv_set_lg_regionmax(DBEnvObject* self, PyObject* args)
{
    int err, lg_max;

    if (!PyArg_ParseTuple(args, "i:set_lg_regionmax", &lg_max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lg_regionmax(self->db_env, lg_max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lg_regionmax(DBEnvObject* self)
{
    int err;
    u_int32_t lg_regionmax;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lg_regionmax(self->db_env, &lg_regionmax);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(lg_regionmax);
}

#if (DBVER >= 47)
static PyObject*
DBEnv_set_lk_partitions(DBEnvObject* self, PyObject* args)
{
    int err, lk_partitions;

    if (!PyArg_ParseTuple(args, "i:set_lk_partitions", &lk_partitions))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_partitions(self->db_env, lk_partitions);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lk_partitions(DBEnvObject* self)
{
    int err;
    u_int32_t lk_partitions;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lk_partitions(self->db_env, &lk_partitions);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(lk_partitions);
}
#endif

static PyObject*
DBEnv_set_lk_detect(DBEnvObject* self, PyObject* args)
{
    int err, lk_detect;

    if (!PyArg_ParseTuple(args, "i:set_lk_detect", &lk_detect))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_detect(self->db_env, lk_detect);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lk_detect(DBEnvObject* self)
{
    int err;
    u_int32_t lk_detect;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lk_detect(self->db_env, &lk_detect);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(lk_detect);
}

#if (DBVER < 45)
static PyObject*
DBEnv_set_lk_max(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_lk_max", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_max(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif



static PyObject*
DBEnv_set_lk_max_locks(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_lk_max_locks", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_max_locks(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lk_max_locks(DBEnvObject* self)
{
    int err;
    u_int32_t lk_max;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lk_max_locks(self->db_env, &lk_max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(lk_max);
}

static PyObject*
DBEnv_set_lk_max_lockers(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_lk_max_lockers", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_max_lockers(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lk_max_lockers(DBEnvObject* self)
{
    int err;
    u_int32_t lk_max;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lk_max_lockers(self->db_env, &lk_max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(lk_max);
}

static PyObject*
DBEnv_set_lk_max_objects(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_lk_max_objects", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_lk_max_objects(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_lk_max_objects(DBEnvObject* self)
{
    int err;
    u_int32_t lk_max;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_lk_max_objects(self->db_env, &lk_max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(lk_max);
}

static PyObject*
DBEnv_get_mp_mmapsize(DBEnvObject* self)
{
    int err;
    size_t mmapsize;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_mp_mmapsize(self->db_env, &mmapsize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(mmapsize);
}

static PyObject*
DBEnv_set_mp_mmapsize(DBEnvObject* self, PyObject* args)
{
    int err, mp_mmapsize;

    if (!PyArg_ParseTuple(args, "i:set_mp_mmapsize", &mp_mmapsize))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_mp_mmapsize(self->db_env, mp_mmapsize);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_set_tmp_dir(DBEnvObject* self, PyObject* args)
{
    int err;
    char *dir;

    if (!PyArg_ParseTuple(args, "s:set_tmp_dir", &dir))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_tmp_dir(self->db_env, dir);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_tmp_dir(DBEnvObject* self)
{
    int err;
    const char *dirpp;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_tmp_dir(self->db_env, &dirpp);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();

    return PyBytes_FromString(dirpp);
}

static PyObject*
DBEnv_txn_recover(DBEnvObject* self)
{
    int flags = DB_FIRST;
    int err, i;
    PyObject *list, *tuple, *gid;
    DBTxnObject *txn;
#define PREPLIST_LEN 16
    DB_PREPLIST preplist[PREPLIST_LEN];
#if (DBVER < 48) || (DBVER >= 52)
    long retp;
#else
    u_int32_t retp;
#endif

    CHECK_ENV_NOT_CLOSED(self);

    list=PyList_New(0);
    if (!list)
        return NULL;
    while (!0) {
        MYDB_BEGIN_ALLOW_THREADS
        err=self->db_env->txn_recover(self->db_env,
                        preplist, PREPLIST_LEN, &retp, flags);
#undef PREPLIST_LEN
        MYDB_END_ALLOW_THREADS
        if (err) {
            Py_DECREF(list);
            RETURN_IF_ERR();
        }
        if (!retp) break;
        flags=DB_NEXT;  /* Prepare for next loop pass */
        for (i=0; i<retp; i++) {
            gid=PyBytes_FromStringAndSize((char *)(preplist[i].gid),
                                DB_GID_SIZE);
            if (!gid) {
                Py_DECREF(list);
                return NULL;
            }
            txn=newDBTxnObject(self, NULL, preplist[i].txn, 0);
            if (!txn) {
                Py_DECREF(list);
                Py_DECREF(gid);
                return NULL;
            }
            txn->flag_prepare=1;  /* Recover state */
            tuple=PyTuple_New(2);
            if (!tuple) {
                Py_DECREF(list);
                Py_DECREF(gid);
                Py_DECREF(txn);
                return NULL;
            }
            if (PyTuple_SetItem(tuple, 0, gid)) {
                Py_DECREF(list);
                Py_DECREF(gid);
                Py_DECREF(txn);
                Py_DECREF(tuple);
                return NULL;
            }
            if (PyTuple_SetItem(tuple, 1, (PyObject *)txn)) {
                Py_DECREF(list);
                Py_DECREF(txn);
                Py_DECREF(tuple); /* This delete the "gid" also */
                return NULL;
            }
            if (PyList_Append(list, tuple)) {
                Py_DECREF(list);
                Py_DECREF(tuple);/* This delete the "gid" and the "txn" also */
                return NULL;
            }
            Py_DECREF(tuple);
        }
    }
    return list;
}

static PyObject*
DBEnv_txn_begin(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int flags = 0;
    PyObject* txnobj = NULL;
    DB_TXN *txn = NULL;
    static char* kwnames[] = { "parent", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:txn_begin", kwnames,
                                     &txnobj, &flags))
        return NULL;

    if (!checkTxnObj(txnobj, &txn))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    return (PyObject*)newDBTxnObject(self, (DBTxnObject *)txnobj, NULL, flags);
}


static PyObject*
DBEnv_txn_checkpoint(DBEnvObject* self, PyObject* args)
{
    int err, kbyte=0, min=0, flags=0;

    if (!PyArg_ParseTuple(args, "|iii:txn_checkpoint", &kbyte, &min, &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->txn_checkpoint(self->db_env, kbyte, min, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_tx_max(DBEnvObject* self)
{
    int err;
    u_int32_t max;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_tx_max(self->db_env, &max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return PyLong_FromUnsignedLong(max);
}

static PyObject*
DBEnv_set_tx_max(DBEnvObject* self, PyObject* args)
{
    int err, max;

    if (!PyArg_ParseTuple(args, "i:set_tx_max", &max))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_tx_max(self->db_env, max);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_tx_timestamp(DBEnvObject* self)
{
    int err;
    time_t timestamp;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_tx_timestamp(self->db_env, &timestamp);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(timestamp);
}

static PyObject*
DBEnv_set_tx_timestamp(DBEnvObject* self, PyObject* args)
{
    int err;
    long stamp;
    time_t timestamp;

    if (!PyArg_ParseTuple(args, "l:set_tx_timestamp", &stamp))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);
    timestamp = (time_t)stamp;
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_tx_timestamp(self->db_env, &timestamp);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_lock_detect(DBEnvObject* self, PyObject* args)
{
    int err, atype, flags=0;
    int aborted = 0;

    if (!PyArg_ParseTuple(args, "i|i:lock_detect", &atype, &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->lock_detect(self->db_env, flags, atype, &aborted);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(aborted);
}


static PyObject*
DBEnv_lock_get(DBEnvObject* self, PyObject* args)
{
    int flags=0;
    int locker, lock_mode;
    DBT obj;
    PyObject* objobj;

    if (!PyArg_ParseTuple(args, "iOi|i:lock_get", &locker, &objobj, &lock_mode, &flags))
        return NULL;


    if (!make_dbt(objobj, &obj))
        return NULL;

    return (PyObject*)newDBLockObject(self, locker, &obj, lock_mode, flags);
}


static PyObject*
DBEnv_lock_id(DBEnvObject* self)
{
    int err;
    u_int32_t theID;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->lock_id(self->db_env, &theID);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    return NUMBER_FromLong((long)theID);
}

static PyObject*
DBEnv_lock_id_free(DBEnvObject* self, PyObject* args)
{
    int err;
    u_int32_t theID;

    if (!PyArg_ParseTuple(args, "I:lock_id_free", &theID))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->lock_id_free(self->db_env, theID);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_lock_put(DBEnvObject* self, PyObject* args)
{
    int err;
    DBLockObject* dblockobj;

    if (!PyArg_ParseTuple(args, "O!:lock_put", &DBLock_Type, &dblockobj))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->lock_put(self->db_env, &dblockobj->lock);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

#if (DBVER >= 44)
static PyObject*
DBEnv_fileid_reset(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    char *file;
    u_int32_t flags = 0;
    static char* kwnames[] = { "file", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "z|i:fileid_reset", kwnames,
                                     &file, &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->fileid_reset(self->db_env, file, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_lsn_reset(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    char *file;
    u_int32_t flags = 0;
    static char* kwnames[] = { "file", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "z|i:lsn_reset", kwnames,
                                     &file, &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->lsn_reset(self->db_env, file, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif /* DBVER >= 4.4 */


static PyObject*
DBEnv_stat_print(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:stat_print",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->stat_print(self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_log_stat(DBEnvObject* self, PyObject* args)
{
    int err;
    DB_LOG_STAT* statp = NULL;
    PyObject* d = NULL;
    u_int32_t flags = 0;

    if (!PyArg_ParseTuple(args, "|i:log_stat", &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->log_stat(self->db_env, &statp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    /* Turn the stat structure into a dictionary */
    d = PyDict_New();
    if (d == NULL) {
        if (statp)
            free(statp);
        return NULL;
    }

#define MAKE_ENTRY(name)  _addIntToDict(d, #name, statp->st_##name)

    MAKE_ENTRY(magic);
    MAKE_ENTRY(version);
    MAKE_ENTRY(mode);
    MAKE_ENTRY(lg_bsize);
#if (DBVER >= 44)
    MAKE_ENTRY(lg_size);
    MAKE_ENTRY(record);
#endif
    MAKE_ENTRY(w_mbytes);
    MAKE_ENTRY(w_bytes);
    MAKE_ENTRY(wc_mbytes);
    MAKE_ENTRY(wc_bytes);
    MAKE_ENTRY(wcount);
    MAKE_ENTRY(wcount_fill);
#if (DBVER >= 44)
    MAKE_ENTRY(rcount);
#endif
    MAKE_ENTRY(scount);
    MAKE_ENTRY(cur_file);
    MAKE_ENTRY(cur_offset);
    MAKE_ENTRY(disk_file);
    MAKE_ENTRY(disk_offset);
    MAKE_ENTRY(maxcommitperflush);
    MAKE_ENTRY(mincommitperflush);
    MAKE_ENTRY(regsize);
    MAKE_ENTRY(region_wait);
    MAKE_ENTRY(region_nowait);

#undef MAKE_ENTRY
    free(statp);
    return d;
} /* DBEnv_log_stat */


static PyObject*
DBEnv_log_stat_print(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:log_stat_print",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->log_stat_print(self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_lock_stat(DBEnvObject* self, PyObject* args)
{
    int err;
    DB_LOCK_STAT* sp;
    PyObject* d = NULL;
    u_int32_t flags = 0;

    if (!PyArg_ParseTuple(args, "|i:lock_stat", &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->lock_stat(self->db_env, &sp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    /* Turn the stat structure into a dictionary */
    d = PyDict_New();
    if (d == NULL) {
        free(sp);
        return NULL;
    }

#define MAKE_ENTRY(name)  _addIntToDict(d, #name, sp->st_##name)

    MAKE_ENTRY(id);
    MAKE_ENTRY(cur_maxid);
    MAKE_ENTRY(nmodes);
    MAKE_ENTRY(maxlocks);
    MAKE_ENTRY(maxlockers);
    MAKE_ENTRY(maxobjects);
    MAKE_ENTRY(nlocks);
    MAKE_ENTRY(maxnlocks);
    MAKE_ENTRY(nlockers);
    MAKE_ENTRY(maxnlockers);
    MAKE_ENTRY(nobjects);
    MAKE_ENTRY(maxnobjects);
    MAKE_ENTRY(nrequests);
    MAKE_ENTRY(nreleases);
#if (DBVER >= 44)
    MAKE_ENTRY(nupgrade);
    MAKE_ENTRY(ndowngrade);
#endif
#if (DBVER < 44)
    MAKE_ENTRY(nnowaits);       /* these were renamed in 4.4 */
    MAKE_ENTRY(nconflicts);
#else
    MAKE_ENTRY(lock_nowait);
    MAKE_ENTRY(lock_wait);
#endif
    MAKE_ENTRY(ndeadlocks);
    MAKE_ENTRY(locktimeout);
    MAKE_ENTRY(txntimeout);
    MAKE_ENTRY(nlocktimeouts);
    MAKE_ENTRY(ntxntimeouts);
#if (DBVER >= 46)
    MAKE_ENTRY(objs_wait);
    MAKE_ENTRY(objs_nowait);
    MAKE_ENTRY(lockers_wait);
    MAKE_ENTRY(lockers_nowait);
#if (DBVER >= 47)
    MAKE_ENTRY(lock_wait);
    MAKE_ENTRY(lock_nowait);
#else
    MAKE_ENTRY(locks_wait);
    MAKE_ENTRY(locks_nowait);
#endif
    MAKE_ENTRY(hash_len);
#endif
    MAKE_ENTRY(regsize);
    MAKE_ENTRY(region_wait);
    MAKE_ENTRY(region_nowait);

#undef MAKE_ENTRY
    free(sp);
    return d;
}

static PyObject*
DBEnv_lock_stat_print(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:lock_stat_print",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->lock_stat_print(self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_log_cursor(DBEnvObject* self)
{
    int err;
    DB_LOGC* dblogc;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->log_cursor(self->db_env, &dblogc, 0);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return (PyObject*) newDBLogCursorObject(dblogc, self);
}


static PyObject*
DBEnv_log_flush(DBEnvObject* self)
{
    int err;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS
    err = self->db_env->log_flush(self->db_env, NULL);
    MYDB_END_ALLOW_THREADS

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_log_file(DBEnvObject* self, PyObject* args)
{
    int err;
    DB_LSN lsn = {0, 0};
    int size = 20;
    char *name = NULL;
    PyObject *retval;

    if (!PyArg_ParseTuple(args, "(ii):log_file", &lsn.file, &lsn.offset))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    do {
        name = malloc(size);
        if (!name) {
            PyErr_NoMemory();
            return NULL;
        }
        MYDB_BEGIN_ALLOW_THREADS;
        err = self->db_env->log_file(self->db_env, &lsn, name, size);
        MYDB_END_ALLOW_THREADS;
        if (err == EINVAL) {
            free(name);
            size *= 2;
        } else if (err) {
            free(name);
            RETURN_IF_ERR();
            assert(0);  /* Unreachable... supposely */
            return NULL;
        }
/*
** If the final buffer we try is too small, we will
** get this exception:
** DBInvalidArgError:
**    (22, 'Invalid argument -- DB_ENV->log_file: name buffer is too short')
*/
    } while ((err == EINVAL) && (size<(1<<17)));

    RETURN_IF_ERR();  /* Maybe the size is not the problem */

    retval = Py_BuildValue("s", name);
    free(name);
    return retval;
}


#if (DBVER >= 44)
static PyObject*
DBEnv_log_printf(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    char *string;
    PyObject *txnobj = NULL;
    DB_TXN *txn = NULL;
    static char* kwnames[] = {"string", "txn", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|O:log_printf", kwnames,
                &string, &txnobj))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    /*
    ** Do not use the format string directly, to avoid attacks.
    */
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->log_printf(self->db_env, txn, "%s", string);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif


static PyObject*
DBEnv_log_archive(DBEnvObject* self, PyObject* args)
{
    int flags=0;
    int err;
    char **log_list = NULL;
    PyObject* list;
    PyObject* item = NULL;

    if (!PyArg_ParseTuple(args, "|i:log_archive", &flags))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->log_archive(self->db_env, &log_list, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    list = PyList_New(0);
    if (list == NULL) {
        if (log_list)
            free(log_list);
        return NULL;
    }

    if (log_list) {
        char **log_list_start;
        for (log_list_start = log_list; *log_list != NULL; ++log_list) {
            item = PyBytes_FromString (*log_list);
            if (item == NULL) {
                Py_DECREF(list);
                list = NULL;
                break;
            }
            if (PyList_Append(list, item)) {
                Py_DECREF(list);
                list = NULL;
                Py_DECREF(item);
                break;
            }
            Py_DECREF(item);
        }
        free(log_list_start);
    }
    return list;
}


#if (DBVER >= 52)
static PyObject*
DBEnv_repmgr_site(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    DB_SITE* site;
    char *host;
    u_int port;
    static char* kwnames[] = {"host", "port", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "si:repmgr_site", kwnames,
                                     &host, &port))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_site(self->db_env, host, port, &site, 0);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return (PyObject*) newDBSiteObject(site, self);
}

static PyObject*
DBEnv_repmgr_site_by_eid(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    DB_SITE* site;
    int eid;
    static char* kwnames[] = {"eid", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:repmgr_site_by_eid",
                kwnames, &eid))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_site_by_eid(self->db_env, eid, &site);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return (PyObject*) newDBSiteObject(site, self);
}
#endif


#if (DBVER >= 44)
static PyObject*
DBEnv_mutex_stat(DBEnvObject* self, PyObject* args)
{
    int err;
    DB_MUTEX_STAT* statp = NULL;
    PyObject* d = NULL;
    u_int32_t flags = 0;

    if (!PyArg_ParseTuple(args, "|i:mutex_stat", &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_stat(self->db_env, &statp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    /* Turn the stat structure into a dictionary */
    d = PyDict_New();
    if (d == NULL) {
        if (statp)
            free(statp);
        return NULL;
    }

#define MAKE_ENTRY(name)  _addIntToDict(d, #name, statp->st_##name)

    MAKE_ENTRY(mutex_align);
    MAKE_ENTRY(mutex_tas_spins);
    MAKE_ENTRY(mutex_cnt);
    MAKE_ENTRY(mutex_free);
    MAKE_ENTRY(mutex_inuse);
    MAKE_ENTRY(mutex_inuse_max);
    MAKE_ENTRY(regsize);
    MAKE_ENTRY(region_wait);
    MAKE_ENTRY(region_nowait);

#undef MAKE_ENTRY
    free(statp);
    return d;
}
#endif


#if (DBVER >= 44)
static PyObject*
DBEnv_mutex_stat_print(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:mutex_stat_print",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->mutex_stat_print(self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif


static PyObject*
DBEnv_txn_stat_print(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:stat_print",
                kwnames, &flags))
    {
        return NULL;
    }

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->txn_stat_print(self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBEnv_txn_stat(DBEnvObject* self, PyObject* args)
{
    int err;
    DB_TXN_STAT* sp;
    PyObject* d = NULL;
    u_int32_t flags=0;

    if (!PyArg_ParseTuple(args, "|i:txn_stat", &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->txn_stat(self->db_env, &sp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    /* Turn the stat structure into a dictionary */
    d = PyDict_New();
    if (d == NULL) {
        free(sp);
        return NULL;
    }

#define MAKE_ENTRY(name)        _addIntToDict(d, #name, sp->st_##name)
#define MAKE_TIME_T_ENTRY(name) _addTimeTToDict(d, #name, sp->st_##name)
#define MAKE_DB_LSN_ENTRY(name) _addDB_lsnToDict(d, #name, sp->st_##name)

    MAKE_DB_LSN_ENTRY(last_ckp);
    MAKE_TIME_T_ENTRY(time_ckp);
    MAKE_ENTRY(last_txnid);
    MAKE_ENTRY(maxtxns);
    MAKE_ENTRY(nactive);
    MAKE_ENTRY(maxnactive);
#if (DBVER >= 45)
    MAKE_ENTRY(nsnapshot);
    MAKE_ENTRY(maxnsnapshot);
#endif
    MAKE_ENTRY(nbegins);
    MAKE_ENTRY(naborts);
    MAKE_ENTRY(ncommits);
    MAKE_ENTRY(nrestores);
    MAKE_ENTRY(regsize);
    MAKE_ENTRY(region_wait);
    MAKE_ENTRY(region_nowait);

#undef MAKE_DB_LSN_ENTRY
#undef MAKE_ENTRY
#undef MAKE_TIME_T_ENTRY
    free(sp);
    return d;
}


static PyObject*
DBEnv_set_get_returns_none(DBEnvObject* self, PyObject* args)
{
    int flags=0;
    int oldValue=0;

    if (!PyArg_ParseTuple(args,"i:set_get_returns_none", &flags))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    if (self->moduleFlags.getReturnsNone)
        ++oldValue;
    if (self->moduleFlags.cursorSetReturnsNone)
        ++oldValue;
    self->moduleFlags.getReturnsNone = (flags >= 1);
    self->moduleFlags.cursorSetReturnsNone = (flags >= 2);
    return NUMBER_FromLong(oldValue);
}

static PyObject*
DBEnv_get_private(DBEnvObject* self)
{
    /* We can give out the private field even if dbenv is closed */
    Py_INCREF(self->private_obj);
    return self->private_obj;
}

static PyObject*
DBEnv_set_private(DBEnvObject* self, PyObject* private_obj)
{
    /* We can set the private field even if dbenv is closed */
    Py_INCREF(private_obj);
    Py_SETREF(self->private_obj, private_obj);
    RETURN_NONE();
}

#if (DBVER >= 47)
static PyObject*
DBEnv_set_intermediate_dir_mode(DBEnvObject* self, PyObject* args)
{
    int err;
    const char *mode;

    if (!PyArg_ParseTuple(args,"s:set_intermediate_dir_mode", &mode))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_intermediate_dir_mode(self->db_env, mode);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_intermediate_dir_mode(DBEnvObject* self)
{
    int err;
    const char *mode;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_intermediate_dir_mode(self->db_env, &mode);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return Py_BuildValue("s", mode);
}
#endif

#if (DBVER < 47)
static PyObject*
DBEnv_set_intermediate_dir(DBEnvObject* self, PyObject* args)
{
    int err;
    int mode;
    u_int32_t flags;

    if (!PyArg_ParseTuple(args, "iI:set_intermediate_dir", &mode, &flags))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_intermediate_dir(self->db_env, mode, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif

static PyObject*
DBEnv_get_open_flags(DBEnvObject* self)
{
    int err;
    unsigned int flags;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_open_flags(self->db_env, &flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(flags);
}

#if (DBVER < 48)
static PyObject*
DBEnv_set_rpc_server(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    char *host;
    long cl_timeout=0, sv_timeout=0;

    static char* kwnames[] = { "host", "cl_timeout", "sv_timeout", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|ll:set_rpc_server", kwnames,
                                     &host, &cl_timeout, &sv_timeout))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_rpc_server(self->db_env, NULL, host, cl_timeout,
            sv_timeout, 0);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif

static PyObject*
DBEnv_set_mp_max_openfd(DBEnvObject* self, PyObject* args)
{
    int err;
    int maxopenfd;

    if (!PyArg_ParseTuple(args, "i:set_mp_max_openfd", &maxopenfd)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_mp_max_openfd(self->db_env, maxopenfd);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_mp_max_openfd(DBEnvObject* self)
{
    int err;
    int maxopenfd;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_mp_max_openfd(self->db_env, &maxopenfd);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(maxopenfd);
}


static PyObject*
DBEnv_set_mp_max_write(DBEnvObject* self, PyObject* args)
{
    int err;
    int maxwrite, maxwrite_sleep;

    if (!PyArg_ParseTuple(args, "ii:set_mp_max_write", &maxwrite,
                &maxwrite_sleep)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_mp_max_write(self->db_env, maxwrite,
            maxwrite_sleep);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_mp_max_write(DBEnvObject* self)
{
    int err;
    int maxwrite;
#if (DBVER >= 46)
    db_timeout_t maxwrite_sleep;
#else
    int maxwrite_sleep;
#endif

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_mp_max_write(self->db_env, &maxwrite,
            &maxwrite_sleep);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    return Py_BuildValue("(ii)", maxwrite, (int)maxwrite_sleep);
}


static PyObject*
DBEnv_set_verbose(DBEnvObject* self, PyObject* args)
{
    int err;
    int which, onoff;

    if (!PyArg_ParseTuple(args, "ii:set_verbose", &which, &onoff)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_verbose(self->db_env, which, onoff);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_get_verbose(DBEnvObject* self, PyObject* args)
{
    int err;
    int which;
    int verbose;

    if (!PyArg_ParseTuple(args, "i:get_verbose", &which)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->get_verbose(self->db_env, which, &verbose);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return PyBool_FromLong(verbose);
}

#if (DBVER >= 45)
static void
_dbenv_event_notifyCallback(DB_ENV* db_env, u_int32_t event, void *event_info)
{
    DBEnvObject *dbenv;
    PyObject* callback;
    PyObject* args;
    PyObject* result = NULL;

    MYDB_BEGIN_BLOCK_THREADS;
    dbenv = (DBEnvObject *)db_env->app_private;
    callback = dbenv->event_notifyCallback;
    if (callback) {
        if (event == DB_EVENT_REP_NEWMASTER) {
            args = Py_BuildValue("(Oii)", dbenv, event, *((int *)event_info));
        } else {
            args = Py_BuildValue("(OiO)", dbenv, event, Py_None);
        }
        if (args) {
            result = PyEval_CallObject(callback, args);
        }
        if ((!args) || (!result)) {
            PyErr_Print();
        }
        Py_XDECREF(args);
        Py_XDECREF(result);
    }
    MYDB_END_BLOCK_THREADS;
}
#endif

#if (DBVER >= 45)
static PyObject*
DBEnv_set_event_notify(DBEnvObject* self, PyObject* notifyFunc)
{
    int err;

    CHECK_ENV_NOT_CLOSED(self);

    if (!PyCallable_Check(notifyFunc)) {
            makeTypeError("Callable", notifyFunc);
            return NULL;
    }

    Py_INCREF(notifyFunc);
    Py_SETREF(self->event_notifyCallback, notifyFunc);

    /* This is to workaround a problem with un-initialized threads (see
       comment in DB_associate) */
#ifdef WITH_THREAD
    PyEval_InitThreads();
#endif

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->set_event_notify(self->db_env, _dbenv_event_notifyCallback);
    MYDB_END_ALLOW_THREADS;

    if (err) {
            Py_DECREF(notifyFunc);
            self->event_notifyCallback = NULL;
    }

    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif


/* --------------------------------------------------------------------- */
/* REPLICATION METHODS: Base Replication */


static PyObject*
DBEnv_rep_process_message(DBEnvObject* self, PyObject* args)
{
    int err;
    PyObject *control_py, *rec_py;
    DBT control, rec;
    int envid;
    DB_LSN lsn;

    if (!PyArg_ParseTuple(args, "OOi:rep_process_message", &control_py,
                &rec_py, &envid))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    if (!make_dbt(control_py, &control))
        return NULL;
    if (!make_dbt(rec_py, &rec))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >= 46)
    err = self->db_env->rep_process_message(self->db_env, &control, &rec,
            envid, &lsn);
#else
    err = self->db_env->rep_process_message(self->db_env, &control, &rec,
            &envid, &lsn);
#endif
    MYDB_END_ALLOW_THREADS;
    switch (err) {
        case DB_REP_NEWMASTER :
          return Py_BuildValue("(iO)", envid, Py_None);
          break;

        case DB_REP_DUPMASTER :
        case DB_REP_HOLDELECTION :
#if (DBVER >= 44)
        case DB_REP_IGNORE :
        case DB_REP_JOIN_FAILURE :
#endif
            return Py_BuildValue("(iO)", err, Py_None);
            break;
        case DB_REP_NEWSITE :
            {
                PyObject *tmp, *r;

                if (!(tmp = PyBytes_FromStringAndSize(rec.data, rec.size))) {
                    return NULL;
                }

                r = Py_BuildValue("(iO)", err, tmp);
                Py_DECREF(tmp);
                return r;
                break;
            }
        case DB_REP_NOTPERM :
        case DB_REP_ISPERM :
            return Py_BuildValue("(i(ll))", err, lsn.file, lsn.offset);
            break;
    }
    RETURN_IF_ERR();
    return PyTuple_Pack(2, Py_None, Py_None);
}

static int
_DBEnv_rep_transportCallback(DB_ENV* db_env, const DBT* control, const DBT* rec,
        const DB_LSN *lsn, int envid, u_int32_t flags)
{
    DBEnvObject *dbenv;
    PyObject* rep_transport;
    PyObject* args;
    PyObject *a, *b;
    PyObject* result = NULL;
    int ret=0;

    MYDB_BEGIN_BLOCK_THREADS;
    dbenv = (DBEnvObject *)db_env->app_private;
    rep_transport = dbenv->rep_transport;

    /*
    ** The errors in 'a' or 'b' are detected in "Py_BuildValue".
    */
    a = PyBytes_FromStringAndSize(control->data, control->size);
    b = PyBytes_FromStringAndSize(rec->data, rec->size);

    args = Py_BuildValue(
            "(OOO(ll)iI)",
            dbenv,
            a, b,
            lsn->file, lsn->offset, envid, flags);
    if (args) {
        result = PyEval_CallObject(rep_transport, args);
    }

    if ((!args) || (!result)) {
        PyErr_Print();
        ret = -1;
    }
    Py_XDECREF(a);
    Py_XDECREF(b);
    Py_XDECREF(args);
    Py_XDECREF(result);
    MYDB_END_BLOCK_THREADS;
    return ret;
}

static PyObject*
DBEnv_rep_set_transport(DBEnvObject* self, PyObject* args)
{
    int err;
    int envid;
    PyObject *rep_transport;

    if (!PyArg_ParseTuple(args, "iO:rep_set_transport", &envid, &rep_transport))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);
    if (!PyCallable_Check(rep_transport)) {
        makeTypeError("Callable", rep_transport);
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
#if (DBVER >=45)
    err = self->db_env->rep_set_transport(self->db_env, envid,
            &_DBEnv_rep_transportCallback);
#else
    err = self->db_env->set_rep_transport(self->db_env, envid,
            &_DBEnv_rep_transportCallback);
#endif
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    Py_INCREF(rep_transport);
    Py_SETREF(self->rep_transport, rep_transport);
    RETURN_NONE();
}

#if (DBVER >= 47)
static PyObject*
DBEnv_rep_set_request(DBEnvObject* self, PyObject* args)
{
    int err;
    unsigned int minimum, maximum;

    if (!PyArg_ParseTuple(args,"II:rep_set_request", &minimum, &maximum))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_set_request(self->db_env, minimum, maximum);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_rep_get_request(DBEnvObject* self)
{
    int err;
    u_int32_t minimum, maximum;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_get_request(self->db_env, &minimum, &maximum);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return Py_BuildValue("II", minimum, maximum);
}
#endif

#if (DBVER >= 45)
static PyObject*
DBEnv_rep_set_limit(DBEnvObject* self, PyObject* args)
{
    int err;
    int limit;

    if (!PyArg_ParseTuple(args,"i:rep_set_limit", &limit))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_set_limit(self->db_env, 0, limit);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_rep_get_limit(DBEnvObject* self)
{
    int err;
    u_int32_t gbytes, bytes;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_get_limit(self->db_env, &gbytes, &bytes);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(bytes);
}
#endif

#if (DBVER >= 44)
static PyObject*
DBEnv_rep_set_config(DBEnvObject* self, PyObject* args)
{
    int err;
    int which;
    int onoff;

    if (!PyArg_ParseTuple(args,"ii:rep_set_config", &which, &onoff))
        return NULL;
    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_set_config(self->db_env, which, onoff);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_rep_get_config(DBEnvObject* self, PyObject* args)
{
    int err;
    int which;
    int onoff;

    if (!PyArg_ParseTuple(args, "i:rep_get_config", &which)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_get_config(self->db_env, which, &onoff);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return PyBool_FromLong(onoff);
}
#endif

#if (DBVER >= 46)
static PyObject*
DBEnv_rep_elect(DBEnvObject* self, PyObject* args)
{
    int err;
    u_int32_t nsites, nvotes;

    if (!PyArg_ParseTuple(args, "II:rep_elect", &nsites, &nvotes)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_elect(self->db_env, nsites, nvotes, 0);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif

static PyObject*
DBEnv_rep_start(DBEnvObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    PyObject *cdata_py = Py_None;
    DBT cdata;
    int flags;
    static char* kwnames[] = {"flags","cdata", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "i|O:rep_start", kwnames, &flags, &cdata_py))
    {
            return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);

    if (!make_dbt(cdata_py, &cdata))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_start(self->db_env, cdata.size ? &cdata : NULL,
            flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

#if (DBVER >= 44)
static PyObject*
DBEnv_rep_sync(DBEnvObject* self)
{
    int err;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_sync(self->db_env, 0);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif


#if (DBVER >= 45)
static PyObject*
DBEnv_rep_set_nsites(DBEnvObject* self, PyObject* args)
{
    int err;
    int nsites;

    if (!PyArg_ParseTuple(args, "i:rep_set_nsites", &nsites)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_set_nsites(self->db_env, nsites);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_rep_get_nsites(DBEnvObject* self)
{
    int err;
#if (DBVER >= 47)
    u_int32_t nsites;
#else
    int nsites;
#endif

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_get_nsites(self->db_env, &nsites);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(nsites);
}

static PyObject*
DBEnv_rep_set_priority(DBEnvObject* self, PyObject* args)
{
    int err;
    int priority;

    if (!PyArg_ParseTuple(args, "i:rep_set_priority", &priority)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_set_priority(self->db_env, priority);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_rep_get_priority(DBEnvObject* self)
{
    int err;
#if (DBVER >= 47)
    u_int32_t priority;
#else
    int priority;
#endif

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_get_priority(self->db_env, &priority);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(priority);
}

static PyObject*
DBEnv_rep_set_timeout(DBEnvObject* self, PyObject* args)
{
    int err;
    int which, timeout;

    if (!PyArg_ParseTuple(args, "ii:rep_set_timeout", &which, &timeout)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_set_timeout(self->db_env, which, timeout);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_rep_get_timeout(DBEnvObject* self, PyObject* args)
{
    int err;
    int which;
    u_int32_t timeout;

    if (!PyArg_ParseTuple(args, "i:rep_get_timeout", &which)) {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_get_timeout(self->db_env, which, &timeout);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(timeout);
}
#endif


#if (DBVER >= 47)
static PyObject*
DBEnv_rep_set_clockskew(DBEnvObject* self, PyObject* args)
{
    int err;
    unsigned int fast, slow;

    if (!PyArg_ParseTuple(args,"II:rep_set_clockskew", &fast, &slow))
        return NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_set_clockskew(self->db_env, fast, slow);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_rep_get_clockskew(DBEnvObject* self)
{
    int err;
    unsigned int fast, slow;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_get_clockskew(self->db_env, &fast, &slow);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return Py_BuildValue("(II)", fast, slow);
}
#endif

static PyObject*
DBEnv_rep_stat_print(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:rep_stat_print",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_stat_print(self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_rep_stat(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    DB_REP_STAT *statp;
    PyObject *stats;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:rep_stat",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->rep_stat(self->db_env, &statp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    stats=PyDict_New();
    if (stats == NULL) {
        free(statp);
        return NULL;
    }

#define MAKE_ENTRY(name)  _addIntToDict(stats, #name, statp->st_##name)
#define MAKE_DB_LSN_ENTRY(name) _addDB_lsnToDict(stats , #name, statp->st_##name)

#if (DBVER >= 44)
    MAKE_ENTRY(bulk_fills);
    MAKE_ENTRY(bulk_overflows);
    MAKE_ENTRY(bulk_records);
    MAKE_ENTRY(bulk_transfers);
    MAKE_ENTRY(client_rerequests);
    MAKE_ENTRY(client_svc_miss);
    MAKE_ENTRY(client_svc_req);
#endif
    MAKE_ENTRY(dupmasters);
    MAKE_ENTRY(egen);
    MAKE_ENTRY(election_nvotes);
    MAKE_ENTRY(startup_complete);
    MAKE_ENTRY(pg_duplicated);
    MAKE_ENTRY(pg_records);
    MAKE_ENTRY(pg_requested);
    MAKE_ENTRY(next_pg);
    MAKE_ENTRY(waiting_pg);
    MAKE_ENTRY(election_cur_winner);
    MAKE_ENTRY(election_gen);
    MAKE_DB_LSN_ENTRY(election_lsn);
    MAKE_ENTRY(election_nsites);
    MAKE_ENTRY(election_priority);
#if (DBVER >= 44)
    MAKE_ENTRY(election_sec);
    MAKE_ENTRY(election_usec);
#endif
    MAKE_ENTRY(election_status);
    MAKE_ENTRY(election_tiebreaker);
    MAKE_ENTRY(election_votes);
    MAKE_ENTRY(elections);
    MAKE_ENTRY(elections_won);
    MAKE_ENTRY(env_id);
    MAKE_ENTRY(env_priority);
    MAKE_ENTRY(gen);
    MAKE_ENTRY(log_duplicated);
    MAKE_ENTRY(log_queued);
    MAKE_ENTRY(log_queued_max);
    MAKE_ENTRY(log_queued_total);
    MAKE_ENTRY(log_records);
    MAKE_ENTRY(log_requested);
    MAKE_ENTRY(master);
    MAKE_ENTRY(master_changes);
#if (DBVER >= 47)
    MAKE_ENTRY(max_lease_sec);
    MAKE_ENTRY(max_lease_usec);
    MAKE_DB_LSN_ENTRY(max_perm_lsn);
#endif
    MAKE_ENTRY(msgs_badgen);
    MAKE_ENTRY(msgs_processed);
    MAKE_ENTRY(msgs_recover);
    MAKE_ENTRY(msgs_send_failures);
    MAKE_ENTRY(msgs_sent);
    MAKE_ENTRY(newsites);
    MAKE_DB_LSN_ENTRY(next_lsn);
    MAKE_ENTRY(nsites);
    MAKE_ENTRY(nthrottles);
    MAKE_ENTRY(outdated);
#if (DBVER >= 46)
    MAKE_ENTRY(startsync_delayed);
#endif
    MAKE_ENTRY(status);
    MAKE_ENTRY(txns_applied);
    MAKE_DB_LSN_ENTRY(waiting_lsn);

#undef MAKE_DB_LSN_ENTRY
#undef MAKE_ENTRY

    free(statp);
    return stats;
}

/* --------------------------------------------------------------------- */
/* REPLICATION METHODS: Replication Manager */

#if (DBVER >= 45)
static PyObject*
DBEnv_repmgr_start(DBEnvObject* self, PyObject* args, PyObject*
        kwargs)
{
    int err;
    int nthreads, flags;
    static char* kwnames[] = {"nthreads","flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "ii:repmgr_start", kwnames, &nthreads, &flags))
    {
            return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_start(self->db_env, nthreads, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

#if (DBVER < 52)
static PyObject*
DBEnv_repmgr_set_local_site(DBEnvObject* self, PyObject* args, PyObject*
        kwargs)
{
    int err;
    char *host;
    int port;
    int flags = 0;
    static char* kwnames[] = {"host", "port", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "si|i:repmgr_set_local_site", kwnames, &host, &port, &flags))
    {
            return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_set_local_site(self->db_env, host, port, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_repmgr_add_remote_site(DBEnvObject* self, PyObject* args, PyObject*
        kwargs)
{
    int err;
    char *host;
    int port;
    int flags = 0;
    int eidp;
    static char* kwnames[] = {"host", "port", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "si|i:repmgr_add_remote_site", kwnames, &host, &port, &flags))
    {
            return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_add_remote_site(self->db_env, host, port, &eidp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(eidp);
}
#endif

static PyObject*
DBEnv_repmgr_set_ack_policy(DBEnvObject* self, PyObject* args)
{
    int err;
    int ack_policy;

    if (!PyArg_ParseTuple(args, "i:repmgr_set_ack_policy", &ack_policy))
    {
            return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_set_ack_policy(self->db_env, ack_policy);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_repmgr_get_ack_policy(DBEnvObject* self)
{
    int err;
    int ack_policy;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_get_ack_policy(self->db_env, &ack_policy);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    return NUMBER_FromLong(ack_policy);
}

static PyObject*
DBEnv_repmgr_site_list(DBEnvObject* self)
{
    int err;
    unsigned int countp;
    DB_REPMGR_SITE *listp;
    PyObject *stats, *key, *tuple;

    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_site_list(self->db_env, &countp, &listp);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    stats=PyDict_New();
    if (stats == NULL) {
        free(listp);
        return NULL;
    }

    for(;countp--;) {
        key=NUMBER_FromLong(listp[countp].eid);
        if(!key) {
            Py_DECREF(stats);
            free(listp);
            return NULL;
        }
        tuple=Py_BuildValue("(sII)", listp[countp].host,
                listp[countp].port, listp[countp].status);
        if(!tuple) {
            Py_DECREF(key);
            Py_DECREF(stats);
            free(listp);
            return NULL;
        }
        if(PyDict_SetItem(stats, key, tuple)) {
            Py_DECREF(key);
            Py_DECREF(tuple);
            Py_DECREF(stats);
            free(listp);
            return NULL;
        }
        Py_DECREF(key);
        Py_DECREF(tuple);
    }
    free(listp);
    return stats;
}
#endif

#if (DBVER >= 46)
static PyObject*
DBEnv_repmgr_stat_print(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:repmgr_stat_print",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_stat_print(self->db_env, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBEnv_repmgr_stat(DBEnvObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    DB_REPMGR_STAT *statp;
    PyObject *stats;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:repmgr_stat",
                kwnames, &flags))
    {
        return NULL;
    }
    CHECK_ENV_NOT_CLOSED(self);
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->db_env->repmgr_stat(self->db_env, &statp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    stats=PyDict_New();
    if (stats == NULL) {
        free(statp);
        return NULL;
    }

#define MAKE_ENTRY(name)  _addIntToDict(stats, #name, statp->st_##name)

    MAKE_ENTRY(perm_failed);
    MAKE_ENTRY(msgs_queued);
    MAKE_ENTRY(msgs_dropped);
    MAKE_ENTRY(connection_drop);
    MAKE_ENTRY(connect_fail);

#undef MAKE_ENTRY

    free(statp);
    return stats;
}
#endif


/* --------------------------------------------------------------------- */
/* DBTxn methods */


static void _close_transaction_cursors(DBTxnObject* txn)
{
    PyObject *dummy;

    while(txn->children_cursors) {
        PyErr_Warn(PyExc_RuntimeWarning,
            "Must close cursors before resolving a transaction.");
        dummy=DBC_close_internal(txn->children_cursors);
        Py_XDECREF(dummy);
    }
}

static void _promote_transaction_dbs_and_sequences(DBTxnObject *txn)
{
    DBObject *db;
    DBSequenceObject *dbs;

    while (txn->children_dbs) {
        db=txn->children_dbs;
        EXTRACT_FROM_DOUBLE_LINKED_LIST_TXN(db);
        if (txn->parent_txn) {
            INSERT_IN_DOUBLE_LINKED_LIST_TXN(txn->parent_txn->children_dbs,db);
            db->txn=txn->parent_txn;
        } else {
            /* The db is already linked to its environment,
            ** so nothing to do.
            */
            db->txn=NULL;
        }
    }

    while (txn->children_sequences) {
        dbs=txn->children_sequences;
        EXTRACT_FROM_DOUBLE_LINKED_LIST_TXN(dbs);
        if (txn->parent_txn) {
            INSERT_IN_DOUBLE_LINKED_LIST_TXN(txn->parent_txn->children_sequences,dbs);
            dbs->txn=txn->parent_txn;
        } else {
            /* The sequence is already linked to its
            ** parent db. Nothing to do.
            */
            dbs->txn=NULL;
        }
    }
}


static PyObject*
DBTxn_commit(DBTxnObject* self, PyObject* args)
{
    int flags=0, err;
    DB_TXN *txn;

    if (!PyArg_ParseTuple(args, "|i:commit", &flags))
        return NULL;

    _close_transaction_cursors(self);

    if (!self->txn) {
        PyObject *t =  Py_BuildValue("(is)", 0, "DBTxn must not be used "
                                     "after txn_commit, txn_abort "
                                     "or txn_discard");
        if (t) {
            PyErr_SetObject(DBError, t);
            Py_DECREF(t);
        }
        return NULL;
    }
    self->flag_prepare=0;
    txn = self->txn;
    self->txn = NULL;   /* this DB_TXN is no longer valid after this call */

    EXTRACT_FROM_DOUBLE_LINKED_LIST(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = txn->commit(txn, flags);
    MYDB_END_ALLOW_THREADS;

    _promote_transaction_dbs_and_sequences(self);

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBTxn_prepare(DBTxnObject* self, PyObject* args)
{
    int err;
    char* gid=NULL;
    int   gid_size=0;

    if (!PyArg_ParseTuple(args, "s#:prepare", &gid, &gid_size))
        return NULL;

    if (gid_size != DB_GID_SIZE) {
        PyErr_SetString(PyExc_TypeError,
                        "gid must be DB_GID_SIZE bytes long");
        return NULL;
    }

    if (!self->txn) {
        PyObject *t = Py_BuildValue("(is)", 0,"DBTxn must not be used "
                                    "after txn_commit, txn_abort "
                                    "or txn_discard");
        if (t) {
            PyErr_SetObject(DBError, t);
            Py_DECREF(t);
        }
        return NULL;
    }
    self->flag_prepare=1;  /* Prepare state */
    MYDB_BEGIN_ALLOW_THREADS;
    err = self->txn->prepare(self->txn, (u_int8_t*)gid);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}


static PyObject*
DBTxn_abort_discard_internal(DBTxnObject* self, int discard)
{
    PyObject *dummy;
    int err=0;
    DB_TXN *txn;

    if (!self->txn) {
        PyObject *t = Py_BuildValue("(is)", 0, "DBTxn must not be used "
                                    "after txn_commit, txn_abort "
                                    "or txn_discard");
        if (t) {
            PyErr_SetObject(DBError, t);
            Py_DECREF(t);
        }
        return NULL;
    }
    txn = self->txn;
    self->txn = NULL;   /* this DB_TXN is no longer valid after this call */

    _close_transaction_cursors(self);
    while (self->children_sequences) {
        dummy=DBSequence_close_internal(self->children_sequences,0,0);
        Py_XDECREF(dummy);
    }
    while (self->children_dbs) {
        dummy=DB_close_internal(self->children_dbs, 0, 0);
        Py_XDECREF(dummy);
    }

    EXTRACT_FROM_DOUBLE_LINKED_LIST(self);

    MYDB_BEGIN_ALLOW_THREADS;
    if (discard) {
        assert(!self->flag_prepare);
        err = txn->discard(txn,0);
    } else {
        /*
        ** If the transaction is in the "prepare" or "recover" state,
        ** we better do not implicitly abort it.
        */
        if (!self->flag_prepare) {
            err = txn->abort(txn);
        }
    }
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBTxn_abort(DBTxnObject* self)
{
    self->flag_prepare=0;
    _close_transaction_cursors(self);

    return DBTxn_abort_discard_internal(self,0);
}

static PyObject*
DBTxn_discard(DBTxnObject* self)
{
    self->flag_prepare=0;
    _close_transaction_cursors(self);

    return DBTxn_abort_discard_internal(self,1);
}


static PyObject*
DBTxn_id(DBTxnObject* self)
{
    int id;

    if (!self->txn) {
        PyObject *t = Py_BuildValue("(is)", 0, "DBTxn must not be used "
                                    "after txn_commit, txn_abort "
                                    "or txn_discard");
        if (t) {
            PyErr_SetObject(DBError, t);
            Py_DECREF(t);
        }
        return NULL;
    }
    MYDB_BEGIN_ALLOW_THREADS;
    id = self->txn->id(self->txn);
    MYDB_END_ALLOW_THREADS;
    return NUMBER_FromLong(id);
}


static PyObject*
DBTxn_set_timeout(DBTxnObject* self, PyObject* args, PyObject* kwargs)
{
    int err;
    u_int32_t flags=0;
    u_int32_t timeout = 0;
    static char* kwnames[] = { "timeout", "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii:set_timeout", kwnames,
                &timeout, &flags)) {
        return NULL;
    }

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->txn->set_timeout(self->txn, (db_timeout_t)timeout, flags);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}


#if (DBVER >= 44)
static PyObject*
DBTxn_set_name(DBTxnObject* self, PyObject* args)
{
    int err;
    const char *name;

    if (!PyArg_ParseTuple(args, "s:set_name", &name))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->txn->set_name(self->txn, name);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
    RETURN_NONE();
}
#endif


#if (DBVER >= 44)
static PyObject*
DBTxn_get_name(DBTxnObject* self)
{
    int err;
    const char *name;

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->txn->get_name(self->txn, &name);
    MYDB_END_ALLOW_THREADS;

    RETURN_IF_ERR();
#if (PY_VERSION_HEX < 0x03000000)
    if (!name) {
        return PyString_FromString("");
    }
    return PyString_FromString(name);
#else
    if (!name) {
        return PyUnicode_FromString("");
    }
    return PyUnicode_FromString(name);
#endif
}
#endif


/* --------------------------------------------------------------------- */
/* DBSequence methods */


static PyObject*
DBSequence_close_internal(DBSequenceObject* self, int flags, int do_not_close)
{
    int err=0;

    if (self->sequence!=NULL) {
        EXTRACT_FROM_DOUBLE_LINKED_LIST(self);
        if (self->txn) {
            EXTRACT_FROM_DOUBLE_LINKED_LIST_TXN(self);
            self->txn=NULL;
        }

        /*
        ** "do_not_close" is used to dispose all related objects in the
        ** tree, without actually releasing the "root" object.
        ** This is done, for example, because function calls like
        ** "DBSequence.remove()" implicitly close the underlying handle. So
        ** the handle doesn't need to be closed, but related objects
        ** must be cleaned up.
        */
        if (!do_not_close) {
            MYDB_BEGIN_ALLOW_THREADS
            err = self->sequence->close(self->sequence, flags);
            MYDB_END_ALLOW_THREADS
        }
        self->sequence = NULL;

        RETURN_IF_ERR();
    }

    RETURN_NONE();
}

static PyObject*
DBSequence_close(DBSequenceObject* self, PyObject* args)
{
    int flags=0;
    if (!PyArg_ParseTuple(args,"|i:close", &flags))
        return NULL;

    return DBSequence_close_internal(self,flags,0);
}

static PyObject*
DBSequence_get(DBSequenceObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags = 0;
    int delta = 1;
    db_seq_t value;
    PyObject *txnobj = NULL;
    DB_TXN *txn = NULL;
    static char* kwnames[] = {"delta", "txn", "flags", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iOi:get", kwnames, &delta, &txnobj, &flags))
        return NULL;
    CHECK_SEQUENCE_NOT_CLOSED(self)

    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->get(self->sequence, txn, delta, &value, flags);
    MYDB_END_ALLOW_THREADS

    RETURN_IF_ERR();
    return PyLong_FromLongLong(value);
}

static PyObject*
DBSequence_get_dbp(DBSequenceObject* self)
{
    CHECK_SEQUENCE_NOT_CLOSED(self)
    Py_INCREF(self->mydb);
    return (PyObject* )self->mydb;
}

static PyObject*
DBSequence_get_key(DBSequenceObject* self)
{
    int err;
    DBT key;
    PyObject *retval = NULL;

    key.flags = DB_DBT_MALLOC;
    CHECK_SEQUENCE_NOT_CLOSED(self)
    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->get_key(self->sequence, &key);
    MYDB_END_ALLOW_THREADS

    if (!err)
        retval = Build_PyString(key.data, key.size);

    FREE_DBT(key);
    RETURN_IF_ERR();

    return retval;
}

static PyObject*
DBSequence_initial_value(DBSequenceObject* self, PyObject* args)
{
    int err;
    PY_LONG_LONG value;
    db_seq_t value2;
    if (!PyArg_ParseTuple(args,"L:initial_value", &value))
        return NULL;
    CHECK_SEQUENCE_NOT_CLOSED(self)

    value2=value; /* If truncation, compiler should show a warning */
    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->initial_value(self->sequence, value2);
    MYDB_END_ALLOW_THREADS

    RETURN_IF_ERR();

    RETURN_NONE();
}

static PyObject*
DBSequence_open(DBSequenceObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags = 0;
    PyObject* keyobj;
    PyObject *txnobj = NULL;
    DB_TXN *txn = NULL;
    DBT key;

    static char* kwnames[] = {"key", "txn", "flags", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oi:open", kwnames, &keyobj, &txnobj, &flags))
        return NULL;

    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    if (!make_key_dbt(self->mydb, keyobj, &key, NULL))
        return NULL;

    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->open(self->sequence, txn, &key, flags);
    MYDB_END_ALLOW_THREADS

    FREE_DBT(key);
    RETURN_IF_ERR();

    if (txn) {
        INSERT_IN_DOUBLE_LINKED_LIST_TXN(((DBTxnObject *)txnobj)->children_sequences,self);
        self->txn=(DBTxnObject *)txnobj;
    }

    RETURN_NONE();
}

static PyObject*
DBSequence_remove(DBSequenceObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject *dummy;
    int err, flags = 0;
    PyObject *txnobj = NULL;
    DB_TXN *txn = NULL;

    static char* kwnames[] = {"txn", "flags", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:remove", kwnames, &txnobj, &flags))
        return NULL;

    if (!checkTxnObj(txnobj, &txn))
        return NULL;

    CHECK_SEQUENCE_NOT_CLOSED(self)

    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->remove(self->sequence, txn, flags);
    MYDB_END_ALLOW_THREADS

    dummy=DBSequence_close_internal(self,flags,1);
    Py_XDECREF(dummy);

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBSequence_set_cachesize(DBSequenceObject* self, PyObject* args)
{
    int err, size;
    if (!PyArg_ParseTuple(args,"i:set_cachesize", &size))
        return NULL;
    CHECK_SEQUENCE_NOT_CLOSED(self)

    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->set_cachesize(self->sequence, size);
    MYDB_END_ALLOW_THREADS

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBSequence_get_cachesize(DBSequenceObject* self)
{
    int err, size;

    CHECK_SEQUENCE_NOT_CLOSED(self)

    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->get_cachesize(self->sequence, &size);
    MYDB_END_ALLOW_THREADS

    RETURN_IF_ERR();
    return NUMBER_FromLong(size);
}

static PyObject*
DBSequence_set_flags(DBSequenceObject* self, PyObject* args)
{
    int err, flags = 0;
    if (!PyArg_ParseTuple(args,"i:set_flags", &flags))
        return NULL;
    CHECK_SEQUENCE_NOT_CLOSED(self)

    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->set_flags(self->sequence, flags);
    MYDB_END_ALLOW_THREADS

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBSequence_get_flags(DBSequenceObject* self)
{
    unsigned int flags;
    int err;

    CHECK_SEQUENCE_NOT_CLOSED(self)

    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->get_flags(self->sequence, &flags);
    MYDB_END_ALLOW_THREADS

    RETURN_IF_ERR();
    return NUMBER_FromLong((int)flags);
}

static PyObject*
DBSequence_set_range(DBSequenceObject* self, PyObject* args)
{
    int err;
    PY_LONG_LONG min, max;
    db_seq_t min2, max2;
    if (!PyArg_ParseTuple(args,"(LL):set_range", &min, &max))
        return NULL;
    CHECK_SEQUENCE_NOT_CLOSED(self)

    min2=min;  /* If truncation, compiler should show a warning */
    max2=max;
    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->set_range(self->sequence, min2, max2);
    MYDB_END_ALLOW_THREADS

    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBSequence_get_range(DBSequenceObject* self)
{
    int err;
    PY_LONG_LONG min, max;
    db_seq_t min2, max2;

    CHECK_SEQUENCE_NOT_CLOSED(self)

    MYDB_BEGIN_ALLOW_THREADS
    err = self->sequence->get_range(self->sequence, &min2, &max2);
    MYDB_END_ALLOW_THREADS

    RETURN_IF_ERR();
    min=min2;  /* If truncation, compiler should show a warning */
    max=max2;
    return Py_BuildValue("(LL)", min, max);
}


static PyObject*
DBSequence_stat_print(DBSequenceObject* self, PyObject* args, PyObject *kwargs)
{
    int err;
    int flags=0;
    static char* kwnames[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:stat_print",
                kwnames, &flags))
    {
        return NULL;
    }

    CHECK_SEQUENCE_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->sequence->stat_print(self->sequence, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();
    RETURN_NONE();
}

static PyObject*
DBSequence_stat(DBSequenceObject* self, PyObject* args, PyObject* kwargs)
{
    int err, flags = 0;
    DB_SEQUENCE_STAT* sp = NULL;
    PyObject* dict_stat;
    static char* kwnames[] = {"flags", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:stat", kwnames, &flags))
        return NULL;
    CHECK_SEQUENCE_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    err = self->sequence->stat(self->sequence, &sp, flags);
    MYDB_END_ALLOW_THREADS;
    RETURN_IF_ERR();

    if ((dict_stat = PyDict_New()) == NULL) {
        free(sp);
        return NULL;
    }


#define MAKE_INT_ENTRY(name)  _addIntToDict(dict_stat, #name, sp->st_##name)
#define MAKE_LONG_LONG_ENTRY(name)  _addDb_seq_tToDict(dict_stat, #name, sp->st_##name)

    MAKE_INT_ENTRY(wait);
    MAKE_INT_ENTRY(nowait);
    MAKE_LONG_LONG_ENTRY(current);
    MAKE_LONG_LONG_ENTRY(value);
    MAKE_LONG_LONG_ENTRY(last_value);
    MAKE_LONG_LONG_ENTRY(min);
    MAKE_LONG_LONG_ENTRY(max);
    MAKE_INT_ENTRY(cache_size);
    MAKE_INT_ENTRY(flags);

#undef MAKE_INT_ENTRY
#undef MAKE_LONG_LONG_ENTRY

    free(sp);
    return dict_stat;
}


/* --------------------------------------------------------------------- */
/* Method definition tables and type objects */

static PyMethodDef DB_methods[] = {
    {"append",          (PyCFunction)DB_append,         METH_VARARGS|METH_KEYWORDS},
    {"associate",       (PyCFunction)DB_associate,      METH_VARARGS|METH_KEYWORDS},
    {"close",           (PyCFunction)DB_close,          METH_VARARGS},
#if (DBVER >= 47)
    {"compact",         (PyCFunction)DB_compact,        METH_VARARGS|METH_KEYWORDS},
#endif
    {"consume",         (PyCFunction)DB_consume,        METH_VARARGS|METH_KEYWORDS},
    {"consume_wait",    (PyCFunction)DB_consume_wait,   METH_VARARGS|METH_KEYWORDS},
    {"cursor",          (PyCFunction)DB_cursor,         METH_VARARGS|METH_KEYWORDS},
    {"delete",          (PyCFunction)DB_delete,         METH_VARARGS|METH_KEYWORDS},
    {"fd",              (PyCFunction)DB_fd,             METH_NOARGS},
#if (DBVER >= 46)
    {"exists",          (PyCFunction)DB_exists,
        METH_VARARGS|METH_KEYWORDS},
#endif
    {"get",             (PyCFunction)DB_get,            METH_VARARGS|METH_KEYWORDS},
    {"pget",            (PyCFunction)DB_pget,           METH_VARARGS|METH_KEYWORDS},
    {"get_both",        (PyCFunction)DB_get_both,       METH_VARARGS|METH_KEYWORDS},
    {"get_byteswapped", (PyCFunction)DB_get_byteswapped,METH_NOARGS},
    {"get_size",        (PyCFunction)DB_get_size,       METH_VARARGS|METH_KEYWORDS},
    {"get_type",        (PyCFunction)DB_get_type,       METH_NOARGS},
    {"join",            (PyCFunction)DB_join,           METH_VARARGS},
    {"key_range",       (PyCFunction)DB_key_range,      METH_VARARGS|METH_KEYWORDS},
    {"has_key",         (PyCFunction)DB_has_key,        METH_VARARGS|METH_KEYWORDS},
    {"items",           (PyCFunction)DB_items,          METH_VARARGS},
    {"keys",            (PyCFunction)DB_keys,           METH_VARARGS},
    {"open",            (PyCFunction)DB_open,           METH_VARARGS|METH_KEYWORDS},
    {"put",             (PyCFunction)DB_put,            METH_VARARGS|METH_KEYWORDS},
    {"remove",          (PyCFunction)DB_remove,         METH_VARARGS|METH_KEYWORDS},
    {"rename",          (PyCFunction)DB_rename,         METH_VARARGS},
    {"set_bt_minkey",   (PyCFunction)DB_set_bt_minkey,  METH_VARARGS},
    {"get_bt_minkey",   (PyCFunction)DB_get_bt_minkey,  METH_NOARGS},
    {"set_bt_compare",  (PyCFunction)DB_set_bt_compare, METH_O},
    {"set_cachesize",   (PyCFunction)DB_set_cachesize,  METH_VARARGS},
    {"get_cachesize",   (PyCFunction)DB_get_cachesize,  METH_NOARGS},
    {"set_dup_compare", (PyCFunction)DB_set_dup_compare, METH_O},
    {"set_encrypt",     (PyCFunction)DB_set_encrypt,    METH_VARARGS|METH_KEYWORDS},
    {"get_encrypt_flags", (PyCFunction)DB_get_encrypt_flags, METH_NOARGS},
    {"set_flags",       (PyCFunction)DB_set_flags,      METH_VARARGS},
    {"get_flags",       (PyCFunction)DB_get_flags,      METH_NOARGS},
    {"get_transactional", (PyCFunction)DB_get_transactional, METH_NOARGS},
    {"set_h_ffactor",   (PyCFunction)DB_set_h_ffactor,  METH_VARARGS},
    {"get_h_ffactor",   (PyCFunction)DB_get_h_ffactor,  METH_NOARGS},
    {"set_h_nelem",     (PyCFunction)DB_set_h_nelem,    METH_VARARGS},
    {"get_h_nelem",     (PyCFunction)DB_get_h_nelem,    METH_NOARGS},
    {"set_lorder",      (PyCFunction)DB_set_lorder,     METH_VARARGS},
    {"get_lorder",      (PyCFunction)DB_get_lorder,     METH_NOARGS},
    {"set_pagesize",    (PyCFunction)DB_set_pagesize,   METH_VARARGS},
    {"get_pagesize",    (PyCFunction)DB_get_pagesize,   METH_NOARGS},
    {"set_re_delim",    (PyCFunction)DB_set_re_delim,   METH_VARARGS},
    {"get_re_delim",    (PyCFunction)DB_get_re_delim,   METH_NOARGS},
    {"set_re_len",      (PyCFunction)DB_set_re_len,     METH_VARARGS},
    {"get_re_len",      (PyCFunction)DB_get_re_len,     METH_NOARGS},
    {"set_re_pad",      (PyCFunction)DB_set_re_pad,     METH_VARARGS},
    {"get_re_pad",      (PyCFunction)DB_get_re_pad,     METH_NOARGS},
    {"set_re_source",   (PyCFunction)DB_set_re_source,  METH_VARARGS},
    {"get_re_source",   (PyCFunction)DB_get_re_source,  METH_NOARGS},
    {"set_q_extentsize",(PyCFunction)DB_set_q_extentsize, METH_VARARGS},
    {"get_q_extentsize",(PyCFunction)DB_get_q_extentsize, METH_NOARGS},
    {"set_private",     (PyCFunction)DB_set_private,    METH_O},
    {"get_private",     (PyCFunction)DB_get_private,    METH_NOARGS},
#if (DBVER >= 46)
    {"set_priority",    (PyCFunction)DB_set_priority,   METH_VARARGS},
    {"get_priority",    (PyCFunction)DB_get_priority,   METH_NOARGS},
#endif
    {"get_dbname",      (PyCFunction)DB_get_dbname,     METH_NOARGS},
    {"get_open_flags",  (PyCFunction)DB_get_open_flags, METH_NOARGS},
    {"stat",            (PyCFunction)DB_stat,           METH_VARARGS|METH_KEYWORDS},
    {"stat_print",      (PyCFunction)DB_stat_print,
        METH_VARARGS|METH_KEYWORDS},
    {"sync",            (PyCFunction)DB_sync,           METH_VARARGS},
    {"truncate",        (PyCFunction)DB_truncate,       METH_VARARGS|METH_KEYWORDS},
    {"type",            (PyCFunction)DB_get_type,       METH_NOARGS},
    {"upgrade",         (PyCFunction)DB_upgrade,        METH_VARARGS},
    {"values",          (PyCFunction)DB_values,         METH_VARARGS},
    {"verify",          (PyCFunction)DB_verify,         METH_VARARGS|METH_KEYWORDS},
    {"set_get_returns_none",(PyCFunction)DB_set_get_returns_none,      METH_VARARGS},
    {NULL,      NULL}       /* sentinel */
};


/* We need this to support __contains__() */
static PySequenceMethods DB_sequence = {
    0, /* sq_length, mapping wins here */
    0, /* sq_concat */
    0, /* sq_repeat */
    0, /* sq_item */
    0, /* sq_slice */
    0, /* sq_ass_item */
    0, /* sq_ass_slice */
    (objobjproc)DB_contains, /* sq_contains */
    0, /* sq_inplace_concat */
    0, /* sq_inplace_repeat */
};

static PyMappingMethods DB_mapping = {
        DB_length,                   /*mp_length*/
        (binaryfunc)DB_subscript,    /*mp_subscript*/
        (objobjargproc)DB_ass_sub,   /*mp_ass_subscript*/
};


static PyMethodDef DBCursor_methods[] = {
    {"close",           (PyCFunction)DBC_close,         METH_NOARGS},
    {"count",           (PyCFunction)DBC_count,         METH_VARARGS},
    {"current",         (PyCFunction)DBC_current,       METH_VARARGS|METH_KEYWORDS},
    {"delete",          (PyCFunction)DBC_delete,        METH_VARARGS},
    {"dup",             (PyCFunction)DBC_dup,           METH_VARARGS},
    {"first",           (PyCFunction)DBC_first,         METH_VARARGS|METH_KEYWORDS},
    {"get",             (PyCFunction)DBC_get,           METH_VARARGS|METH_KEYWORDS},
    {"pget",            (PyCFunction)DBC_pget,          METH_VARARGS|METH_KEYWORDS},
    {"get_recno",       (PyCFunction)DBC_get_recno,     METH_NOARGS},
    {"last",            (PyCFunction)DBC_last,          METH_VARARGS|METH_KEYWORDS},
    {"next",            (PyCFunction)DBC_next,          METH_VARARGS|METH_KEYWORDS},
    {"prev",            (PyCFunction)DBC_prev,          METH_VARARGS|METH_KEYWORDS},
    {"put",             (PyCFunction)DBC_put,           METH_VARARGS|METH_KEYWORDS},
    {"set",             (PyCFunction)DBC_set,           METH_VARARGS|METH_KEYWORDS},
    {"set_range",       (PyCFunction)DBC_set_range,     METH_VARARGS|METH_KEYWORDS},
    {"get_both",        (PyCFunction)DBC_get_both,      METH_VARARGS},
    {"get_current_size",(PyCFunction)DBC_get_current_size, METH_NOARGS},
    {"set_both",        (PyCFunction)DBC_set_both,      METH_VARARGS},
    {"set_recno",       (PyCFunction)DBC_set_recno,     METH_VARARGS|METH_KEYWORDS},
    {"consume",         (PyCFunction)DBC_consume,       METH_VARARGS|METH_KEYWORDS},
    {"next_dup",        (PyCFunction)DBC_next_dup,      METH_VARARGS|METH_KEYWORDS},
    {"next_nodup",      (PyCFunction)DBC_next_nodup,    METH_VARARGS|METH_KEYWORDS},
#if (DBVER >= 46)
    {"prev_dup",        (PyCFunction)DBC_prev_dup,
        METH_VARARGS|METH_KEYWORDS},
#endif
    {"prev_nodup",      (PyCFunction)DBC_prev_nodup,    METH_VARARGS|METH_KEYWORDS},
    {"join_item",       (PyCFunction)DBC_join_item,     METH_VARARGS},
#if (DBVER >= 46)
    {"set_priority",    (PyCFunction)DBC_set_priority,
        METH_VARARGS|METH_KEYWORDS},
    {"get_priority",    (PyCFunction)DBC_get_priority, METH_NOARGS},
#endif
    {NULL,      NULL}       /* sentinel */
};


static PyMethodDef DBLogCursor_methods[] = {
    {"close",   (PyCFunction)DBLogCursor_close,     METH_NOARGS},
    {"current", (PyCFunction)DBLogCursor_current,   METH_NOARGS},
    {"first",   (PyCFunction)DBLogCursor_first,     METH_NOARGS},
    {"last",    (PyCFunction)DBLogCursor_last,      METH_NOARGS},
    {"next",    (PyCFunction)DBLogCursor_next,      METH_NOARGS},
    {"prev",    (PyCFunction)DBLogCursor_prev,      METH_NOARGS},
    {"set",     (PyCFunction)DBLogCursor_set,       METH_VARARGS},
    {NULL,      NULL}       /* sentinel */
};

#if (DBVER >= 52)
static PyMethodDef DBSite_methods[] = {
    {"get_config",  (PyCFunction)DBSite_get_config,
        METH_VARARGS | METH_KEYWORDS},
    {"set_config",  (PyCFunction)DBSite_set_config,
        METH_VARARGS | METH_KEYWORDS},
    {"remove",      (PyCFunction)DBSite_remove,     METH_NOARGS},
    {"get_eid",     (PyCFunction)DBSite_get_eid,    METH_NOARGS},
    {"get_address", (PyCFunction)DBSite_get_address,    METH_NOARGS},
    {"close",       (PyCFunction)DBSite_close,      METH_NOARGS},
    {NULL,      NULL}       /* sentinel */
};
#endif

static PyMethodDef DBEnv_methods[] = {
    {"close",           (PyCFunction)DBEnv_close,            METH_VARARGS},
    {"open",            (PyCFunction)DBEnv_open,             METH_VARARGS},
    {"remove",          (PyCFunction)DBEnv_remove,           METH_VARARGS},
    {"dbremove",        (PyCFunction)DBEnv_dbremove,         METH_VARARGS|METH_KEYWORDS},
    {"dbrename",        (PyCFunction)DBEnv_dbrename,         METH_VARARGS|METH_KEYWORDS},
#if (DBVER >= 46)
    {"set_thread_count", (PyCFunction)DBEnv_set_thread_count, METH_VARARGS},
    {"get_thread_count", (PyCFunction)DBEnv_get_thread_count, METH_NOARGS},
#endif
    {"set_encrypt",     (PyCFunction)DBEnv_set_encrypt,      METH_VARARGS|METH_KEYWORDS},
    {"get_encrypt_flags", (PyCFunction)DBEnv_get_encrypt_flags, METH_NOARGS},
    {"get_timeout",     (PyCFunction)DBEnv_get_timeout,
        METH_VARARGS|METH_KEYWORDS},
    {"set_timeout",     (PyCFunction)DBEnv_set_timeout,     METH_VARARGS|METH_KEYWORDS},
    {"set_shm_key",     (PyCFunction)DBEnv_set_shm_key,     METH_VARARGS},
    {"get_shm_key",     (PyCFunction)DBEnv_get_shm_key,     METH_NOARGS},
#if (DBVER >= 46)
    {"set_cache_max",   (PyCFunction)DBEnv_set_cache_max,   METH_VARARGS},
    {"get_cache_max",   (PyCFunction)DBEnv_get_cache_max,   METH_NOARGS},
#endif
    {"set_cachesize",   (PyCFunction)DBEnv_set_cachesize,   METH_VARARGS},
    {"get_cachesize",   (PyCFunction)DBEnv_get_cachesize,   METH_NOARGS},
    {"memp_trickle",    (PyCFunction)DBEnv_memp_trickle,    METH_VARARGS},
    {"memp_sync",       (PyCFunction)DBEnv_memp_sync,       METH_VARARGS},
    {"memp_stat",       (PyCFunction)DBEnv_memp_stat,
        METH_VARARGS|METH_KEYWORDS},
    {"memp_stat_print", (PyCFunction)DBEnv_memp_stat_print,
        METH_VARARGS|METH_KEYWORDS},
#if (DBVER >= 44)
    {"mutex_set_max",   (PyCFunction)DBEnv_mutex_set_max,   METH_VARARGS},
    {"mutex_get_max",   (PyCFunction)DBEnv_mutex_get_max,   METH_NOARGS},
    {"mutex_set_align", (PyCFunction)DBEnv_mutex_set_align, METH_VARARGS},
    {"mutex_get_align", (PyCFunction)DBEnv_mutex_get_align, METH_NOARGS},
    {"mutex_set_increment", (PyCFunction)DBEnv_mutex_set_increment,
        METH_VARARGS},
    {"mutex_get_increment", (PyCFunction)DBEnv_mutex_get_increment,
        METH_NOARGS},
    {"mutex_set_tas_spins", (PyCFunction)DBEnv_mutex_set_tas_spins,
        METH_VARARGS},
    {"mutex_get_tas_spins", (PyCFunction)DBEnv_mutex_get_tas_spins,
        METH_NOARGS},
    {"mutex_stat",      (PyCFunction)DBEnv_mutex_stat,      METH_VARARGS},
#if (DBVER >= 44)
    {"mutex_stat_print", (PyCFunction)DBEnv_mutex_stat_print,
                                         METH_VARARGS|METH_KEYWORDS},
#endif
#endif
    {"set_data_dir",    (PyCFunction)DBEnv_set_data_dir,    METH_VARARGS},
    {"get_data_dirs",   (PyCFunction)DBEnv_get_data_dirs,   METH_NOARGS},
    {"get_flags",       (PyCFunction)DBEnv_get_flags,       METH_NOARGS},
    {"set_flags",       (PyCFunction)DBEnv_set_flags,       METH_VARARGS},
#if (DBVER >= 47)
    {"log_set_config",  (PyCFunction)DBEnv_log_set_config,  METH_VARARGS},
    {"log_get_config",  (PyCFunction)DBEnv_log_get_config,  METH_VARARGS},
#endif
    {"set_lg_bsize",    (PyCFunction)DBEnv_set_lg_bsize,    METH_VARARGS},
    {"get_lg_bsize",    (PyCFunction)DBEnv_get_lg_bsize,    METH_NOARGS},
    {"set_lg_dir",      (PyCFunction)DBEnv_set_lg_dir,      METH_VARARGS},
    {"get_lg_dir",      (PyCFunction)DBEnv_get_lg_dir,      METH_NOARGS},
    {"set_lg_max",      (PyCFunction)DBEnv_set_lg_max,      METH_VARARGS},
    {"get_lg_max",      (PyCFunction)DBEnv_get_lg_max,      METH_NOARGS},
    {"set_lg_regionmax",(PyCFunction)DBEnv_set_lg_regionmax, METH_VARARGS},
    {"get_lg_regionmax",(PyCFunction)DBEnv_get_lg_regionmax, METH_NOARGS},
#if (DBVER >= 44)
    {"set_lg_filemode", (PyCFunction)DBEnv_set_lg_filemode, METH_VARARGS},
    {"get_lg_filemode", (PyCFunction)DBEnv_get_lg_filemode, METH_NOARGS},
#endif
#if (DBVER >= 47)
    {"set_lk_partitions", (PyCFunction)DBEnv_set_lk_partitions, METH_VARARGS},
    {"get_lk_partitions", (PyCFunction)DBEnv_get_lk_partitions, METH_NOARGS},
#endif
    {"set_lk_detect",   (PyCFunction)DBEnv_set_lk_detect,   METH_VARARGS},
    {"get_lk_detect",   (PyCFunction)DBEnv_get_lk_detect,   METH_NOARGS},
#if (DBVER < 45)
    {"set_lk_max",      (PyCFunction)DBEnv_set_lk_max,      METH_VARARGS},
#endif
    {"set_lk_max_locks", (PyCFunction)DBEnv_set_lk_max_locks, METH_VARARGS},
    {"get_lk_max_locks", (PyCFunction)DBEnv_get_lk_max_locks, METH_NOARGS},
    {"set_lk_max_lockers", (PyCFunction)DBEnv_set_lk_max_lockers, METH_VARARGS},
    {"get_lk_max_lockers", (PyCFunction)DBEnv_get_lk_max_lockers, METH_NOARGS},
    {"set_lk_max_objects", (PyCFunction)DBEnv_set_lk_max_objects, METH_VARARGS},
    {"get_lk_max_objects", (PyCFunction)DBEnv_get_lk_max_objects, METH_NOARGS},
    {"stat_print",          (PyCFunction)DBEnv_stat_print,
        METH_VARARGS|METH_KEYWORDS},
    {"set_mp_mmapsize", (PyCFunction)DBEnv_set_mp_mmapsize, METH_VARARGS},
    {"get_mp_mmapsize", (PyCFunction)DBEnv_get_mp_mmapsize, METH_NOARGS},
    {"set_tmp_dir",     (PyCFunction)DBEnv_set_tmp_dir,     METH_VARARGS},
    {"get_tmp_dir",     (PyCFunction)DBEnv_get_tmp_dir,     METH_NOARGS},
    {"txn_begin",       (PyCFunction)DBEnv_txn_begin,       METH_VARARGS|METH_KEYWORDS},
    {"txn_checkpoint",  (PyCFunction)DBEnv_txn_checkpoint,  METH_VARARGS},
    {"txn_stat",        (PyCFunction)DBEnv_txn_stat,        METH_VARARGS},
    {"txn_stat_print",  (PyCFunction)DBEnv_txn_stat_print,
        METH_VARARGS|METH_KEYWORDS},
    {"get_tx_max",      (PyCFunction)DBEnv_get_tx_max,      METH_NOARGS},
    {"get_tx_timestamp", (PyCFunction)DBEnv_get_tx_timestamp, METH_NOARGS},
    {"set_tx_max",      (PyCFunction)DBEnv_set_tx_max,      METH_VARARGS},
    {"set_tx_timestamp", (PyCFunction)DBEnv_set_tx_timestamp, METH_VARARGS},
    {"lock_detect",     (PyCFunction)DBEnv_lock_detect,     METH_VARARGS},
    {"lock_get",        (PyCFunction)DBEnv_lock_get,        METH_VARARGS},
    {"lock_id",         (PyCFunction)DBEnv_lock_id,         METH_NOARGS},
    {"lock_id_free",    (PyCFunction)DBEnv_lock_id_free,    METH_VARARGS},
    {"lock_put",        (PyCFunction)DBEnv_lock_put,        METH_VARARGS},
    {"lock_stat",       (PyCFunction)DBEnv_lock_stat,       METH_VARARGS},
    {"lock_stat_print", (PyCFunction)DBEnv_lock_stat_print,
        METH_VARARGS|METH_KEYWORDS},
    {"log_cursor",      (PyCFunction)DBEnv_log_cursor,      METH_NOARGS},
    {"log_file",        (PyCFunction)DBEnv_log_file,        METH_VARARGS},
#if (DBVER >= 44)
    {"log_printf",      (PyCFunction)DBEnv_log_printf,
        METH_VARARGS|METH_KEYWORDS},
#endif
    {"log_archive",     (PyCFunction)DBEnv_log_archive,     METH_VARARGS},
    {"log_flush",       (PyCFunction)DBEnv_log_flush,       METH_NOARGS},
    {"log_stat",        (PyCFunction)DBEnv_log_stat,        METH_VARARGS},
    {"log_stat_print",  (PyCFunction)DBEnv_log_stat_print,
        METH_VARARGS|METH_KEYWORDS},
#if (DBVER >= 44)
    {"fileid_reset",    (PyCFunction)DBEnv_fileid_reset,    METH_VARARGS|METH_KEYWORDS},
    {"lsn_reset",       (PyCFunction)DBEnv_lsn_reset,       METH_VARARGS|METH_KEYWORDS},
#endif
    {"set_get_returns_none",(PyCFunction)DBEnv_set_get_returns_none, METH_VARARGS},
    {"txn_recover",     (PyCFunction)DBEnv_txn_recover,     METH_NOARGS},
#if (DBVER < 48)
    {"set_rpc_server",  (PyCFunction)DBEnv_set_rpc_server,
        METH_VARARGS|METH_KEYWORDS},
#endif
    {"set_mp_max_openfd", (PyCFunction)DBEnv_set_mp_max_openfd, METH_VARARGS},
    {"get_mp_max_openfd", (PyCFunction)DBEnv_get_mp_max_openfd, METH_NOARGS},
    {"set_mp_max_write", (PyCFunction)DBEnv_set_mp_max_write, METH_VARARGS},
    {"get_mp_max_write", (PyCFunction)DBEnv_get_mp_max_write, METH_NOARGS},
    {"set_verbose",     (PyCFunction)DBEnv_set_verbose,     METH_VARARGS},
    {"get_verbose",     (PyCFunction)DBEnv_get_verbose,     METH_VARARGS},
    {"set_private",     (PyCFunction)DBEnv_set_private,     METH_O},
    {"get_private",     (PyCFunction)DBEnv_get_private,     METH_NOARGS},
    {"get_open_flags",  (PyCFunction)DBEnv_get_open_flags,  METH_NOARGS},
#if (DBVER >= 47)
    {"set_intermediate_dir_mode", (PyCFunction)DBEnv_set_intermediate_dir_mode,
        METH_VARARGS},
    {"get_intermediate_dir_mode", (PyCFunction)DBEnv_get_intermediate_dir_mode,
        METH_NOARGS},
#endif
#if (DBVER < 47)
    {"set_intermediate_dir", (PyCFunction)DBEnv_set_intermediate_dir,
        METH_VARARGS},
#endif
    {"rep_start",       (PyCFunction)DBEnv_rep_start,
        METH_VARARGS|METH_KEYWORDS},
    {"rep_set_transport", (PyCFunction)DBEnv_rep_set_transport, METH_VARARGS},
    {"rep_process_message", (PyCFunction)DBEnv_rep_process_message,
        METH_VARARGS},
#if (DBVER >= 46)
    {"rep_elect",       (PyCFunction)DBEnv_rep_elect,         METH_VARARGS},
#endif
#if (DBVER >= 44)
    {"rep_set_config",  (PyCFunction)DBEnv_rep_set_config,    METH_VARARGS},
    {"rep_get_config",  (PyCFunction)DBEnv_rep_get_config,    METH_VARARGS},
    {"rep_sync",        (PyCFunction)DBEnv_rep_sync,          METH_NOARGS},
#endif
#if (DBVER >= 45)
    {"rep_set_limit",   (PyCFunction)DBEnv_rep_set_limit,     METH_VARARGS},
    {"rep_get_limit",   (PyCFunction)DBEnv_rep_get_limit,     METH_NOARGS},
#endif
#if (DBVER >= 47)
    {"rep_set_request", (PyCFunction)DBEnv_rep_set_request,   METH_VARARGS},
    {"rep_get_request", (PyCFunction)DBEnv_rep_get_request,   METH_NOARGS},
#endif
#if (DBVER >= 45)
    {"set_event_notify", (PyCFunction)DBEnv_set_event_notify, METH_O},
#endif
#if (DBVER >= 45)
    {"rep_set_nsites", (PyCFunction)DBEnv_rep_set_nsites, METH_VARARGS},
    {"rep_get_nsites", (PyCFunction)DBEnv_rep_get_nsites, METH_NOARGS},
    {"rep_set_priority", (PyCFunction)DBEnv_rep_set_priority, METH_VARARGS},
    {"rep_get_priority", (PyCFunction)DBEnv_rep_get_priority, METH_NOARGS},
    {"rep_set_timeout", (PyCFunction)DBEnv_rep_set_timeout, METH_VARARGS},
    {"rep_get_timeout", (PyCFunction)DBEnv_rep_get_timeout, METH_VARARGS},
#endif
#if (DBVER >= 47)
    {"rep_set_clockskew", (PyCFunction)DBEnv_rep_set_clockskew, METH_VARARGS},
    {"rep_get_clockskew", (PyCFunction)DBEnv_rep_get_clockskew, METH_VARARGS},
#endif
    {"rep_stat", (PyCFunction)DBEnv_rep_stat,
        METH_VARARGS|METH_KEYWORDS},
    {"rep_stat_print", (PyCFunction)DBEnv_rep_stat_print,
        METH_VARARGS|METH_KEYWORDS},

#if (DBVER >= 45)
    {"repmgr_start", (PyCFunction)DBEnv_repmgr_start,
        METH_VARARGS|METH_KEYWORDS},
#if (DBVER < 52)
    {"repmgr_set_local_site", (PyCFunction)DBEnv_repmgr_set_local_site,
        METH_VARARGS|METH_KEYWORDS},
    {"repmgr_add_remote_site", (PyCFunction)DBEnv_repmgr_add_remote_site,
        METH_VARARGS|METH_KEYWORDS},
#endif
    {"repmgr_set_ack_policy", (PyCFunction)DBEnv_repmgr_set_ack_policy,
        METH_VARARGS},
    {"repmgr_get_ack_policy", (PyCFunction)DBEnv_repmgr_get_ack_policy,
        METH_NOARGS},
    {"repmgr_site_list", (PyCFunction)DBEnv_repmgr_site_list,
        METH_NOARGS},
#endif
#if (DBVER >= 46)
    {"repmgr_stat", (PyCFunction)DBEnv_repmgr_stat,
        METH_VARARGS|METH_KEYWORDS},
    {"repmgr_stat_print", (PyCFunction)DBEnv_repmgr_stat_print,
        METH_VARARGS|METH_KEYWORDS},
#endif
#if (DBVER >= 52)
    {"repmgr_site", (PyCFunction)DBEnv_repmgr_site,
        METH_VARARGS | METH_KEYWORDS},
    {"repmgr_site_by_eid",  (PyCFunction)DBEnv_repmgr_site_by_eid,
        METH_VARARGS | METH_KEYWORDS},
#endif
    {NULL,      NULL}       /* sentinel */
};


static PyMethodDef DBTxn_methods[] = {
    {"commit",          (PyCFunction)DBTxn_commit,      METH_VARARGS},
    {"prepare",         (PyCFunction)DBTxn_prepare,     METH_VARARGS},
    {"discard",         (PyCFunction)DBTxn_discard,     METH_NOARGS},
    {"abort",           (PyCFunction)DBTxn_abort,       METH_NOARGS},
    {"id",              (PyCFunction)DBTxn_id,          METH_NOARGS},
    {"set_timeout",     (PyCFunction)DBTxn_set_timeout,
        METH_VARARGS|METH_KEYWORDS},
#if (DBVER >= 44)
    {"set_name",        (PyCFunction)DBTxn_set_name, METH_VARARGS},
    {"get_name",        (PyCFunction)DBTxn_get_name, METH_NOARGS},
#endif
    {NULL,      NULL}       /* sentinel */
};


static PyMethodDef DBSequence_methods[] = {
    {"close",           (PyCFunction)DBSequence_close,          METH_VARARGS},
    {"get",             (PyCFunction)DBSequence_get,            METH_VARARGS|METH_KEYWORDS},
    {"get_dbp",         (PyCFunction)DBSequence_get_dbp,        METH_NOARGS},
    {"get_key",         (PyCFunction)DBSequence_get_key,        METH_NOARGS},
    {"initial_value",   (PyCFunction)DBSequence_initial_value,  METH_VARARGS},
    {"open",            (PyCFunction)DBSequence_open,           METH_VARARGS|METH_KEYWORDS},
    {"remove",          (PyCFunction)DBSequence_remove,         METH_VARARGS|METH_KEYWORDS},
    {"set_cachesize",   (PyCFunction)DBSequence_set_cachesize,  METH_VARARGS},
    {"get_cachesize",   (PyCFunction)DBSequence_get_cachesize,  METH_NOARGS},
    {"set_flags",       (PyCFunction)DBSequence_set_flags,      METH_VARARGS},
    {"get_flags",       (PyCFunction)DBSequence_get_flags,      METH_NOARGS},
    {"set_range",       (PyCFunction)DBSequence_set_range,      METH_VARARGS},
    {"get_range",       (PyCFunction)DBSequence_get_range,      METH_NOARGS},
    {"stat",            (PyCFunction)DBSequence_stat,           METH_VARARGS|METH_KEYWORDS},
    {"stat_print",      (PyCFunction)DBSequence_stat_print,
        METH_VARARGS|METH_KEYWORDS},
    {NULL,      NULL}       /* sentinel */
};


static PyObject*
DBEnv_db_home_get(DBEnvObject* self)
{
    const char *home = NULL;

    CHECK_ENV_NOT_CLOSED(self);

    MYDB_BEGIN_ALLOW_THREADS;
    self->db_env->get_home(self->db_env, &home);
    MYDB_END_ALLOW_THREADS;

    if (home == NULL) {
        RETURN_NONE();
    }
    return PyBytes_FromString(home);
}

static PyGetSetDef DBEnv_getsets[] = {
    {"db_home", (getter)DBEnv_db_home_get, NULL,},
    {NULL}
};


statichere PyTypeObject DB_Type = {
#if (PY_VERSION_HEX < 0x03000000)
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
#else
    PyVarObject_HEAD_INIT(NULL, 0)
#endif
    "DB",               /*tp_name*/
    sizeof(DBObject),   /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    /* methods */
    (destructor)DB_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    &DB_sequence,/*tp_as_sequence*/
    &DB_mapping,/*tp_as_mapping*/
    0,          /*tp_hash*/
    0,                  /* tp_call */
    0,                  /* tp_str */
    0,                  /* tp_getattro */
    0,          /* tp_setattro */
    0,                  /* tp_as_buffer */
#if (PY_VERSION_HEX < 0x03000000)
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS,      /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT,      /* tp_flags */
#endif
    0,          /* tp_doc */
    0,              /* tp_traverse */
    0,                  /* tp_clear */
    0,                  /* tp_richcompare */
    offsetof(DBObject, in_weakreflist),   /* tp_weaklistoffset */
    0,          /*tp_iter*/
    0,          /*tp_iternext*/
    DB_methods, /*tp_methods*/
    0, /*tp_members*/
};


statichere PyTypeObject DBCursor_Type = {
#if (PY_VERSION_HEX < 0x03000000)
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
#else
    PyVarObject_HEAD_INIT(NULL, 0)
#endif
    "DBCursor",         /*tp_name*/
    sizeof(DBCursorObject),  /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBCursor_dealloc,/*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
    0,          /*tp_call*/
    0,          /*tp_str*/
    0,          /*tp_getattro*/
    0,          /*tp_setattro*/
    0,          /*tp_as_buffer*/
#if (PY_VERSION_HEX < 0x03000000)
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS,      /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT,      /* tp_flags */
#endif
    0,          /* tp_doc */
    0,          /* tp_traverse */
    0,          /* tp_clear */
    0,          /* tp_richcompare */
    offsetof(DBCursorObject, in_weakreflist),   /* tp_weaklistoffset */
    0,          /*tp_iter*/
    0,          /*tp_iternext*/
    DBCursor_methods, /*tp_methods*/
    0,          /*tp_members*/
};


statichere PyTypeObject DBLogCursor_Type = {
#if (PY_VERSION_HEX < 0x03000000)
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
#else
    PyVarObject_HEAD_INIT(NULL, 0)
#endif
    "DBLogCursor",         /*tp_name*/
    sizeof(DBLogCursorObject),  /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBLogCursor_dealloc,/*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
    0,          /*tp_call*/
    0,          /*tp_str*/
    0,          /*tp_getattro*/
    0,          /*tp_setattro*/
    0,          /*tp_as_buffer*/
#if (PY_VERSION_HEX < 0x03000000)
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS,      /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT,      /* tp_flags */
#endif
    0,          /* tp_doc */
    0,          /* tp_traverse */
    0,          /* tp_clear */
    0,          /* tp_richcompare */
    offsetof(DBLogCursorObject, in_weakreflist),   /* tp_weaklistoffset */
    0,          /*tp_iter*/
    0,          /*tp_iternext*/
    DBLogCursor_methods, /*tp_methods*/
    0,          /*tp_members*/
};

#if (DBVER >= 52)
statichere PyTypeObject DBSite_Type = {
#if (PY_VERSION_HEX < 0x03000000)
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
#else
    PyVarObject_HEAD_INIT(NULL, 0)
#endif
    "DBSite",         /*tp_name*/
    sizeof(DBSiteObject),  /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBSite_dealloc,/*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
    0,          /*tp_call*/
    0,          /*tp_str*/
    0,          /*tp_getattro*/
    0,          /*tp_setattro*/
    0,          /*tp_as_buffer*/
#if (PY_VERSION_HEX < 0x03000000)
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS,      /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT,      /* tp_flags */
#endif
    0,          /* tp_doc */
    0,          /* tp_traverse */
    0,          /* tp_clear */
    0,          /* tp_richcompare */
    offsetof(DBSiteObject, in_weakreflist),   /* tp_weaklistoffset */
    0,          /*tp_iter*/
    0,          /*tp_iternext*/
    DBSite_methods, /*tp_methods*/
    0,          /*tp_members*/
};
#endif

statichere PyTypeObject DBEnv_Type = {
#if (PY_VERSION_HEX < 0x03000000)
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
#else
    PyVarObject_HEAD_INIT(NULL, 0)
#endif
    "DBEnv",            /*tp_name*/
    sizeof(DBEnvObject),    /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBEnv_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
    0,                  /* tp_call */
    0,                  /* tp_str */
    0,                  /* tp_getattro */
    0,          /* tp_setattro */
    0,                  /* tp_as_buffer */
#if (PY_VERSION_HEX < 0x03000000)
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS,      /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT,      /* tp_flags */
#endif
    0,          /* tp_doc */
    0,              /* tp_traverse */
    0,                  /* tp_clear */
    0,                  /* tp_richcompare */
    offsetof(DBEnvObject, in_weakreflist),   /* tp_weaklistoffset */
    0,          /* tp_iter */
    0,          /* tp_iternext */
    DBEnv_methods,      /* tp_methods */
    0,          /* tp_members */
    DBEnv_getsets,      /* tp_getsets */
};

statichere PyTypeObject DBTxn_Type = {
#if (PY_VERSION_HEX < 0x03000000)
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
#else
    PyVarObject_HEAD_INIT(NULL, 0)
#endif
    "DBTxn",    /*tp_name*/
    sizeof(DBTxnObject),  /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBTxn_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
    0,                  /* tp_call */
    0,                  /* tp_str */
    0,                  /* tp_getattro */
    0,          /* tp_setattro */
    0,                  /* tp_as_buffer */
#if (PY_VERSION_HEX < 0x03000000)
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS,      /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT,      /* tp_flags */
#endif
    0,          /* tp_doc */
    0,          /* tp_traverse */
    0,                  /* tp_clear */
    0,                  /* tp_richcompare */
    offsetof(DBTxnObject, in_weakreflist),   /* tp_weaklistoffset */
    0,          /*tp_iter*/
    0,          /*tp_iternext*/
    DBTxn_methods, /*tp_methods*/
    0,          /*tp_members*/
};


statichere PyTypeObject DBLock_Type = {
#if (PY_VERSION_HEX < 0x03000000)
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
#else
    PyVarObject_HEAD_INIT(NULL, 0)
#endif
    "DBLock",   /*tp_name*/
    sizeof(DBLockObject),  /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBLock_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
    0,                  /* tp_call */
    0,                  /* tp_str */
    0,                  /* tp_getattro */
    0,          /* tp_setattro */
    0,                  /* tp_as_buffer */
#if (PY_VERSION_HEX < 0x03000000)
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS,      /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT,      /* tp_flags */
#endif
    0,          /* tp_doc */
    0,              /* tp_traverse */
    0,                  /* tp_clear */
    0,                  /* tp_richcompare */
    offsetof(DBLockObject, in_weakreflist),   /* tp_weaklistoffset */
};

statichere PyTypeObject DBSequence_Type = {
#if (PY_VERSION_HEX < 0x03000000)
    PyObject_HEAD_INIT(NULL)
    0,                  /*ob_size*/
#else
    PyVarObject_HEAD_INIT(NULL, 0)
#endif
    "DBSequence",                   /*tp_name*/
    sizeof(DBSequenceObject),       /*tp_basicsize*/
    0,          /*tp_itemsize*/
    /* methods */
    (destructor)DBSequence_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash*/
    0,                  /* tp_call */
    0,                  /* tp_str */
    0,                  /* tp_getattro */
    0,          /* tp_setattro */
    0,                  /* tp_as_buffer */
#if (PY_VERSION_HEX < 0x03000000)
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS,      /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT,      /* tp_flags */
#endif
    0,          /* tp_doc */
    0,              /* tp_traverse */
    0,                  /* tp_clear */
    0,                  /* tp_richcompare */
    offsetof(DBSequenceObject, in_weakreflist),   /* tp_weaklistoffset */
    0,          /*tp_iter*/
    0,          /*tp_iternext*/
    DBSequence_methods, /*tp_methods*/
    0,          /*tp_members*/
};

/* --------------------------------------------------------------------- */
/* Module-level functions */

static PyObject*
DB_construct(PyObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* dbenvobj = NULL;
    int flags = 0;
    static char* kwnames[] = { "dbEnv", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:DB", kwnames,
                                     &dbenvobj, &flags))
        return NULL;
    if (dbenvobj == Py_None)
        dbenvobj = NULL;
    else if (dbenvobj && !DBEnvObject_Check(dbenvobj)) {
        makeTypeError("DBEnv", dbenvobj);
        return NULL;
    }

    return (PyObject* )newDBObject((DBEnvObject*)dbenvobj, flags);
}


static PyObject*
DBEnv_construct(PyObject* self, PyObject* args)
{
    int flags = 0;
    if (!PyArg_ParseTuple(args, "|i:DbEnv", &flags)) return NULL;
    return (PyObject* )newDBEnvObject(flags);
}

static PyObject*
DBSequence_construct(PyObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* dbobj;
    int flags = 0;
    static char* kwnames[] = { "db", "flags", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i:DBSequence", kwnames, &dbobj, &flags))
        return NULL;
    if (!DBObject_Check(dbobj)) {
        makeTypeError("DB", dbobj);
        return NULL;
    }
    return (PyObject* )newDBSequenceObject((DBObject*)dbobj, flags);
}

static char bsddb_version_doc[] =
"Returns a tuple of major, minor, and patch release numbers of the\n\
underlying DB library.";

static PyObject*
bsddb_version(PyObject* self)
{
    int major, minor, patch;

    /* This should be instantaneous, no need to release the GIL */
    db_version(&major, &minor, &patch);
    return Py_BuildValue("(iii)", major, minor, patch);
}

#if (DBVER >= 50)
static PyObject*
bsddb_version_full(PyObject* self)
{
    char *version_string;
    int family, release, major, minor, patch;

    /* This should be instantaneous, no need to release the GIL */
    version_string = db_full_version(&family, &release, &major, &minor, &patch);
    return Py_BuildValue("(siiiii)",
            version_string, family, release, major, minor, patch);
}
#endif


/* List of functions defined in the module */
static PyMethodDef bsddb_methods[] = {
    {"DB",          (PyCFunction)DB_construct,          METH_VARARGS | METH_KEYWORDS },
    {"DBEnv",       (PyCFunction)DBEnv_construct,       METH_VARARGS},
    {"DBSequence",  (PyCFunction)DBSequence_construct,  METH_VARARGS | METH_KEYWORDS },
    {"version",     (PyCFunction)bsddb_version,         METH_NOARGS, bsddb_version_doc},
#if (DBVER >= 50)
    {"full_version", (PyCFunction)bsddb_version_full, METH_NOARGS},
#endif
    {NULL,      NULL}       /* sentinel */
};


/* API structure */
static BSDDB_api bsddb_api;


/* --------------------------------------------------------------------- */
/* Module initialization */


/* Convenience routine to export an integer value.
 * Errors are silently ignored, for better or for worse...
 */
#define ADD_INT(dict, NAME)         _addIntToDict(dict, #NAME, NAME)

/*
** We can rename the module at import time, so the string allocated
** must be big enough, and any use of the name must use this particular
** string.
*/
#define MODULE_NAME_MAX_LEN     11
static char _bsddbModuleName[MODULE_NAME_MAX_LEN+1] = "_bsddb";

#if (PY_VERSION_HEX >= 0x03000000)
static struct PyModuleDef bsddbmodule = {
    PyModuleDef_HEAD_INIT,
    _bsddbModuleName,   /* Name of module */
    NULL,               /* module documentation, may be NULL */
    -1,                 /* size of per-interpreter state of the module,
                            or -1 if the module keeps state in global variables. */
    bsddb_methods,
    NULL,   /* Reload */
    NULL,   /* Traverse */
    NULL,   /* Clear */
    NULL    /* Free */
};
#endif


#if (PY_VERSION_HEX < 0x03000000)
DL_EXPORT(void) init_bsddb(void)
#else
PyMODINIT_FUNC  PyInit__bsddb(void)    /* Note the two underscores */
#endif
{
    PyObject* m;
    PyObject* d;
    PyObject* py_api;
    PyObject* pybsddb_version_s;
    PyObject* db_version_s;
    PyObject* cvsid_s;

#if (PY_VERSION_HEX < 0x03000000)
    pybsddb_version_s = PyString_FromString(PY_BSDDB_VERSION);
    db_version_s = PyString_FromString(DB_VERSION_STRING);
    cvsid_s = PyString_FromString(rcs_id);
#else
    /* This data should be ascii, so UTF-8 conversion is fine */
    pybsddb_version_s = PyUnicode_FromString(PY_BSDDB_VERSION);
    db_version_s = PyUnicode_FromString(DB_VERSION_STRING);
    cvsid_s = PyUnicode_FromString(rcs_id);
#endif

    /* Initialize object types */
    if ((PyType_Ready(&DB_Type) < 0)
        || (PyType_Ready(&DBCursor_Type) < 0)
        || (PyType_Ready(&DBLogCursor_Type) < 0)
        || (PyType_Ready(&DBEnv_Type) < 0)
        || (PyType_Ready(&DBTxn_Type) < 0)
        || (PyType_Ready(&DBLock_Type) < 0)
        || (PyType_Ready(&DBSequence_Type) < 0)
#if (DBVER >= 52)
        || (PyType_Ready(&DBSite_Type) < 0)
#endif
        ) {
#if (PY_VERSION_HEX < 0x03000000)
        return;
#else
        return NULL;
#endif
    }

    /* Create the module and add the functions */
#if (PY_VERSION_HEX < 0x03000000)
    m = Py_InitModule(_bsddbModuleName, bsddb_methods);
#else
    m=PyModule_Create(&bsddbmodule);
#endif
    if (m == NULL) {
#if (PY_VERSION_HEX < 0x03000000)
        return;
#else
        return NULL;
#endif
    }

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);
    PyDict_SetItemString(d, "__version__", pybsddb_version_s);
    PyDict_SetItemString(d, "cvsid", cvsid_s);
    PyDict_SetItemString(d, "DB_VERSION_STRING", db_version_s);
    Py_DECREF(pybsddb_version_s);
    pybsddb_version_s = NULL;
    Py_DECREF(cvsid_s);
    cvsid_s = NULL;
    Py_DECREF(db_version_s);
    db_version_s = NULL;

    ADD_INT(d, DB_VERSION_MAJOR);
    ADD_INT(d, DB_VERSION_MINOR);
    ADD_INT(d, DB_VERSION_PATCH);

    ADD_INT(d, DB_MAX_PAGES);
    ADD_INT(d, DB_MAX_RECORDS);

#if (DBVER < 48)
    ADD_INT(d, DB_RPCCLIENT);
#endif

#if (DBVER < 48)
    ADD_INT(d, DB_XA_CREATE);
#endif

    ADD_INT(d, DB_CREATE);
    ADD_INT(d, DB_NOMMAP);
    ADD_INT(d, DB_THREAD);
#if (DBVER >= 45)
    ADD_INT(d, DB_MULTIVERSION);
#endif

    ADD_INT(d, DB_FORCE);
    ADD_INT(d, DB_INIT_CDB);
    ADD_INT(d, DB_INIT_LOCK);
    ADD_INT(d, DB_INIT_LOG);
    ADD_INT(d, DB_INIT_MPOOL);
    ADD_INT(d, DB_INIT_TXN);
    ADD_INT(d, DB_JOINENV);

#if (DBVER >= 48)
    ADD_INT(d, DB_GID_SIZE);
#else
    ADD_INT(d, DB_XIDDATASIZE);
    /* Allow new code to work in old BDB releases */
    _addIntToDict(d, "DB_GID_SIZE", DB_XIDDATASIZE);
#endif

    ADD_INT(d, DB_RECOVER);
    ADD_INT(d, DB_RECOVER_FATAL);
    ADD_INT(d, DB_TXN_NOSYNC);
    ADD_INT(d, DB_USE_ENVIRON);
    ADD_INT(d, DB_USE_ENVIRON_ROOT);

    ADD_INT(d, DB_LOCKDOWN);
    ADD_INT(d, DB_PRIVATE);
    ADD_INT(d, DB_SYSTEM_MEM);

    ADD_INT(d, DB_TXN_SYNC);
    ADD_INT(d, DB_TXN_NOWAIT);

#if (DBVER >= 51)
    ADD_INT(d, DB_TXN_BULK);
#endif

#if (DBVER >= 48)
    ADD_INT(d, DB_CURSOR_BULK);
#endif

#if (DBVER >= 46)
    ADD_INT(d, DB_TXN_WAIT);
#endif

    ADD_INT(d, DB_EXCL);
    ADD_INT(d, DB_FCNTL_LOCKING);
    ADD_INT(d, DB_ODDFILESIZE);
    ADD_INT(d, DB_RDWRMASTER);
    ADD_INT(d, DB_RDONLY);
    ADD_INT(d, DB_TRUNCATE);
    ADD_INT(d, DB_EXTENT);
    ADD_INT(d, DB_CDB_ALLDB);
    ADD_INT(d, DB_VERIFY);
    ADD_INT(d, DB_UPGRADE);

    ADD_INT(d, DB_PRINTABLE);
    ADD_INT(d, DB_AGGRESSIVE);
    ADD_INT(d, DB_NOORDERCHK);
    ADD_INT(d, DB_ORDERCHKONLY);
    ADD_INT(d, DB_PR_PAGE);

    ADD_INT(d, DB_PR_RECOVERYTEST);
    ADD_INT(d, DB_SALVAGE);

    ADD_INT(d, DB_LOCK_NORUN);
    ADD_INT(d, DB_LOCK_DEFAULT);
    ADD_INT(d, DB_LOCK_OLDEST);
    ADD_INT(d, DB_LOCK_RANDOM);
    ADD_INT(d, DB_LOCK_YOUNGEST);
    ADD_INT(d, DB_LOCK_MAXLOCKS);
    ADD_INT(d, DB_LOCK_MINLOCKS);
    ADD_INT(d, DB_LOCK_MINWRITE);

    ADD_INT(d, DB_LOCK_EXPIRE);
    ADD_INT(d, DB_LOCK_MAXWRITE);

    _addIntToDict(d, "DB_LOCK_CONFLICT", 0);

    ADD_INT(d, DB_LOCK_DUMP);
    ADD_INT(d, DB_LOCK_GET);
    ADD_INT(d, DB_LOCK_INHERIT);
    ADD_INT(d, DB_LOCK_PUT);
    ADD_INT(d, DB_LOCK_PUT_ALL);
    ADD_INT(d, DB_LOCK_PUT_OBJ);

    ADD_INT(d, DB_LOCK_NG);
    ADD_INT(d, DB_LOCK_READ);
    ADD_INT(d, DB_LOCK_WRITE);
    ADD_INT(d, DB_LOCK_NOWAIT);
    ADD_INT(d, DB_LOCK_WAIT);
    ADD_INT(d, DB_LOCK_IWRITE);
    ADD_INT(d, DB_LOCK_IREAD);
    ADD_INT(d, DB_LOCK_IWR);
#if (DBVER < 44)
    ADD_INT(d, DB_LOCK_DIRTY);
#else
    ADD_INT(d, DB_LOCK_READ_UNCOMMITTED);  /* renamed in 4.4 */
#endif
    ADD_INT(d, DB_LOCK_WWRITE);

    ADD_INT(d, DB_LOCK_RECORD);
    ADD_INT(d, DB_LOCK_UPGRADE);
    ADD_INT(d, DB_LOCK_SWITCH);
    ADD_INT(d, DB_LOCK_UPGRADE_WRITE);

    ADD_INT(d, DB_LOCK_NOWAIT);
    ADD_INT(d, DB_LOCK_RECORD);
    ADD_INT(d, DB_LOCK_UPGRADE);

    ADD_INT(d, DB_LSTAT_ABORTED);
    ADD_INT(d, DB_LSTAT_FREE);
    ADD_INT(d, DB_LSTAT_HELD);

    ADD_INT(d, DB_LSTAT_PENDING);
    ADD_INT(d, DB_LSTAT_WAITING);

    ADD_INT(d, DB_ARCH_ABS);
    ADD_INT(d, DB_ARCH_DATA);
    ADD_INT(d, DB_ARCH_LOG);
    ADD_INT(d, DB_ARCH_REMOVE);

    ADD_INT(d, DB_BTREE);
    ADD_INT(d, DB_HASH);
    ADD_INT(d, DB_RECNO);
    ADD_INT(d, DB_QUEUE);
    ADD_INT(d, DB_UNKNOWN);

    ADD_INT(d, DB_DUP);
    ADD_INT(d, DB_DUPSORT);
    ADD_INT(d, DB_RECNUM);
    ADD_INT(d, DB_RENUMBER);
    ADD_INT(d, DB_REVSPLITOFF);
    ADD_INT(d, DB_SNAPSHOT);

    ADD_INT(d, DB_INORDER);

    ADD_INT(d, DB_JOIN_NOSORT);

    ADD_INT(d, DB_AFTER);
    ADD_INT(d, DB_APPEND);
    ADD_INT(d, DB_BEFORE);
#if (DBVER < 45)
    ADD_INT(d, DB_CACHED_COUNTS);
#endif

    ADD_INT(d, DB_CONSUME);
    ADD_INT(d, DB_CONSUME_WAIT);
    ADD_INT(d, DB_CURRENT);
    ADD_INT(d, DB_FAST_STAT);
    ADD_INT(d, DB_FIRST);
    ADD_INT(d, DB_FLUSH);
    ADD_INT(d, DB_GET_BOTH);
    ADD_INT(d, DB_GET_BOTH_RANGE);
    ADD_INT(d, DB_GET_RECNO);
    ADD_INT(d, DB_JOIN_ITEM);
    ADD_INT(d, DB_KEYFIRST);
    ADD_INT(d, DB_KEYLAST);
    ADD_INT(d, DB_LAST);
    ADD_INT(d, DB_NEXT);
    ADD_INT(d, DB_NEXT_DUP);
    ADD_INT(d, DB_NEXT_NODUP);
    ADD_INT(d, DB_NODUPDATA);
    ADD_INT(d, DB_NOOVERWRITE);
    ADD_INT(d, DB_NOSYNC);
    ADD_INT(d, DB_POSITION);
    ADD_INT(d, DB_PREV);
    ADD_INT(d, DB_PREV_NODUP);
#if (DBVER >= 46)
    ADD_INT(d, DB_PREV_DUP);
#endif
#if (DBVER < 45)
    ADD_INT(d, DB_RECORDCOUNT);
#endif
    ADD_INT(d, DB_SET);
    ADD_INT(d, DB_SET_RANGE);
    ADD_INT(d, DB_SET_RECNO);
    ADD_INT(d, DB_WRITECURSOR);

    ADD_INT(d, DB_OPFLAGS_MASK);
    ADD_INT(d, DB_RMW);
    ADD_INT(d, DB_DIRTY_READ);
    ADD_INT(d, DB_MULTIPLE);
    ADD_INT(d, DB_MULTIPLE_KEY);

#if (DBVER >= 44)
    ADD_INT(d, DB_IMMUTABLE_KEY);
    ADD_INT(d, DB_READ_UNCOMMITTED);    /* replaces DB_DIRTY_READ in 4.4 */
    ADD_INT(d, DB_READ_COMMITTED);
#endif

#if (DBVER >= 44)
    ADD_INT(d, DB_FREELIST_ONLY);
    ADD_INT(d, DB_FREE_SPACE);
#endif

    ADD_INT(d, DB_DONOTINDEX);

    ADD_INT(d, DB_KEYEMPTY);
    ADD_INT(d, DB_KEYEXIST);
    ADD_INT(d, DB_LOCK_DEADLOCK);
    ADD_INT(d, DB_LOCK_NOTGRANTED);
    ADD_INT(d, DB_NOSERVER);
#if (DBVER < 52)
    ADD_INT(d, DB_NOSERVER_HOME);
    ADD_INT(d, DB_NOSERVER_ID);
#endif
    ADD_INT(d, DB_NOTFOUND);
    ADD_INT(d, DB_OLD_VERSION);
    ADD_INT(d, DB_RUNRECOVERY);
    ADD_INT(d, DB_VERIFY_BAD);
    ADD_INT(d, DB_PAGE_NOTFOUND);
    ADD_INT(d, DB_SECONDARY_BAD);
    ADD_INT(d, DB_STAT_CLEAR);
    ADD_INT(d, DB_REGION_INIT);
    ADD_INT(d, DB_NOLOCKING);
    ADD_INT(d, DB_YIELDCPU);
    ADD_INT(d, DB_PANIC_ENVIRONMENT);
    ADD_INT(d, DB_NOPANIC);
    ADD_INT(d, DB_OVERWRITE);

    ADD_INT(d, DB_STAT_SUBSYSTEM);
    ADD_INT(d, DB_STAT_MEMP_HASH);
    ADD_INT(d, DB_STAT_LOCK_CONF);
    ADD_INT(d, DB_STAT_LOCK_LOCKERS);
    ADD_INT(d, DB_STAT_LOCK_OBJECTS);
    ADD_INT(d, DB_STAT_LOCK_PARAMS);

#if (DBVER >= 48)
    ADD_INT(d, DB_OVERWRITE_DUP);
#endif

#if (DBVER >= 47)
    ADD_INT(d, DB_FOREIGN_ABORT);
    ADD_INT(d, DB_FOREIGN_CASCADE);
    ADD_INT(d, DB_FOREIGN_NULLIFY);
#endif

#if (DBVER >= 44)
    ADD_INT(d, DB_REGISTER);
#endif

    ADD_INT(d, DB_EID_INVALID);
    ADD_INT(d, DB_EID_BROADCAST);

    ADD_INT(d, DB_TIME_NOTGRANTED);
    ADD_INT(d, DB_TXN_NOT_DURABLE);
    ADD_INT(d, DB_TXN_WRITE_NOSYNC);
    ADD_INT(d, DB_DIRECT_DB);
    ADD_INT(d, DB_INIT_REP);
    ADD_INT(d, DB_ENCRYPT);
    ADD_INT(d, DB_CHKSUM);

#if (DBVER < 47)
    ADD_INT(d, DB_LOG_AUTOREMOVE);
    ADD_INT(d, DB_DIRECT_LOG);
#endif

#if (DBVER >= 47)
    ADD_INT(d, DB_LOG_DIRECT);
    ADD_INT(d, DB_LOG_DSYNC);
    ADD_INT(d, DB_LOG_IN_MEMORY);
    ADD_INT(d, DB_LOG_AUTO_REMOVE);
    ADD_INT(d, DB_LOG_ZERO);
#endif

#if (DBVER >= 44)
    ADD_INT(d, DB_DSYNC_DB);
#endif

#if (DBVER >= 45)
    ADD_INT(d, DB_TXN_SNAPSHOT);
#endif

    ADD_INT(d, DB_VERB_DEADLOCK);
#if (DBVER >= 46)
    ADD_INT(d, DB_VERB_FILEOPS);
    ADD_INT(d, DB_VERB_FILEOPS_ALL);
#endif
    ADD_INT(d, DB_VERB_RECOVERY);
#if (DBVER >= 44)
    ADD_INT(d, DB_VERB_REGISTER);
#endif
    ADD_INT(d, DB_VERB_REPLICATION);
    ADD_INT(d, DB_VERB_WAITSFOR);

#if (DBVER >= 50)
    ADD_INT(d, DB_VERB_REP_SYSTEM);
#endif

#if (DBVER >= 47)
    ADD_INT(d, DB_VERB_REP_ELECT);
    ADD_INT(d, DB_VERB_REP_LEASE);
    ADD_INT(d, DB_VERB_REP_MISC);
    ADD_INT(d, DB_VERB_REP_MSGS);
    ADD_INT(d, DB_VERB_REP_SYNC);
    ADD_INT(d, DB_VERB_REPMGR_CONNFAIL);
    ADD_INT(d, DB_VERB_REPMGR_MISC);
#endif

#if (DBVER >= 45)
    ADD_INT(d, DB_EVENT_PANIC);
    ADD_INT(d, DB_EVENT_REP_CLIENT);
#if (DBVER >= 46)
    ADD_INT(d, DB_EVENT_REP_ELECTED);
#endif
    ADD_INT(d, DB_EVENT_REP_MASTER);
    ADD_INT(d, DB_EVENT_REP_NEWMASTER);
#if (DBVER >= 46)
    ADD_INT(d, DB_EVENT_REP_PERM_FAILED);
#endif
    ADD_INT(d, DB_EVENT_REP_STARTUPDONE);
    ADD_INT(d, DB_EVENT_WRITE_FAILED);
#endif

#if (DBVER >= 50)
    ADD_INT(d, DB_REPMGR_CONF_ELECTIONS);
    ADD_INT(d, DB_EVENT_REP_MASTER_FAILURE);
    ADD_INT(d, DB_EVENT_REP_DUPMASTER);
    ADD_INT(d, DB_EVENT_REP_ELECTION_FAILED);
#endif
#if (DBVER >= 48)
    ADD_INT(d, DB_EVENT_REG_ALIVE);
    ADD_INT(d, DB_EVENT_REG_PANIC);
#endif

#if (DBVER >=52)
    ADD_INT(d, DB_EVENT_REP_SITE_ADDED);
    ADD_INT(d, DB_EVENT_REP_SITE_REMOVED);
    ADD_INT(d, DB_EVENT_REP_LOCAL_SITE_REMOVED);
    ADD_INT(d, DB_EVENT_REP_CONNECT_BROKEN);
    ADD_INT(d, DB_EVENT_REP_CONNECT_ESTD);
    ADD_INT(d, DB_EVENT_REP_CONNECT_TRY_FAILED);
    ADD_INT(d, DB_EVENT_REP_INIT_DONE);

    ADD_INT(d, DB_MEM_LOCK);
    ADD_INT(d, DB_MEM_LOCKOBJECT);
    ADD_INT(d, DB_MEM_LOCKER);
    ADD_INT(d, DB_MEM_LOGID);
    ADD_INT(d, DB_MEM_TRANSACTION);
    ADD_INT(d, DB_MEM_THREAD);

    ADD_INT(d, DB_BOOTSTRAP_HELPER);
    ADD_INT(d, DB_GROUP_CREATOR);
    ADD_INT(d, DB_LEGACY);
    ADD_INT(d, DB_LOCAL_SITE);
    ADD_INT(d, DB_REPMGR_PEER);
#endif

    ADD_INT(d, DB_REP_DUPMASTER);
    ADD_INT(d, DB_REP_HOLDELECTION);
#if (DBVER >= 44)
    ADD_INT(d, DB_REP_IGNORE);
    ADD_INT(d, DB_REP_JOIN_FAILURE);
#endif
    ADD_INT(d, DB_REP_ISPERM);
    ADD_INT(d, DB_REP_NOTPERM);
    ADD_INT(d, DB_REP_NEWSITE);

    ADD_INT(d, DB_REP_MASTER);
    ADD_INT(d, DB_REP_CLIENT);

    ADD_INT(d, DB_REP_PERMANENT);

#if (DBVER >= 44)
#if (DBVER >= 50)
    ADD_INT(d, DB_REP_CONF_AUTOINIT);
#else
    ADD_INT(d, DB_REP_CONF_NOAUTOINIT);
#endif /* 5.0 */
#endif /* 4.4 */
#if (DBVER >= 44)
    ADD_INT(d, DB_REP_CONF_DELAYCLIENT);
    ADD_INT(d, DB_REP_CONF_BULK);
    ADD_INT(d, DB_REP_CONF_NOWAIT);
    ADD_INT(d, DB_REP_ANYWHERE);
    ADD_INT(d, DB_REP_REREQUEST);
#endif

    ADD_INT(d, DB_REP_NOBUFFER);

#if (DBVER >= 46)
    ADD_INT(d, DB_REP_LEASE_EXPIRED);
    ADD_INT(d, DB_IGNORE_LEASE);
#endif

#if (DBVER >= 47)
    ADD_INT(d, DB_REP_CONF_LEASE);
    ADD_INT(d, DB_REPMGR_CONF_2SITE_STRICT);
#endif

#if (DBVER >= 45)
    ADD_INT(d, DB_REP_ELECTION);

    ADD_INT(d, DB_REP_ACK_TIMEOUT);
    ADD_INT(d, DB_REP_CONNECTION_RETRY);
    ADD_INT(d, DB_REP_ELECTION_TIMEOUT);
    ADD_INT(d, DB_REP_ELECTION_RETRY);
#endif
#if (DBVER >= 46)
    ADD_INT(d, DB_REP_CHECKPOINT_DELAY);
    ADD_INT(d, DB_REP_FULL_ELECTION_TIMEOUT);
    ADD_INT(d, DB_REP_LEASE_TIMEOUT);
#endif
#if (DBVER >= 47)
    ADD_INT(d, DB_REP_HEARTBEAT_MONITOR);
    ADD_INT(d, DB_REP_HEARTBEAT_SEND);
#endif

#if (DBVER >= 45)
    ADD_INT(d, DB_REPMGR_PEER);
    ADD_INT(d, DB_REPMGR_ACKS_ALL);
    ADD_INT(d, DB_REPMGR_ACKS_ALL_PEERS);
    ADD_INT(d, DB_REPMGR_ACKS_NONE);
    ADD_INT(d, DB_REPMGR_ACKS_ONE);
    ADD_INT(d, DB_REPMGR_ACKS_ONE_PEER);
    ADD_INT(d, DB_REPMGR_ACKS_QUORUM);
    ADD_INT(d, DB_REPMGR_CONNECTED);
    ADD_INT(d, DB_REPMGR_DISCONNECTED);
    ADD_INT(d, DB_STAT_ALL);
#endif

#if (DBVER >= 51)
    ADD_INT(d, DB_REPMGR_ACKS_ALL_AVAILABLE);
#endif

#if (DBVER >= 48)
    ADD_INT(d, DB_REP_CONF_INMEM);
#endif

    ADD_INT(d, DB_TIMEOUT);

#if (DBVER >= 50)
    ADD_INT(d, DB_FORCESYNC);
#endif

#if (DBVER >= 48)
    ADD_INT(d, DB_FAILCHK);
#endif

#if (DBVER >= 51)
    ADD_INT(d, DB_HOTBACKUP_IN_PROGRESS);
#endif

    ADD_INT(d, DB_BUFFER_SMALL);
    ADD_INT(d, DB_SEQ_DEC);
    ADD_INT(d, DB_SEQ_INC);
    ADD_INT(d, DB_SEQ_WRAP);

#if (DBVER < 47)
    ADD_INT(d, DB_LOG_INMEMORY);
    ADD_INT(d, DB_DSYNC_LOG);
#endif

    ADD_INT(d, DB_ENCRYPT_AES);
    ADD_INT(d, DB_AUTO_COMMIT);
    ADD_INT(d, DB_PRIORITY_VERY_LOW);
    ADD_INT(d, DB_PRIORITY_LOW);
    ADD_INT(d, DB_PRIORITY_DEFAULT);
    ADD_INT(d, DB_PRIORITY_HIGH);
    ADD_INT(d, DB_PRIORITY_VERY_HIGH);

#if (DBVER >= 46)
    ADD_INT(d, DB_PRIORITY_UNCHANGED);
#endif

    ADD_INT(d, EINVAL);
    ADD_INT(d, EACCES);
    ADD_INT(d, ENOSPC);
    ADD_INT(d, ENOMEM);
    ADD_INT(d, EAGAIN);
    ADD_INT(d, EBUSY);
    ADD_INT(d, EEXIST);
    ADD_INT(d, ENOENT);
    ADD_INT(d, EPERM);

    ADD_INT(d, DB_SET_LOCK_TIMEOUT);
    ADD_INT(d, DB_SET_TXN_TIMEOUT);

#if (DBVER >= 48)
    ADD_INT(d, DB_SET_REG_TIMEOUT);
#endif

    /* The exception name must be correct for pickled exception *
     * objects to unpickle properly.                            */
#ifdef PYBSDDB_STANDALONE  /* different value needed for standalone pybsddb */
#define PYBSDDB_EXCEPTION_BASE  "bsddb3.db."
#else
#define PYBSDDB_EXCEPTION_BASE  "bsddb.db."
#endif

    /* All the rest of the exceptions derive only from DBError */
#define MAKE_EX(name)   name = PyErr_NewException(PYBSDDB_EXCEPTION_BASE #name, DBError, NULL); \
                        PyDict_SetItemString(d, #name, name)

    /* The base exception class is DBError */
    DBError = NULL;     /* used in MAKE_EX so that it derives from nothing */
    MAKE_EX(DBError);

#if (PY_VERSION_HEX < 0x03000000)
    /* Some magic to make DBNotFoundError and DBKeyEmptyError derive
     * from both DBError and KeyError, since the API only supports
     * using one base class. */
    PyDict_SetItemString(d, "KeyError", PyExc_KeyError);
    PyRun_String("class DBNotFoundError(DBError, KeyError): pass\n"
                 "class DBKeyEmptyError(DBError, KeyError): pass",
                 Py_file_input, d, d);
    DBNotFoundError = PyDict_GetItemString(d, "DBNotFoundError");
    DBKeyEmptyError = PyDict_GetItemString(d, "DBKeyEmptyError");
    PyDict_DelItemString(d, "KeyError");
#else
    /* Since Python 2.5, PyErr_NewException() accepts a tuple, to be able to
    ** derive from several classes. We use this new API only for Python 3.0,
    ** though.
    */
    {
        PyObject* bases;

        bases = PyTuple_Pack(2, DBError, PyExc_KeyError);

#define MAKE_EX2(name)   name = PyErr_NewException(PYBSDDB_EXCEPTION_BASE #name, bases, NULL); \
                         PyDict_SetItemString(d, #name, name)
        MAKE_EX2(DBNotFoundError);
        MAKE_EX2(DBKeyEmptyError);

#undef MAKE_EX2

        Py_XDECREF(bases);
    }
#endif

    MAKE_EX(DBCursorClosedError);
    MAKE_EX(DBKeyExistError);
    MAKE_EX(DBLockDeadlockError);
    MAKE_EX(DBLockNotGrantedError);
    MAKE_EX(DBOldVersionError);
    MAKE_EX(DBRunRecoveryError);
    MAKE_EX(DBVerifyBadError);
    MAKE_EX(DBNoServerError);
#if (DBVER < 52)
    MAKE_EX(DBNoServerHomeError);
    MAKE_EX(DBNoServerIDError);
#endif
    MAKE_EX(DBPageNotFoundError);
    MAKE_EX(DBSecondaryBadError);

    MAKE_EX(DBInvalidArgError);
    MAKE_EX(DBAccessError);
    MAKE_EX(DBNoSpaceError);
    MAKE_EX(DBNoMemoryError);
    MAKE_EX(DBAgainError);
    MAKE_EX(DBBusyError);
    MAKE_EX(DBFileExistsError);
    MAKE_EX(DBNoSuchFileError);
    MAKE_EX(DBPermissionsError);

    MAKE_EX(DBRepHandleDeadError);
#if (DBVER >= 44)
    MAKE_EX(DBRepLockoutError);
#endif

    MAKE_EX(DBRepUnavailError);

#if (DBVER >= 46)
    MAKE_EX(DBRepLeaseExpiredError);
#endif

#if (DBVER >= 47)
        MAKE_EX(DBForeignConflictError);
#endif

#undef MAKE_EX

    /* Initialise the C API structure and add it to the module */
    bsddb_api.api_version      = PYBSDDB_API_VERSION;
    bsddb_api.db_type          = &DB_Type;
    bsddb_api.dbcursor_type    = &DBCursor_Type;
    bsddb_api.dblogcursor_type = &DBLogCursor_Type;
    bsddb_api.dbenv_type       = &DBEnv_Type;
    bsddb_api.dbtxn_type       = &DBTxn_Type;
    bsddb_api.dblock_type      = &DBLock_Type;
    bsddb_api.dbsequence_type  = &DBSequence_Type;
    bsddb_api.makeDBError      = makeDBError;

    /*
    ** Capsules exist from Python 2.7 and 3.1.
    ** We don't support Python 3.0 anymore, so...
    ** #if (PY_VERSION_HEX < ((PY_MAJOR_VERSION < 3) ? 0x02070000 : 0x03020000))
    */
#if (PY_VERSION_HEX < 0x02070000)
    py_api = PyCObject_FromVoidPtr((void*)&bsddb_api, NULL);
#else
    {
        /*
        ** The data must outlive the call!!. So, the static definition.
        ** The buffer must be big enough...
        */
        static char py_api_name[MODULE_NAME_MAX_LEN+10];

        strcpy(py_api_name, _bsddbModuleName);
        strcat(py_api_name, ".api");

        py_api = PyCapsule_New((void*)&bsddb_api, py_api_name, NULL);
    }
#endif

    /* Check error control */
    /*
    ** PyErr_NoMemory();
    ** py_api = NULL;
    */

    if (py_api) {
        PyDict_SetItemString(d, "api", py_api);
        Py_DECREF(py_api);
    } else { /* Something bad happened */
        PyErr_WriteUnraisable(m);
        if(PyErr_Warn(PyExc_RuntimeWarning,
                "_bsddb/_pybsddb C API will be not available")) {
            PyErr_WriteUnraisable(m);
        }
        PyErr_Clear();
    }

    /* Check for errors */
    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_FatalError("can't initialize module _bsddb/_pybsddb");
        Py_DECREF(m);
        m = NULL;
    }
#if (PY_VERSION_HEX < 0x03000000)
    return;
#else
    return m;
#endif
}

/* allow this module to be named _pybsddb so that it can be installed
 * and imported on top of python >= 2.3 that includes its own older
 * copy of the library named _bsddb without importing the old version. */
#if (PY_VERSION_HEX < 0x03000000)
DL_EXPORT(void) init_pybsddb(void)
#else
PyMODINIT_FUNC PyInit__pybsddb(void)  /* Note the two underscores */
#endif
{
    strncpy(_bsddbModuleName, "_pybsddb", MODULE_NAME_MAX_LEN);
#if (PY_VERSION_HEX < 0x03000000)
    init_bsddb();
#else
    return PyInit__bsddb();   /* Note the two underscores */
#endif
}
