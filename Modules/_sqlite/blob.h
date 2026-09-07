#ifndef PYSQLITE_BLOB_H
#define PYSQLITE_BLOB_H

#include "Python.h"
#include "sqlite3.h"
#include "connection.h"

#define BLOB_SEEK_START 0
#define BLOB_SEEK_CUR   1
#define BLOB_SEEK_END   2

typedef struct {
    PyObject_HEAD
    pysqlite_Connection *connection;
    sqlite3_blob *blob;
    int offset;

    PyObject *in_weakreflist;
} pysqlite_Blob;

int pysqlite_blob_setup_types(PyObject *mod);
void pysqlite_close_all_blobs(pysqlite_Connection *self);

#endif
