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
 * to compile with BerkeleyDB versions 3.2 through 4.2.
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
 * Gregory P. Smith <greg@krypto.org> is once again the maintainer.
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
 * This module contains 6 types:
 *
 * DB           (Database)
 * DBCursor     (Database Cursor)
 * DBEnv        (database environment)
 * DBTxn        (An explicit database transaction)
 * DBLock       (A lock handle)
 * DBSequence   (Sequence)
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

/*
 * Work to split it up into a separate header and to add a C API was
 * contributed by Duncan Grisby <duncan@tideway.com>.   See here:
 *  http://sourceforge.net/tracker/index.php?func=detail&aid=1551895&group_id=13900&atid=313900
 */

/* --------------------------------------------------------------------- */

#ifndef _BSDDB_H_
#define _BSDDB_H_

#include <db.h>


/* 40 = 4.0, 33 = 3.3; this will break if the minor revision is > 9 */
#define DBVER (DB_VERSION_MAJOR * 10 + DB_VERSION_MINOR)
#if DB_VERSION_MINOR > 9
#error "eek! DBVER can't handle minor versions > 9"
#endif

#define PY_BSDDB_VERSION "4.6.0"

/* Python object definitions */

struct behaviourFlags {
    /* What is the default behaviour when DB->get or DBCursor->get returns a
       DB_NOTFOUND || DB_KEYEMPTY error?  Return None or raise an exception? */
    unsigned int getReturnsNone : 1;
    /* What is the default behaviour for DBCursor.set* methods when DBCursor->get
     * returns a DB_NOTFOUND || DB_KEYEMPTY  error?  Return None or raise? */
    unsigned int cursorSetReturnsNone : 1;
};


typedef struct {
    PyObject_HEAD
    DB_ENV*     db_env;
    u_int32_t   flags;             /* saved flags from open() */
    int         closed;
    struct behaviourFlags moduleFlags;
    PyObject        *in_weakreflist; /* List of weak references */
} DBEnvObject;


typedef struct {
    PyObject_HEAD
    DB*             db;
    DBEnvObject*    myenvobj;  /* PyObject containing the DB_ENV */
    u_int32_t       flags;     /* saved flags from open() */
    u_int32_t       setflags;  /* saved flags from set_flags() */
    int             haveStat;
    struct behaviourFlags moduleFlags;
#if (DBVER >= 33)
    PyObject*       associateCallback;
    PyObject*       btCompareCallback;
    int             primaryDBType;
#endif
    PyObject        *in_weakreflist; /* List of weak references */
} DBObject;


typedef struct {
    PyObject_HEAD
    DBC*            dbc;
    DBObject*       mydb;
    PyObject        *in_weakreflist; /* List of weak references */
} DBCursorObject;


typedef struct {
    PyObject_HEAD
    DB_TXN*         txn;
    PyObject        *env;
    PyObject        *in_weakreflist; /* List of weak references */
} DBTxnObject;


typedef struct {
    PyObject_HEAD
    DB_LOCK         lock;
    PyObject        *in_weakreflist; /* List of weak references */
} DBLockObject;


#if (DBVER >= 43)
typedef struct {
    PyObject_HEAD
    DB_SEQUENCE*     sequence;
    DBObject*        mydb;
    PyObject        *in_weakreflist; /* List of weak references */
} DBSequenceObject;
static PyTypeObject DBSequence_Type;
#endif


/* API structure for use by C code */

/* To access the structure from an external module, use code like the
   following (error checking missed out for clarity):

     BSDDB_api* bsddb_api;
     PyObject*  mod;
     PyObject*  cobj;

     mod  = PyImport_ImportModule("bsddb._bsddb");
     // Use "bsddb3._pybsddb" if you're using the standalone pybsddb add-on.
     cobj = PyObject_GetAttrString(mod, "api");
     api  = (BSDDB_api*)PyCObject_AsVoidPtr(cobj);
     Py_DECREF(cobj);
     Py_DECREF(mod);

   The structure's members must not be changed.
*/

typedef struct {
    /* Type objects */
    PyTypeObject* db_type;
    PyTypeObject* dbcursor_type;
    PyTypeObject* dbenv_type;
    PyTypeObject* dbtxn_type;
    PyTypeObject* dblock_type;
#if (DBVER >= 43)
    PyTypeObject* dbsequence_type;
#endif

    /* Functions */
    int (*makeDBError)(int err);

} BSDDB_api;


#ifndef COMPILING_BSDDB_C

/* If not inside _bsddb.c, define type check macros that use the api
   structure.  The calling code must have a value named bsddb_api
   pointing to the api structure.
*/

#define DBObject_Check(v)       ((v)->ob_type == bsddb_api->db_type)
#define DBCursorObject_Check(v) ((v)->ob_type == bsddb_api->dbcursor_type)
#define DBEnvObject_Check(v)    ((v)->ob_type == bsddb_api->dbenv_type)
#define DBTxnObject_Check(v)    ((v)->ob_type == bsddb_api->dbtxn_type)
#define DBLockObject_Check(v)   ((v)->ob_type == bsddb_api->dblock_type)
#if (DBVER >= 43)
#define DBSequenceObject_Check(v)  ((v)->ob_type == bsddb_api->dbsequence_type)
#endif

#endif // COMPILING_BSDDB_C


#endif // _BSDDB_H_
