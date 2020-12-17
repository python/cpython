/*
 * tdbcmysql.c --
 *
 *	Bridge between TDBC (Tcl DataBase Connectivity) and MYSQL.
 *
 * Copyright (c) 2008, 2009 by Kevin B. Kenny.
 *
 * Please refer to the file, 'license.terms' for the conditions on
 * redistribution of this file and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: $
 *
 *-----------------------------------------------------------------------------
 */

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <tcl.h>
#include <tclOO.h>
#include <tdbc.h>

#include <stdio.h>
#include <string.h>

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include "int2ptr_ptr2int.h"

#include "fakemysql.h"

/* Static data contained in this file */

TCL_DECLARE_MUTEX(mysqlMutex);	/* Mutex protecting the global environment
				 * and its reference count */

static int mysqlRefCount = 0;	/* Reference count on the global environment */
Tcl_LoadHandle mysqlLoadHandle = NULL;
				/* Handle to the MySQL library */
unsigned long mysqlClientVersion;
				/* Version number of MySQL */

/*
 * Objects to create within the literal pool
 */

const char* LiteralValues[] = {
    "",
    "0",
    "1",
    "direction",
    "in",
    "inout",
    "name",
    "nullable",
    "out",
    "precision",
    "scale",
    "type",
    NULL
};
enum LiteralIndex {
    LIT_EMPTY,
    LIT_0,
    LIT_1,
    LIT_DIRECTION,
    LIT_IN,
    LIT_INOUT,
    LIT_NAME,
    LIT_NULLABLE,
    LIT_OUT,
    LIT_PRECISION,
    LIT_SCALE,
    LIT_TYPE,
    LIT__END
};

/*
 * Structure that holds per-interpreter data for the MYSQL package.
 */

typedef struct PerInterpData {
    int refCount;		/* Reference count */
    Tcl_Obj* literals[LIT__END];
				/* Literal pool */
    Tcl_HashTable typeNumHash;	/* Lookup table for type numbers */
} PerInterpData;
#define IncrPerInterpRefCount(x)  \
    do {			  \
	++((x)->refCount);	  \
    } while(0)
#define DecrPerInterpRefCount(x)		\
    do {					\
	PerInterpData* _pidata = x;		\
	if ((--(_pidata->refCount)) <= 0) {	\
	    DeletePerInterpData(_pidata);	\
	}					\
    } while(0)

/*
 * Structure that carries the data for an MYSQL connection
 *
 * 	The ConnectionData structure is refcounted to simplify the
 *	destruction of statements associated with a connection.
 *	When a connection is destroyed, the subordinate namespace that
 *	contains its statements is taken down, destroying them. It's
 *	not safe to take down the ConnectionData until nothing is
 *	referring to it, which avoids taking down the hDBC until the
 *	other objects that refer to it vanish.
 */

typedef struct ConnectionData {
    int refCount;		/* Reference count. */
    PerInterpData* pidata;	/* Per-interpreter data */
    MYSQL* mysqlPtr;		/* MySql connection handle */
    unsigned int nCollations;	/* Number of collations defined */
    int* collationSizes;	/* Character lengths indexed by collation ID */
    int flags;
} ConnectionData;

/*
 * Flags for the state of an MYSQL connection
 */

#define CONN_FLAG_AUTOCOMMIT	0x1	/* Autocommit is set */
#define CONN_FLAG_IN_XCN	0x2 	/* Transaction is in progress */
#define CONN_FLAG_INTERACTIVE	0x4	/* -interactive requested at connect */

#define IncrConnectionRefCount(x) \
    do {			  \
	++((x)->refCount);	  \
    } while(0)
#define DecrConnectionRefCount(x)		\
    do {					\
	ConnectionData* conn = x;		\
	if ((--(conn->refCount)) <= 0) {	\
	    DeleteConnection(conn);		\
	}					\
    } while(0)

/*
 * Structure that carries the data for a MySQL prepared statement.
 *
 *	Just as with connections, statements need to defer taking down
 *	their client data until other objects (i.e., result sets) that
 * 	refer to them have had a chance to clean up. Hence, this
 *	structure is reference counted as well.
 */

typedef struct StatementData {
    int refCount;		/* Reference count */
    ConnectionData* cdata;	/* Data for the connection to which this
				 * statement pertains. */
    Tcl_Obj* subVars;	        /* List of variables to be substituted, in the
				 * order in which they appear in the
				 * statement */
    struct ParamData *params;	/* Data types and attributes of parameters */
    Tcl_Obj* nativeSql;		/* Native SQL statement to pass into
				 * MySQL */
    MYSQL_STMT* stmtPtr;	/* MySQL statement handle */
    MYSQL_RES* metadataPtr;	/* MySQL result set metadata */
    Tcl_Obj* columnNames;	/* Column names in the result set */
    int flags;
} StatementData;
#define IncrStatementRefCount(x)		\
    do {					\
	++((x)->refCount);			\
    } while (0)
#define DecrStatementRefCount(x)		\
    do {					\
	StatementData* stmt = (x);		\
	if (--(stmt->refCount) <= 0) {		\
	    DeleteStatement(stmt);		\
	}					\
    } while(0)

/* Flags in the 'StatementData->flags' word */

#define STMT_FLAG_BUSY		0x1	/* Statement handle is in use */

/*
 * Structure describing the data types of substituted parameters in
 * a SQL statement.
 */

typedef struct ParamData {
    int flags;			/* Flags regarding the parameters - see below */
    int dataType;		/* Data type */
    int precision;		/* Size of the expected data */
    int scale;			/* Digits after decimal point of the
				 * expected data */
} ParamData;

#define PARAM_KNOWN	1<<0	/* Something is known about the parameter */
#define PARAM_IN 	1<<1	/* Parameter is an input parameter */
#define PARAM_OUT 	1<<2	/* Parameter is an output parameter */
				/* (Both bits are set if parameter is
				 * an INOUT parameter) */
#define PARAM_BINARY	1<<3	/* Parameter is binary */

/*
 * Structure describing a MySQL result set.  The object that the Tcl
 * API terms a "result set" actually has to be represented by a MySQL
 * "statement", since a MySQL statement can have only one set of results
 * at any given time.
 */

typedef struct ResultSetData {
    int refCount;		/* Reference count */
    StatementData* sdata;	/* Statement that generated this result set */
    MYSQL_STMT* stmtPtr;	/* Handle to the MySQL statement object */
    Tcl_Obj* paramValues;	/* List of parameter values */
    MYSQL_BIND* paramBindings;	/* Parameter bindings */
    unsigned long* paramLengths;/* Parameter lengths */
    my_ulonglong rowCount;	/* Number of affected rows */
    my_bool* resultErrors;	/* Failure indicators for retrieving columns */
    my_bool* resultNulls;	/* NULL indicators for retrieving columns */
    unsigned long* resultLengths;
				/* Byte lengths of retrieved columns */
    MYSQL_BIND* resultBindings;	/* Bindings controlling column retrieval */
} ResultSetData;
#define IncrResultSetRefCount(x)		\
    do {					\
	++((x)->refCount);			\
    } while (0)
#define DecrResultSetRefCount(x)		\
    do {					\
	ResultSetData* rs = (x);		\
	if (--(rs->refCount) <= 0) {		\
	    DeleteResultSet(rs);		\
	}					\
    } while(0)

/* Table of MySQL type names */

#define IS_BINARY	(1<<16)	/* Flag to OR in if a param is binary */
typedef struct MysqlDataType {
    const char* name;		/* Type name */
    int num;			/* Type number */
} MysqlDataType;
static const MysqlDataType dataTypes[] = {
    { "tinyint",	MYSQL_TYPE_TINY },
    { "smallint",	MYSQL_TYPE_SHORT },
    { "integer",	MYSQL_TYPE_LONG },
    { "float",		MYSQL_TYPE_FLOAT },
    { "real",		MYSQL_TYPE_FLOAT },
    { "double",		MYSQL_TYPE_DOUBLE },
    { "NULL",		MYSQL_TYPE_NULL },
    { "timestamp",	MYSQL_TYPE_TIMESTAMP },
    { "bigint",		MYSQL_TYPE_LONGLONG },
    { "mediumint",	MYSQL_TYPE_INT24 },
    { "date",		MYSQL_TYPE_NEWDATE },
    { "date",		MYSQL_TYPE_DATE },
    { "time",		MYSQL_TYPE_TIME },
    { "datetime",	MYSQL_TYPE_DATETIME },
    { "year",		MYSQL_TYPE_YEAR },
    { "bit",		MYSQL_TYPE_BIT | IS_BINARY },
    { "numeric",	MYSQL_TYPE_NEWDECIMAL },
    { "decimal",	MYSQL_TYPE_NEWDECIMAL },
    { "numeric",	MYSQL_TYPE_DECIMAL },
    { "decimal",	MYSQL_TYPE_DECIMAL },
    { "enum",		MYSQL_TYPE_ENUM },
    { "set",		MYSQL_TYPE_SET },
    { "tinytext",	MYSQL_TYPE_TINY_BLOB },
    { "tinyblob",	MYSQL_TYPE_TINY_BLOB | IS_BINARY },
    { "mediumtext",	MYSQL_TYPE_MEDIUM_BLOB },
    { "mediumblob",	MYSQL_TYPE_MEDIUM_BLOB | IS_BINARY },
    { "longtext",	MYSQL_TYPE_LONG_BLOB },
    { "longblob",	MYSQL_TYPE_LONG_BLOB | IS_BINARY },
    { "text",		MYSQL_TYPE_BLOB },
    { "blob",		MYSQL_TYPE_BLOB | IS_BINARY },
    { "varbinary",	MYSQL_TYPE_VAR_STRING | IS_BINARY },
    { "varchar",	MYSQL_TYPE_VAR_STRING },
    { "varbinary",	MYSQL_TYPE_VARCHAR | IS_BINARY },
    { "varchar",	MYSQL_TYPE_VARCHAR },
    { "binary",		MYSQL_TYPE_STRING | IS_BINARY },
    { "char",		MYSQL_TYPE_STRING },
    { "geometry",	MYSQL_TYPE_GEOMETRY },
    { NULL, 		0 }
};

/* Configuration options for MySQL connections */

/* Data types of configuration options */

enum OptType {
    TYPE_STRING,		/* Arbitrary character string */
    TYPE_FLAG, 			/* Boolean flag */
    TYPE_ENCODING,		/* Encoding name */
    TYPE_ISOLATION,		/* Transaction isolation level */
    TYPE_PORT, 			/* Port number */
    TYPE_READONLY,		/* Read-only indicator */
    TYPE_TIMEOUT		/* Timeout value */
};

/* Locations of the string options in the string array */

enum OptStringIndex {
    INDX_DB, INDX_HOST, INDX_PASSWD, INDX_SOCKET,
    INDX_SSLCA, INDX_SSLCAPATH, INDX_SSLCERT, INDX_SSLCIPHER, INDX_SSLKEY,
    INDX_USER,
    INDX_MAX
};

/* Flags in the configuration table */

#define CONN_OPT_FLAG_MOD 0x1	/* Configuration value changable at runtime */
#define CONN_OPT_FLAG_SSL 0x2	/* Configuration change requires setting
				 * SSL options */
#define CONN_OPT_FLAG_ALIAS 0x4	/* Configuration option is an alias */

 /* Table of configuration options */

static const struct {
    const char * name;	/* Option name */
    enum OptType type;	/* Option data type */
    int info;		/* Option index or flag value */
    int flags;		/* Flags - modifiable; SSL related; is an alias */
    const char* query;	/* How to determine the option value? */
} ConnOptions [] = {
    { "-compress",    TYPE_FLAG,      CLIENT_COMPRESS,	  0,
      "SELECT '', @@SLAVE_COMPRESSED_PROTOCOL" },
    { "-database",    TYPE_STRING,    INDX_DB,		  CONN_OPT_FLAG_MOD,
      "SELECT '', DATABASE();"},
    { "-db",	      TYPE_STRING,    INDX_DB, 		  CONN_OPT_FLAG_MOD
                                                        | CONN_OPT_FLAG_ALIAS,
      "SELECT '', DATABASE()" },
    { "-encoding",    TYPE_ENCODING,  0,		  0,
      "SELECT '', 'utf-8'" },
    { "-host",	      TYPE_STRING,    INDX_HOST,	  0,
      "SHOW SESSION VARIABLES WHERE VARIABLE_NAME = 'hostname'" },
    { "-interactive", TYPE_FLAG,      CLIENT_INTERACTIVE, 0,
      "SELECT '', 0" },
    { "-isolation",   TYPE_ISOLATION, 0,		  CONN_OPT_FLAG_MOD,
      "SELECT '', LCASE(REPLACE(@@TX_ISOLATION, '-', ''))" },
    { "-passwd",      TYPE_STRING,    INDX_PASSWD,	  CONN_OPT_FLAG_MOD
                                                        | CONN_OPT_FLAG_ALIAS,
      "SELECT '', ''" },
    { "-password",    TYPE_STRING,    INDX_PASSWD,	  CONN_OPT_FLAG_MOD,
      "SELECT '', ''" },
    { "-port",	      TYPE_PORT,      0,		  0,
      "SHOW SESSION VARIABLES WHERE VARIABLE_NAME = 'port'" },
    { "-readonly",    TYPE_READONLY,  0,		  0,
      "SELECT '', 0" },
    { "-socket",      TYPE_STRING,    INDX_SOCKET,	  0,
      "SHOW SESSION VARIABLES WHERE VARIABLE_NAME = 'socket'" },
    { "-ssl_ca",      TYPE_STRING,    INDX_SSLCA,	  CONN_OPT_FLAG_SSL,
      "SELECT '', @@SSL_CA"},
    { "-ssl_capath",  TYPE_STRING,    INDX_SSLCAPATH,	  CONN_OPT_FLAG_SSL,
      "SELECT '', @@SSL_CAPATH" },
    { "-ssl_cert",    TYPE_STRING,    INDX_SSLCERT,	  CONN_OPT_FLAG_SSL,
      "SELECT '', @@SSL_CERT" },
    { "-ssl_cipher",  TYPE_STRING,    INDX_SSLCIPHER,	  CONN_OPT_FLAG_SSL,
      "SELECT '', @@SSL_CIPHER" },
    { "-ssl_cypher",  TYPE_STRING,    INDX_SSLCIPHER,	  CONN_OPT_FLAG_SSL
                                                        | CONN_OPT_FLAG_ALIAS,
      "SELECT '', @@SSL_CIPHER" },
    { "-ssl_key",     TYPE_STRING,    INDX_SSLKEY,	  CONN_OPT_FLAG_SSL,
      "SELECT '', @@SSL_KEY" },
    { "-timeout",     TYPE_TIMEOUT,   0,		  CONN_OPT_FLAG_MOD,
      "SELECT '', @@WAIT_TIMEOUT" },
    { "-user",	      TYPE_STRING,    INDX_USER,	  CONN_OPT_FLAG_MOD,
      "SELECT '', USER()" },
    { NULL,	      0,	      0,		  0 }
};

/* Tables of isolation levels: Tcl, SQL, and MySQL 'tx_isolation' */

static const char *const TclIsolationLevels[] = {
    "readuncommitted",
    "readcommitted",
    "repeatableread",
    "serializable",
    NULL
};
static const char *const SqlIsolationLevels[] = {
    "SET TRANSACTION ISOLATION LEVEL READ UNCOMMITTED",
    "SET TRANSACTION ISOLATION LEVEL READ COMMITTED",
    "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ",
    "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE",
    NULL
};
enum IsolationLevel {
    ISOL_READ_UNCOMMITTED,
    ISOL_READ_COMMITTED,
    ISOL_REPEATABLE_READ,
    ISOL_SERIALIZABLE,
    ISOL_NONE = -1
};

/* Declarations of static functions appearing in this file */

static MYSQL_BIND* MysqlBindAlloc(int nBindings);
static MYSQL_BIND* MysqlBindIndex(MYSQL_BIND* b, int i);
static void* MysqlBindAllocBuffer(MYSQL_BIND* b, int i, unsigned long len);
static void MysqlBindFreeBuffer(MYSQL_BIND* b, int i);
static void MysqlBindSetBufferType(MYSQL_BIND* b, int i,
				   enum enum_field_types t);
static void* MysqlBindGetBuffer(MYSQL_BIND* b, int i);
static unsigned long MysqlBindGetBufferLength(MYSQL_BIND* b, int i);
static void MysqlBindSetLength(MYSQL_BIND* b, int i, unsigned long* p);
static void MysqlBindSetIsNull(MYSQL_BIND* b, int i, my_bool* p);
static void MysqlBindSetError(MYSQL_BIND* b, int i, my_bool* p);

static MYSQL_FIELD* MysqlFieldIndex(MYSQL_FIELD* fields, int i);

static void TransferMysqlError(Tcl_Interp* interp, MYSQL* mysqlPtr);
static void TransferMysqlStmtError(Tcl_Interp* interp, MYSQL_STMT* mysqlPtr);

static Tcl_Obj* QueryConnectionOption(ConnectionData* cdata, Tcl_Interp* interp,
				      int optionNum);
static int ConfigureConnection(ConnectionData* cdata, Tcl_Interp* interp,
			       int objc, Tcl_Obj *const objv[], int skip);
static int ConnectionConstructor(ClientData clientData, Tcl_Interp* interp,
				 Tcl_ObjectContext context,
				 int objc, Tcl_Obj *const objv[]);
static int ConnectionBegintransactionMethod(ClientData clientData,
					    Tcl_Interp* interp,
					    Tcl_ObjectContext context,
					    int objc, Tcl_Obj *const objv[]);
static int ConnectionColumnsMethod(ClientData clientData, Tcl_Interp* interp,
				  Tcl_ObjectContext context,
				  int objc, Tcl_Obj *const objv[]);
static int ConnectionCommitMethod(ClientData clientData, Tcl_Interp* interp,
				  Tcl_ObjectContext context,
				  int objc, Tcl_Obj *const objv[]);
static int ConnectionConfigureMethod(ClientData clientData, Tcl_Interp* interp,
				     Tcl_ObjectContext context,
				     int objc, Tcl_Obj *const objv[]);
static int ConnectionEvaldirectMethod(ClientData clientData, Tcl_Interp* interp,
				      Tcl_ObjectContext context,
				      int objc, Tcl_Obj *const objv[]);
static int ConnectionNeedCollationInfoMethod(ClientData clientData,
					     Tcl_Interp* interp,
					     Tcl_ObjectContext context,
					     int objc, Tcl_Obj *const objv[]);
static int ConnectionRollbackMethod(ClientData clientData, Tcl_Interp* interp,
				    Tcl_ObjectContext context,
				    int objc, Tcl_Obj *const objv[]);
static int ConnectionSetCollationInfoMethod(ClientData clientData,
					    Tcl_Interp* interp,
					    Tcl_ObjectContext context,
					    int objc, Tcl_Obj *const objv[]);
static int ConnectionTablesMethod(ClientData clientData, Tcl_Interp* interp,
				  Tcl_ObjectContext context,
				  int objc, Tcl_Obj *const objv[]);

static void DeleteConnectionMetadata(ClientData clientData);
static void DeleteConnection(ConnectionData* cdata);
static int CloneConnection(Tcl_Interp* interp, ClientData oldClientData,
			   ClientData* newClientData);

static StatementData* NewStatement(ConnectionData* cdata);
static MYSQL_STMT* AllocAndPrepareStatement(Tcl_Interp* interp,
					    StatementData* sdata);
static Tcl_Obj* ResultDescToTcl(MYSQL_RES* resultDesc, int flags);

static int StatementConstructor(ClientData clientData, Tcl_Interp* interp,
				Tcl_ObjectContext context,
				int objc, Tcl_Obj *const objv[]);
static int StatementParamtypeMethod(ClientData clientData, Tcl_Interp* interp,
				    Tcl_ObjectContext context,
				    int objc, Tcl_Obj *const objv[]);
static int StatementParamsMethod(ClientData clientData, Tcl_Interp* interp,
				 Tcl_ObjectContext context,
				 int objc, Tcl_Obj *const objv[]);

static void DeleteStatementMetadata(ClientData clientData);
static void DeleteStatement(StatementData* sdata);
static int CloneStatement(Tcl_Interp* interp, ClientData oldClientData,
			  ClientData* newClientData);

static int ResultSetConstructor(ClientData clientData, Tcl_Interp* interp,
				Tcl_ObjectContext context,
				int objc, Tcl_Obj *const objv[]);
static int ResultSetColumnsMethod(ClientData clientData, Tcl_Interp* interp,
				  Tcl_ObjectContext context,
				  int objc, Tcl_Obj *const objv[]);
static int ResultSetNextrowMethod(ClientData clientData, Tcl_Interp* interp,
				  Tcl_ObjectContext context,
				  int objc, Tcl_Obj *const objv[]);
static int ResultSetRowcountMethod(ClientData clientData, Tcl_Interp* interp,
				   Tcl_ObjectContext context,
				   int objc, Tcl_Obj *const objv[]);

static void DeleteResultSetMetadata(ClientData clientData);
static void DeleteResultSet(ResultSetData* rdata);
static int CloneResultSet(Tcl_Interp* interp, ClientData oldClientData,
			  ClientData* newClientData);


static void DeleteCmd(ClientData clientData);
static int CloneCmd(Tcl_Interp* interp,
		    ClientData oldMetadata, ClientData* newMetadata);

static void DeletePerInterpData(PerInterpData* pidata);

/* Metadata type that holds connection data */

const static Tcl_ObjectMetadataType connectionDataType = {
    TCL_OO_METADATA_VERSION_CURRENT,
				/* version */
    "ConnectionData",		/* name */
    DeleteConnectionMetadata,	/* deleteProc */
    CloneConnection		/* cloneProc - should cause an error
				 * 'cuz connections aren't clonable */
};

/* Metadata type that holds statement data */

const static Tcl_ObjectMetadataType statementDataType = {
    TCL_OO_METADATA_VERSION_CURRENT,
				/* version */
    "StatementData",		/* name */
    DeleteStatementMetadata,	/* deleteProc */
    CloneStatement		/* cloneProc - should cause an error
				 * 'cuz statements aren't clonable */
};

/* Metadata type for result set data */

const static Tcl_ObjectMetadataType resultSetDataType = {
    TCL_OO_METADATA_VERSION_CURRENT,
				/* version */
    "ResultSetData",		/* name */
    DeleteResultSetMetadata,	/* deleteProc */
    CloneResultSet		/* cloneProc - should cause an error
				 * 'cuz result sets aren't clonable */
};

/* Method types of the connection methods that are implemented in C */

const static Tcl_MethodType ConnectionConstructorType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "CONSTRUCTOR",		/* name */
    ConnectionConstructor,	/* callProc */
    DeleteCmd,			/* deleteProc */
    CloneCmd			/* cloneProc */
};

const static Tcl_MethodType ConnectionBegintransactionMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "begintransaction",		/* name */
    ConnectionBegintransactionMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ConnectionColumnsMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "Columns",			/* name */
    ConnectionColumnsMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ConnectionCommitMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "commit",			/* name */
    ConnectionCommitMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ConnectionConfigureMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "configure",		/* name */
    ConnectionConfigureMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ConnectionEvaldirectMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "evaldirect",		/* name */
    ConnectionEvaldirectMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ConnectionNeedCollationInfoMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "NeedCollationInfo",	/* name */
    ConnectionNeedCollationInfoMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ConnectionRollbackMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "rollback",			/* name */
    ConnectionRollbackMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ConnectionSetCollationInfoMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "SetCollationInfo",		/* name */
    ConnectionSetCollationInfoMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ConnectionTablesMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "tables",			/* name */
    ConnectionTablesMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};

const static Tcl_MethodType* ConnectionMethods[] = {
    &ConnectionBegintransactionMethodType,
    &ConnectionColumnsMethodType,
    &ConnectionCommitMethodType,
    &ConnectionConfigureMethodType,
    &ConnectionEvaldirectMethodType,
    &ConnectionNeedCollationInfoMethodType,
    &ConnectionRollbackMethodType,
    &ConnectionSetCollationInfoMethodType,
    &ConnectionTablesMethodType,
    NULL
};

/* Method types of the statement methods that are implemented in C */

const static Tcl_MethodType StatementConstructorType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "CONSTRUCTOR",		/* name */
    StatementConstructor,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType StatementParamsMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "params",			/* name */
    StatementParamsMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType StatementParamtypeMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "paramtype",		/* name */
    StatementParamtypeMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};

/*
 * Methods to create on the statement class.
 */

const static Tcl_MethodType* StatementMethods[] = {
    &StatementParamsMethodType,
    &StatementParamtypeMethodType,
    NULL
};

/* Method types of the result set methods that are implemented in C */

const static Tcl_MethodType ResultSetConstructorType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "CONSTRUCTOR",		/* name */
    ResultSetConstructor,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ResultSetColumnsMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */    "columns",			/* name */
    ResultSetColumnsMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ResultSetNextrowMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "nextrow",			/* name */
    ResultSetNextrowMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};
const static Tcl_MethodType ResultSetRowcountMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "rowcount",			/* name */
    ResultSetRowcountMethod,	/* callProc */
    NULL,			/* deleteProc */
    NULL			/* cloneProc */
};


/* Methods to create on the result set class */

const static Tcl_MethodType* ResultSetMethods[] = {
    &ResultSetColumnsMethodType,
    &ResultSetRowcountMethodType,
    NULL
};

/*
 *-----------------------------------------------------------------------------
 *
 * MysqlBindAlloc --
 *
 *	Allocate a number of MYSQL_BIND structures.
 *
 * Results:
 *	Returns a pointer to the array of structures, which will be zeroed out.
 *
 *-----------------------------------------------------------------------------
 */

static MYSQL_BIND*
MysqlBindAlloc(int nBindings)
{
    int size;
    void* retval = NULL;
    if (mysqlClientVersion >= 50100) {
	size = sizeof(struct st_mysql_bind_51);
    } else {
	size = sizeof(struct st_mysql_bind_50);
    }
    size *= nBindings;
    if (size != 0) {
	retval = ckalloc(size);
	memset(retval, 0, size);
    }
    return (MYSQL_BIND*) retval;
}

/*
 *-----------------------------------------------------------------------------
 *
 * MysqlBindIndex --
 *
 *	Returns a pointer to one of an array of MYSQL_BIND objects
 *
 *-----------------------------------------------------------------------------
 */

static MYSQL_BIND*
MysqlBindIndex(
    MYSQL_BIND* b, 		/* Binding array to alter */
    int i			/* Index in the binding array */
) {
    if (mysqlClientVersion >= 50100) {
	return (MYSQL_BIND*)(((struct st_mysql_bind_51*) b) + i);
    } else {
	return (MYSQL_BIND*)(((struct st_mysql_bind_50*) b) + i);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * MysqlBindAllocBuffer --
 *
 *	Allocates the buffer in a MYSQL_BIND object
 *
 * Results:
 *	Returns a pointer to the allocated buffer
 *
 *-----------------------------------------------------------------------------
 */

static void*
MysqlBindAllocBuffer(
    MYSQL_BIND* b,		/* Pointer to a binding array */
    int i,			/* Index into the array */
    unsigned long len		/* Length of the buffer to allocate or 0 */
) {
    void* block = NULL;
    if (len != 0) {
	block = ckalloc(len);
    }
    if (mysqlClientVersion >= 50100) {
	((struct st_mysql_bind_51*) b)[i].buffer = block;
	((struct st_mysql_bind_51*) b)[i].buffer_length = len;
    } else {
	((struct st_mysql_bind_50*) b)[i].buffer = block;
	((struct st_mysql_bind_50*) b)[i].buffer_length = len;
    }
    return block;
}

/*
 *-----------------------------------------------------------------------------
 *
 * MysqlBindFreeBuffer --
 *
 *	Frees trhe buffer in a MYSQL_BIND object
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Buffer is returned to the system.
 *
 *-----------------------------------------------------------------------------
 */
static void
MysqlBindFreeBuffer(
    MYSQL_BIND* b,		/* Pointer to a binding array */
    int i			/* Index into the array */
) {
    if (mysqlClientVersion >= 50100) {
	struct st_mysql_bind_51* bindings = (struct st_mysql_bind_51*) b;
	if (bindings[i].buffer) {
	    ckfree(bindings[i].buffer);
	    bindings[i].buffer = NULL;
	}
	bindings[i].buffer_length = 0;
    } else {
	struct st_mysql_bind_50* bindings = (struct st_mysql_bind_50*) b;
	if (bindings[i].buffer) {
	    ckfree(bindings[i].buffer);
	    bindings[i].buffer = NULL;
	}
	bindings[i].buffer_length = 0;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * MysqlBindGetBufferLength, MysqlBindSetBufferType, MysqlBindGetBufferType,
 * MysqlBindSetLength, MysqlBindSetIsNull,
 * MysqlBindSetError --
 *
 *	Access the fields of a MYSQL_BIND object
 *
 *-----------------------------------------------------------------------------
 */

static void*
MysqlBindGetBuffer(
    MYSQL_BIND* b, 		/* Binding array to alter */
    int i			/* Index in the binding array */
) {
    if (mysqlClientVersion >= 50100) {
	return ((struct st_mysql_bind_51*) b)[i].buffer;
    } else {
	return ((struct st_mysql_bind_50*) b)[i].buffer;
    }
}

static unsigned long
MysqlBindGetBufferLength(
    MYSQL_BIND* b, 		/* Binding array to alter */
    int i			/* Index in the binding array */
) {
    if (mysqlClientVersion >= 50100) {
	return ((struct st_mysql_bind_51*) b)[i].buffer_length;
    } else {
	return ((struct st_mysql_bind_50*) b)[i].buffer_length;
    }

}

static enum enum_field_types
MysqlBindGetBufferType(
    MYSQL_BIND* b, 		/* Binding array to alter */
    int i			/* Index in the binding array */
) {
    if (mysqlClientVersion >= 50100) {
	return ((struct st_mysql_bind_51*) b)[i].buffer_type;
    } else {
	return ((struct st_mysql_bind_50*) b)[i].buffer_type;
    }
}

static void
MysqlBindSetBufferType(
    MYSQL_BIND* b, 		/* Binding array to alter */
    int i,			/* Index in the binding array */
    enum enum_field_types t	/* Buffer type to assign */
) {
    if (mysqlClientVersion >= 50100) {
	((struct st_mysql_bind_51*) b)[i].buffer_type = t;
    } else {
	((struct st_mysql_bind_50*) b)[i].buffer_type = t;
    }
}

static void
MysqlBindSetLength(
    MYSQL_BIND* b, 		/* Binding array to alter */
    int i,			/* Index in the binding array */
    unsigned long* p		/* Length pointer to assign */
) {
    if (mysqlClientVersion >= 50100) {
	((struct st_mysql_bind_51*) b)[i].length = p;
    } else {
	((struct st_mysql_bind_50*) b)[i].length = p;
    }
}

static void
MysqlBindSetIsNull(
    MYSQL_BIND* b, 		/* Binding array to alter */
    int i,			/* Index in the binding array */
    my_bool* p			/* "Is null" indicator pointer to assign */
) {
    if (mysqlClientVersion >= 50100) {
	((struct st_mysql_bind_51*) b)[i].is_null = p;
    } else {
	((struct st_mysql_bind_50*) b)[i].is_null = p;
    }
}

static void
MysqlBindSetError(
    MYSQL_BIND* b, 		/* Binding array to alter */
    int i,			/* Index in the binding array */
    my_bool* p			/* Error indicator pointer to assign */
) {
    if (mysqlClientVersion >= 50100) {
	((struct st_mysql_bind_51*) b)[i].error = p;
    } else {
	((struct st_mysql_bind_50*) b)[i].error = p;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * MysqlFieldIndex --
 *
 *	Return a pointer to a given MYSQL_FIELD structure in an array
 *
 * The MYSQL_FIELD structure grows by one pointer between 5.0 and 5.1.
 * Our code never creates a MYSQL_FIELD, nor does it try to access that
 * pointer, so we handle things simply by casting the types.
 *
 *-----------------------------------------------------------------------------
 */

static MYSQL_FIELD*
MysqlFieldIndex(MYSQL_FIELD* fields,
				/*  Pointer to the array*/
		int i)		/* Index in the array */
{
    MYSQL_FIELD* retval;
    if (mysqlClientVersion >= 50100) {
	retval = (MYSQL_FIELD*)(((struct st_mysql_field_51*) fields)+i);
    } else {
	retval = (MYSQL_FIELD*)(((struct st_mysql_field_50*) fields)+i);
    }
    return retval;
}

/*
 *-----------------------------------------------------------------------------
 *
 * TransferMysqlError --
 *
 *	Obtains the error message, SQL state, and error number from the
 *	MySQL client library and transfers them into the Tcl interpreter
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the interpreter result and error code to describe the SQL error
 *
 *-----------------------------------------------------------------------------
 */

static void
TransferMysqlError(
    Tcl_Interp* interp,		/* Tcl interpreter */
    MYSQL* mysqlPtr		/* MySQL connection handle */
) {
    const char* sqlstate = mysql_sqlstate(mysqlPtr);
    Tcl_Obj* errorCode = Tcl_NewObj();
    Tcl_ListObjAppendElement(NULL, errorCode, Tcl_NewStringObj("TDBC", -1));
    Tcl_ListObjAppendElement(NULL, errorCode,
			     Tcl_NewStringObj(Tdbc_MapSqlState(sqlstate), -1));
    Tcl_ListObjAppendElement(NULL, errorCode,
			     Tcl_NewStringObj(sqlstate, -1));
    Tcl_ListObjAppendElement(NULL, errorCode, Tcl_NewStringObj("MYSQL", -1));
    Tcl_ListObjAppendElement(NULL, errorCode,
			     Tcl_NewIntObj(mysql_errno(mysqlPtr)));
    Tcl_SetObjErrorCode(interp, errorCode);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(mysql_error(mysqlPtr), -1));
}

/*
 *-----------------------------------------------------------------------------
 *
 * TransferMysqlStmtError --
 *
 *	Obtains the error message, SQL state, and error number from the
 *	MySQL client library and transfers them into the Tcl interpreter
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the interpreter result and error code to describe the SQL error
 *
 *-----------------------------------------------------------------------------
 */

static void
TransferMysqlStmtError(
    Tcl_Interp* interp,		/* Tcl interpreter */
    MYSQL_STMT* stmtPtr		/* MySQL statment handle */
) {
    const char* sqlstate = mysql_stmt_sqlstate(stmtPtr);
    Tcl_Obj* errorCode = Tcl_NewObj();
    Tcl_ListObjAppendElement(NULL, errorCode, Tcl_NewStringObj("TDBC", -1));
    Tcl_ListObjAppendElement(NULL, errorCode,
			     Tcl_NewStringObj(Tdbc_MapSqlState(sqlstate), -1));
    Tcl_ListObjAppendElement(NULL, errorCode,
			     Tcl_NewStringObj(sqlstate, -1));
    Tcl_ListObjAppendElement(NULL, errorCode, Tcl_NewStringObj("MYSQL", -1));
    Tcl_ListObjAppendElement(NULL, errorCode,
			     Tcl_NewIntObj(mysql_stmt_errno(stmtPtr)));
    Tcl_SetObjErrorCode(interp, errorCode);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(mysql_stmt_error(stmtPtr), -1));
}

/*
 *-----------------------------------------------------------------------------
 *
 * QueryConnectionOption --
 *
 *	Determine the current value of a connection option.
 *
 * Results:
 *	Returns a Tcl object containing the value if successful, or NULL
 *	if unsuccessful. If unsuccessful, stores error information in the
 *	Tcl interpreter.
 *
 *-----------------------------------------------------------------------------
 */

static Tcl_Obj*
QueryConnectionOption (
    ConnectionData* cdata,	/* Connection data */
    Tcl_Interp* interp,		/* Tcl interpreter */
    int optionNum		/* Position of the option in the table */
) {
    MYSQL_RES* result;		/* Result of the MySQL query for the option */
    MYSQL_ROW row;		/* Row of the result set */
    int fieldCount;		/* Number of fields in a row */
    unsigned long* lengths;	/* Character lengths of the fields */
    Tcl_Obj* retval;		/* Return value */

    if (mysql_query(cdata->mysqlPtr, ConnOptions[optionNum].query)) {
	TransferMysqlError(interp, cdata->mysqlPtr);
	return NULL;
    }
    result = mysql_store_result(cdata->mysqlPtr);
    if (result == NULL) {
	TransferMysqlError(interp, cdata->mysqlPtr);
	return NULL;
    }
    fieldCount = mysql_num_fields(result);
    if (fieldCount < 2) {
	retval = cdata->pidata->literals[LIT_EMPTY];
    } else {
	if ((row = mysql_fetch_row(result)) == NULL) {
	    if (mysql_errno(cdata->mysqlPtr)) {
		TransferMysqlError(interp, cdata->mysqlPtr);
		mysql_free_result(result);
		return NULL;
	    } else {
		retval = cdata->pidata->literals[LIT_EMPTY];
	    }
	} else {
	    lengths = mysql_fetch_lengths(result);
	    retval = Tcl_NewStringObj(row[1], lengths[1]);
	}
    }
    mysql_free_result(result);
    return retval;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConfigureConnection --
 *
 *	Applies configuration settings to a MySQL connection.
 *
 * Results:
 *	Returns a Tcl result. If the result is TCL_ERROR, error information
 *	is stored in the interpreter.
 *
 * Side effects:
 *	Updates configuration in the connection data. Opens a connection
 *	if none is yet open.
 *
 *-----------------------------------------------------------------------------
 */

static int
ConfigureConnection(
    ConnectionData* cdata,	/* Connection data */
    Tcl_Interp* interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj* const objv[],	/* Parameter data */
    int skip			/* Number of parameters to skip */
) {

    const char* stringOpts[INDX_MAX];
				/* String-valued options */
    unsigned long mysqlFlags=0;	/* Connection flags */
    int sslFlag = 0;		/* Flag==1 if SSL configuration is needed */
    int optionIndex;		/* Index of the current option in ConnOptions */
    int optionValue;		/* Integer value of the current option */
    unsigned short port = 0;	/* Server port number */
    int isolation = ISOL_NONE;	/* Isolation level */
    int timeout = 0;		/* Timeout value */
    int i;
    Tcl_Obj* retval;
    Tcl_Obj* optval;

    if (cdata->mysqlPtr != NULL) {

	/* Query configuration options on an existing connection */

	if (objc == skip) {
	    retval = Tcl_NewObj();
	    for (i = 0; ConnOptions[i].name != NULL; ++i) {
		if (ConnOptions[i].flags & CONN_OPT_FLAG_ALIAS) continue;
		optval = QueryConnectionOption(cdata, interp, i);
		if (optval == NULL) {
		    return TCL_ERROR;
		}
		Tcl_DictObjPut(NULL, retval,
			       Tcl_NewStringObj(ConnOptions[i].name, -1),
			       optval);
	    }
	    Tcl_SetObjResult(interp, retval);
	    return TCL_OK;
	} else if (objc == skip+1) {

	    if (Tcl_GetIndexFromObjStruct(interp, objv[skip],
					  (void*) ConnOptions,
					  sizeof(ConnOptions[0]), "option",
					  0, &optionIndex) != TCL_OK) {
		return TCL_ERROR;
	    }
	    retval = QueryConnectionOption(cdata, interp, optionIndex);
	    if (retval == NULL) {
		return TCL_ERROR;
	    } else {
		Tcl_SetObjResult(interp, retval);
		return TCL_OK;
	    }
	}
    }

    if ((objc-skip) % 2 != 0) {
	Tcl_WrongNumArgs(interp, skip, objv, "?-option value?...");
	return TCL_ERROR;
    }

    /* Extract options from the command line */

    for (i = 0; i < INDX_MAX; ++i) {
	stringOpts[i] = NULL;
    }
    for (i = skip; i < objc; i += 2) {

	/* Unknown option */

	if (Tcl_GetIndexFromObjStruct(interp, objv[i], (void*) ConnOptions,
				      sizeof(ConnOptions[0]), "option",
				      0, &optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}

	/* Unmodifiable option */

	if (cdata->mysqlPtr != NULL && !(ConnOptions[optionIndex].flags
					 & CONN_OPT_FLAG_MOD)) {
	    Tcl_Obj* msg = Tcl_NewStringObj("\"", -1);
	    Tcl_AppendObjToObj(msg, objv[i]);
	    Tcl_AppendToObj(msg, "\" option cannot be changed dynamically", -1);
	    Tcl_SetObjResult(interp, msg);
	    Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY000",
			     "MYSQL", "-1", NULL);
	    return TCL_ERROR;
	}

	/* Record option value */

	switch (ConnOptions[optionIndex].type) {
	case TYPE_STRING:
	    stringOpts[ConnOptions[optionIndex].info] =
		Tcl_GetString(objv[i+1]);
	    break;
	case TYPE_FLAG:
	    if (Tcl_GetBooleanFromObj(interp, objv[i+1], &optionValue)
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    if (optionValue) {
		mysqlFlags |= ConnOptions[optionIndex].info;
	    }
	    break;
	case TYPE_ENCODING:
	    if (strcmp(Tcl_GetString(objv[i+1]), "utf-8")) {
		Tcl_SetObjResult(interp,
				 Tcl_NewStringObj("Only UTF-8 transfer "
						  "encoding is supported.\n",
						  -1));
		Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY000",
				 "MYSQL", "-1", NULL);
		return TCL_ERROR;
	    }
	    break;
	case TYPE_ISOLATION:
	    if (Tcl_GetIndexFromObj(interp, objv[i+1], TclIsolationLevels,
				    "isolation level", TCL_EXACT, &isolation)
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TYPE_PORT:
	    if (Tcl_GetIntFromObj(interp, objv[i+1], &optionValue) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (optionValue < 0 || optionValue > 0xffff) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("port number must "
							  "be in range "
							  "[0..65535]", -1));
		Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY000",
				 "MYSQL", "-1", NULL);
		return TCL_ERROR;
	    }
	    port = optionValue;
	    break;
	case TYPE_READONLY:
	    if (Tcl_GetBooleanFromObj(interp, objv[i+1], &optionValue)
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    if (optionValue != 0) {
		Tcl_SetObjResult(interp,
				 Tcl_NewStringObj("MySQL does not support "
						  "readonly connections", -1));
		Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY000",
				 "MYSQL", "-1", NULL);
		return TCL_ERROR;
	    }
	    break;
	case TYPE_TIMEOUT:
	    if (Tcl_GetIntFromObj(interp, objv[i+1], &timeout) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	}
	if (ConnOptions[optionIndex].flags & CONN_OPT_FLAG_SSL) {
	    sslFlag = 1;
	}
    }

    if (cdata->mysqlPtr == NULL) {

	/* Configuring a new connection. Open the database */

	cdata->mysqlPtr = mysql_init(NULL);
	if (cdata->mysqlPtr == NULL) {
	    Tcl_SetObjResult(interp,
			     Tcl_NewStringObj("mysql_init() failed.", -1));
	    Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY001",
			     "MYSQL", "NULL", NULL);
	    return TCL_ERROR;
	}

	/* Set character set for the connection */

	mysql_options(cdata->mysqlPtr, MYSQL_SET_CHARSET_NAME, "utf8");

	    /* Set SSL options if needed */

	if (sslFlag) {
	    mysql_ssl_set(cdata->mysqlPtr, stringOpts[INDX_SSLKEY],
			  stringOpts[INDX_SSLCERT], stringOpts[INDX_SSLCA],
			  stringOpts[INDX_SSLCAPATH],
			  stringOpts[INDX_SSLCIPHER]);
	}

	/* Establish the connection */

	/*
	 * TODO - mutex around this unless linked to libmysqlclient_r ?
	 */

	if (mysql_real_connect(cdata->mysqlPtr, stringOpts[INDX_HOST],
			       stringOpts[INDX_USER], stringOpts[INDX_PASSWD],
			       stringOpts[INDX_DB], port,
			       stringOpts[INDX_SOCKET], mysqlFlags) == NULL) {
	    TransferMysqlError(interp, cdata->mysqlPtr);
	    return TCL_ERROR;
	}

	cdata->flags |= CONN_FLAG_AUTOCOMMIT;

    } else {

	/* Already open connection */

	if (stringOpts[INDX_USER] != NULL) {

	    /* User name changed - log in again */

	    if (mysql_change_user(cdata->mysqlPtr,
				  stringOpts[INDX_USER],
				  stringOpts[INDX_PASSWD],
				  stringOpts[INDX_DB])) {
		TransferMysqlError(interp, cdata->mysqlPtr);
		return TCL_ERROR;
	    }
	} else if (stringOpts[INDX_DB] != NULL) {

	    /* Database name changed - use the new database */

	    if (mysql_select_db(cdata->mysqlPtr, stringOpts[INDX_DB])) {
		TransferMysqlError(interp, cdata->mysqlPtr);
		return TCL_ERROR;
	    }
	}
    }

    /* Transaction isolation level */

    if (isolation != ISOL_NONE) {
	if (mysql_query(cdata->mysqlPtr, SqlIsolationLevels[isolation])) {
	    TransferMysqlError(interp, cdata->mysqlPtr);
	    return TCL_ERROR;
	}
    }

    /* Timeout */

    if (timeout != 0) {
        int result;
	Tcl_Obj* query = Tcl_ObjPrintf("SET SESSION WAIT_TIMEOUT = %d\n",
				       timeout);
	Tcl_IncrRefCount(query);
	result = mysql_query(cdata->mysqlPtr, Tcl_GetString(query));
	Tcl_DecrRefCount(query);
	if (result) {
	    TransferMysqlError(interp, cdata->mysqlPtr);
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionConstructor --
 *
 *	Constructor for ::tdbc::mysql::connection, which represents a
 *	database connection.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * The ConnectionInitMethod takes alternating keywords and values giving
 * the configuration parameters of the connection, and attempts to connect
 * to the database.
 *
 *-----------------------------------------------------------------------------
 */

static int
ConnectionConstructor(
    ClientData clientData,	/* Environment handle */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext context, /* Object context */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    PerInterpData* pidata = (PerInterpData*) clientData;
				/* Per-interp data for the MYSQL package */
    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current object */
    int skip = Tcl_ObjectContextSkippedArgs(context);
				/* The number of leading arguments to skip */
    ConnectionData* cdata;	/* Per-connection data */

    /* Hang client data on this connection */

    cdata = (ConnectionData*) ckalloc(sizeof(ConnectionData));
    cdata->refCount = 1;
    cdata->pidata = pidata;
    cdata->mysqlPtr = NULL;
    cdata->nCollations = 0;
    cdata->collationSizes = NULL;
    cdata->flags = 0;
    IncrPerInterpRefCount(pidata);
    Tcl_ObjectSetMetadata(thisObject, &connectionDataType, (ClientData) cdata);

    /* Configure the connection */

    if (ConfigureConnection(cdata, interp, objc, objv, skip) != TCL_OK) {
	return TCL_ERROR;
    }

    return TCL_OK;

}

/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionBegintransactionMethod --
 *
 *	Method that requests that following operations on an OBBC connection
 *	be executed as an atomic transaction.
 *
 * Usage:
 *	$connection begintransaction
 *
 * Parameters:
 *	None.
 *
 * Results:
 *	Returns an empty result if successful, and throws an error otherwise.
 *
 *-----------------------------------------------------------------------------
*/

static int
ConnectionBegintransactionMethod(
    ClientData clientData,	/* Unused */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext objectContext, /* Object context */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(objectContext);
				/* The current connection object */
    ConnectionData* cdata = (ConnectionData*)
	Tcl_ObjectGetMetadata(thisObject, &connectionDataType);

    /* Check parameters */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    /* Reject attempts at nested transactions */

    if (cdata->flags & CONN_FLAG_IN_XCN) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("MySQL does not support "
						  "nested transactions", -1));
	Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HYC00",
			 "MYSQL", "-1", NULL);
	return TCL_ERROR;
    }
    cdata->flags |= CONN_FLAG_IN_XCN;

    /* Turn off autocommit for the duration of the transaction */

    if (cdata->flags & CONN_FLAG_AUTOCOMMIT) {
	if (mysql_autocommit(cdata->mysqlPtr, 0)) {
	    TransferMysqlError(interp, cdata->mysqlPtr);
	    return TCL_ERROR;
	}
	cdata->flags &= ~CONN_FLAG_AUTOCOMMIT;
    }

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionColumnsMethod --
 *
 *	Method that asks for the names of columns in a table
 *	in the database (optionally matching a given pattern)
 *
 * Usage:
 * 	$connection columns table ?pattern?
 *
 * Parameters:
 *	None.
 *
 * Results:
 *	Returns the list of tables
 *
 *-----------------------------------------------------------------------------
 */

static int
ConnectionColumnsMethod(
    ClientData clientData,	/* Completion type */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext objectContext, /* Object context */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(objectContext);
				/* The current connection object */
    ConnectionData* cdata = (ConnectionData*)
	Tcl_ObjectGetMetadata(thisObject, &connectionDataType);
				/* Instance data */
    PerInterpData* pidata = cdata->pidata;
				/* Per-interpreter data */
    Tcl_Obj** literals = pidata->literals;
				/* Literal pool */
    const char* patternStr;	/* Pattern to match table names */
    MYSQL_RES* results;		/* Result set */
    Tcl_Obj* retval;		/* List of table names */
    Tcl_Obj* name;		/* Name of a column */
    Tcl_Obj* attrs;		/* Attributes of the column */
    Tcl_HashEntry* entry;	/* Hash entry for data type */

    /* Check parameters */

    if (objc == 3) {
	patternStr = NULL;
    } else if (objc == 4) {
	patternStr = Tcl_GetString(objv[3]);
    } else {
	Tcl_WrongNumArgs(interp, 2, objv, "table ?pattern?");
	return TCL_ERROR;
    }

    results = mysql_list_fields(cdata->mysqlPtr, Tcl_GetString(objv[2]),
				patternStr);
    if (results == NULL) {
	TransferMysqlError(interp, cdata->mysqlPtr);
	return TCL_ERROR;
    } else {
	unsigned int fieldCount = mysql_num_fields(results);
	MYSQL_FIELD* fields = mysql_fetch_fields(results);
	unsigned int i;
	retval = Tcl_NewObj();
	Tcl_IncrRefCount(retval);
	for (i = 0; i < fieldCount; ++i) {
	    MYSQL_FIELD* field = MysqlFieldIndex(fields, i);
	    attrs = Tcl_NewObj();
	    name = Tcl_NewStringObj(field->name, field->name_length);

	    Tcl_DictObjPut(NULL, attrs, literals[LIT_NAME], name);
	    /* TODO - Distinguish CHAR and BINARY */
	    entry = Tcl_FindHashEntry(&(pidata->typeNumHash),
				      (char*) field->type);
	    if (entry != NULL) {
		Tcl_DictObjPut(NULL, attrs, literals[LIT_TYPE],
			       (Tcl_Obj*) Tcl_GetHashValue(entry));
	    }
	    if (IS_NUM(field->type)) {
		Tcl_DictObjPut(NULL, attrs, literals[LIT_PRECISION],
			       Tcl_NewIntObj(field->length));
	    } else if (field->charsetnr < cdata->nCollations) {
		Tcl_DictObjPut(NULL, attrs, literals[LIT_PRECISION],
		    Tcl_NewIntObj(field->length
			/ cdata->collationSizes[field->charsetnr]));
	    }
	    Tcl_DictObjPut(NULL, attrs, literals[LIT_SCALE],
			   Tcl_NewIntObj(field->decimals));
	    Tcl_DictObjPut(NULL, attrs, literals[LIT_NULLABLE],
			   Tcl_NewIntObj(!(field->flags
					   & (NOT_NULL_FLAG))));
	    Tcl_DictObjPut(NULL, retval, name, attrs);
	}
	mysql_free_result(results);
	Tcl_SetObjResult(interp, retval);
	Tcl_DecrRefCount(retval);
	return TCL_OK;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionCommitMethod --
 *
 *	Method that requests that a pending transaction against a database
 * 	be committed.
 *
 * Usage:
 *	$connection commit
 *
 * Parameters:
 *	None.
 *
 * Results:
 *	Returns an empty Tcl result if successful, and throws an error
 *	otherwise.
 *
 *-----------------------------------------------------------------------------
 */

static int
ConnectionCommitMethod(
    ClientData clientData,	/* Completion type */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext objectContext, /* Object context */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(objectContext);
				/* The current connection object */
    ConnectionData* cdata = (ConnectionData*)
	Tcl_ObjectGetMetadata(thisObject, &connectionDataType);
				/* Instance data */
    my_bool rc;			/* MySQL status return */

    /* Check parameters */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    /* Reject the request if no transaction is in progress */

    if (!(cdata->flags & CONN_FLAG_IN_XCN)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("no transaction is in "
						  "progress", -1));
	Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY010",
			 "MYSQL", "-1", NULL);
	return TCL_ERROR;
    }

    /* End transaction, turn off "transaction in progress", and report status */

    rc = mysql_commit(cdata->mysqlPtr);
    cdata->flags &= ~ CONN_FLAG_IN_XCN;
    if (rc) {
	TransferMysqlError(interp, cdata->mysqlPtr);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionConfigureMethod --
 *
 *	Change configuration parameters on an open connection.
 *
 * Usage:
 *	$connection configure ?-keyword? ?value? ?-keyword value ...?
 *
 * Parameters:
 *	Keyword-value pairs (or a single keyword, or an empty set)
 *	of configuration options.
 *
 * Options:
 *	The following options are supported;
 *	    -database
 *		Name of the database to use by default in queries
 *	    -encoding
 *		Character encoding to use with the server. (Must be utf-8)
 *	    -isolation
 *		Transaction isolation level.
 *	    -readonly
 *		Read-only flag (must be a false Boolean value)
 *	    -timeout
 *		Timeout value (both wait_timeout and interactive_timeout)
 *
 *	Other options supported by the constructor are here in read-only
 *	mode; any attempt to change them will result in an error.
 *
 *-----------------------------------------------------------------------------
 */

static int ConnectionConfigureMethod(
     ClientData clientData,
     Tcl_Interp* interp,
     Tcl_ObjectContext objectContext,
     int objc,
     Tcl_Obj *const objv[]
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(objectContext);
				/* The current connection object */
    int skip = Tcl_ObjectContextSkippedArgs(objectContext);
				/* Number of arguments to skip */
    ConnectionData* cdata = (ConnectionData*)
	Tcl_ObjectGetMetadata(thisObject, &connectionDataType);
				/* Instance data */
    return ConfigureConnection(cdata, interp, objc, objv, skip);
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionEvaldirectMethod --
 *
 *	Evaluates a MySQL statement that is not supported by the prepared
 *	statement API.
 *
 * Usage:
 *	$connection evaldirect sql-statement
 *
 * Parameters:
 *	sql-statement -
 *		SQL statement to evaluate. The statement may not contain
 *		substitutions.
 *
 * Results:
 *	Returns a standard Tcl result. If the operation is successful,
 *	the result consists of a list of rows (in the same form as
 *	[$connection allrows -as dicts]). If the operation fails, the
 *	result is an error message.
 *
 * Side effects:
 *	Whatever the SQL statement does.
 *
 *-----------------------------------------------------------------------------
 */

static int
ConnectionEvaldirectMethod(
    ClientData clientData,	     /* Unused */
    Tcl_Interp* interp,		     /* Tcl interpreter */
    Tcl_ObjectContext objectContext, /* Object context */
    int objc,			     /* Parameter count */
    Tcl_Obj *const objv[])           /* Parameter vector */
{
    Tcl_Object thisObject = Tcl_ObjectContextObject(objectContext);
				/* Current connection object */
    ConnectionData* cdata = (ConnectionData*)
	Tcl_ObjectGetMetadata(thisObject, &connectionDataType);
				/* Instance data */
    int nColumns;		/* Number of columns in the result set */
    MYSQL_RES* resultPtr;	/* MySQL result set */
    MYSQL_ROW rowPtr;		/* One row of the result set */
    unsigned long* lengths;	/* Lengths of the fields in a row */
    Tcl_Obj* retObj;		/* Result set as a Tcl list */
    Tcl_Obj* rowObj;		/* One row of the result set as a Tcl list */
    Tcl_Obj* fieldObj;		/* One field of the row */
    int i;

    /* Check parameters */

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    /* Execute the given statement */

    if (mysql_query(cdata->mysqlPtr, Tcl_GetString(objv[2]))) {
	TransferMysqlError(interp, cdata->mysqlPtr);
	return TCL_ERROR;
    }

    /* Retrieve the result set */

    resultPtr = mysql_store_result(cdata->mysqlPtr);
    nColumns = mysql_field_count(cdata->mysqlPtr);
    if (resultPtr == NULL) {
	/*
	 * Can't retrieve result set. Distinguish result-less statements
	 * from MySQL errors.
	 */
	if (nColumns == 0) {
	    Tcl_SetObjResult
		(interp,
		 Tcl_NewWideIntObj(mysql_affected_rows(cdata->mysqlPtr)));
	    return TCL_OK;
	} else {
	    TransferMysqlError(interp, cdata->mysqlPtr);
	    return TCL_ERROR;
	}
    }

    /* Make a list-of-lists of the result */

    retObj = Tcl_NewObj();
    while ((rowPtr = mysql_fetch_row(resultPtr)) != NULL) {
	rowObj = Tcl_NewObj();
	lengths = mysql_fetch_lengths(resultPtr);
	for (i = 0; i < nColumns; ++i) {
	    if (rowPtr[i] != NULL) {
		fieldObj = Tcl_NewStringObj(rowPtr[i], lengths[i]);
	    } else {
		fieldObj = cdata->pidata->literals[LIT_EMPTY];
	    }
	    Tcl_ListObjAppendElement(NULL, rowObj, fieldObj);
	}
	Tcl_ListObjAppendElement(NULL, retObj, rowObj);
    }
    Tcl_SetObjResult(interp, retObj);

    /*
     * Free the result set.
     */
    mysql_free_result(resultPtr);

    return TCL_OK;
}


/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionNeedCollationInfoMethod --
 *
 *	Internal method that determines whether the collation lengths
 *	are known yet.
 *
 * Usage:
 *	$connection NeedCollationInfo
 *
 * Parameters:
 *	None.
 *
 * Results:
 *	Returns a Boolean value.
 *
 *-----------------------------------------------------------------------------
 */

static int
ConnectionNeedCollationInfoMethod(
    ClientData clientData,	/* Completion type */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext objectContext, /* Object context */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(objectContext);
				/* The current connection object */
    ConnectionData* cdata = (ConnectionData*)
	Tcl_ObjectGetMetadata(thisObject, &connectionDataType);
				/* Instance data */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(cdata->collationSizes == NULL));
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionRollbackMethod --
 *
 *	Method that requests that a pending transaction against a database
 * 	be rolled back.
 *
 * Usage:
 * 	$connection rollback
 *
 * Parameters:
 *	None.
 *
 * Results:
 *	Returns an empty Tcl result if successful, and throws an error
 *	otherwise.
 *
 *-----------------------------------------------------------------------------
 */

static int
ConnectionRollbackMethod(
    ClientData clientData,	/* Completion type */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext objectContext, /* Object context */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(objectContext);
				/* The current connection object */
    ConnectionData* cdata = (ConnectionData*)
	Tcl_ObjectGetMetadata(thisObject, &connectionDataType);
				/* Instance data */
    my_bool rc;		/* Result code from MySQL operations */

    /* Check parameters */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    /* Reject the request if no transaction is in progress */

    if (!(cdata->flags & CONN_FLAG_IN_XCN)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("no transaction is in "
						  "progress", -1));
	Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY010",
			 "MYSQL", "-1", NULL);
	return TCL_ERROR;
    }

    /* End transaction, turn off "transaction in progress", and report status */

    rc = mysql_rollback(cdata->mysqlPtr);
    cdata->flags &= ~CONN_FLAG_IN_XCN;
    if (rc) {
	TransferMysqlError(interp, cdata->mysqlPtr);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionSetCollationInfoMethod --
 *
 *	Internal method that saves the character lengths of the collations
 *
 * Usage:
 *	$connection SetCollationInfo {collationNum size} ...
 *
 * Parameters:
 *	One or more pairs of collation number and character length,
 *	ordered in decreasing sequence by collation number.
 *
 * Results:
 *	None.
 *
 * The [$connection columns $table] method needs to know the sizes
 * of characters in a given column's collation and character set.
 * This information is available by querying INFORMATION_SCHEMA, which
 * is easier to do from Tcl than C. This method passes in the results.
 *
 *-----------------------------------------------------------------------------
 */

static int
ConnectionSetCollationInfoMethod(
    ClientData clientData,	/* Completion type */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext objectContext, /* Object context */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(objectContext);
				/* The current connection object */
    ConnectionData* cdata = (ConnectionData*)
	Tcl_ObjectGetMetadata(thisObject, &connectionDataType);
				/* Instance data */
    int listLen;
    Tcl_Obj* objPtr;
    unsigned int collationNum;
    int i;
    int t;

    if (objc <= 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "{collationNum size}...");
	return TCL_ERROR;
    }
    if (Tcl_ListObjIndex(interp, objv[2], 0, &objPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objPtr, &t) != TCL_OK) {
	return TCL_ERROR;
    }
    cdata->nCollations = (unsigned int)(t+1);
    if (cdata->collationSizes) {
	ckfree((char*) cdata->collationSizes);
    }
    cdata->collationSizes =
	(int*) ckalloc(cdata->nCollations * sizeof(int));
    memset(cdata->collationSizes, 0, cdata->nCollations * sizeof(int));
    for (i = 2; i < objc; ++i) {
	if (Tcl_ListObjLength(interp, objv[i], &listLen) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (listLen != 2) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("args must be 2-element "
						      "lists", -1));
	    return TCL_ERROR;
	}
	if (Tcl_ListObjIndex(interp, objv[i], 0, &objPtr) != TCL_OK
	    || Tcl_GetIntFromObj(interp, objPtr, &t) != TCL_OK) {
	    return TCL_ERROR;
	}
	collationNum = (unsigned int) t;
	if (collationNum > cdata->nCollations) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("collations must be "
						      "in decreasing sequence",
						      -1));
	    return TCL_ERROR;
	}
	if ((Tcl_ListObjIndex(interp, objv[i], 1, &objPtr) != TCL_OK)
	    || (Tcl_GetIntFromObj(interp, objPtr,
				 cdata->collationSizes+collationNum)
		!= TCL_OK)) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConnectionTablesMethod --
 *
 *	Method that asks for the names of tables in the database (optionally
 *	matching a given pattern
 *
 * Usage:
 * 	$connection tables ?pattern?
 *
 * Parameters:
 *	None.
 *
 * Results:
 *	Returns the list of tables
 *
 *-----------------------------------------------------------------------------
 */

static int
ConnectionTablesMethod(
    ClientData clientData,	/* Completion type */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext objectContext, /* Object context */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(objectContext);
				/* The current connection object */
    ConnectionData* cdata = (ConnectionData*)
	Tcl_ObjectGetMetadata(thisObject, &connectionDataType);
				/* Instance data */
    Tcl_Obj** literals = cdata->pidata->literals;
				/* Literal pool */
    const char* patternStr = NULL;
				/* Pattern to match table names */
    MYSQL_RES* results = NULL;	/* Result set */
    MYSQL_ROW row = NULL;	/* Row in the result set */
    int status = TCL_OK;	/* Return status */
    Tcl_Obj* retval = NULL;	/* List of table names */

    /* Check parameters */

    if (objc == 2) {
	patternStr = NULL;
    } else if (objc == 3) {
	patternStr = Tcl_GetString(objv[2]);
    } else {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    results = mysql_list_tables(cdata->mysqlPtr, patternStr);
    if (results == NULL) {
	TransferMysqlError(interp, cdata->mysqlPtr);
	return TCL_ERROR;
    } else {
	retval = Tcl_NewObj();
	Tcl_IncrRefCount(retval);
	while ((row = mysql_fetch_row(results)) != NULL) {
	    unsigned long * lengths = mysql_fetch_lengths(results);
	    if (row[0]) {
		Tcl_ListObjAppendElement(NULL, retval,
					 Tcl_NewStringObj(row[0],
							  (int)lengths[0]));
		Tcl_ListObjAppendElement(NULL, retval, literals[LIT_EMPTY]);
	    }
	}
	if (mysql_errno(cdata->mysqlPtr)) {
	    TransferMysqlError(interp, cdata->mysqlPtr);
	    status = TCL_ERROR;
	}
	if (status == TCL_OK) {
	    Tcl_SetObjResult(interp, retval);
	}
	Tcl_DecrRefCount(retval);
	mysql_free_result(results);
	return status;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeleteCmd --
 *
 *	Callback executed when the initialization method of the connection
 *	class is deleted.
 *
 * Side effects:
 *	Dismisses the environment, which has the effect of shutting
 *	down MYSQL when it is no longer required.
 *
 *-----------------------------------------------------------------------------
 */

static void
DeleteCmd (
    ClientData clientData	/* Environment handle */
) {
    PerInterpData* pidata = (PerInterpData*) clientData;
    DecrPerInterpRefCount(pidata);
}

/*
 *-----------------------------------------------------------------------------
 *
 * CloneCmd --
 *
 *	Callback executed when any of the MYSQL client methods is cloned.
 *
 * Results:
 *	Returns TCL_OK to allow the method to be copied.
 *
 * Side effects:
 *	Obtains a fresh copy of the environment handle, to keep the
 *	refcounts accurate
 *
 *-----------------------------------------------------------------------------
 */

static int
CloneCmd(
    Tcl_Interp* interp,		/* Tcl interpreter */
    ClientData oldClientData,	/* Environment handle to be discarded */
    ClientData* newClientData	/* New environment handle to be used */
) {
    *newClientData = oldClientData;
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeleteConnectionMetadata, DeleteConnection --
 *
 *	Cleans up when a database connection is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Terminates the connection and frees all system resources associated
 *	with it.
 *
 *-----------------------------------------------------------------------------
 */

static void
DeleteConnectionMetadata(
    ClientData clientData	/* Instance data for the connection */
) {
    DecrConnectionRefCount((ConnectionData*)clientData);
}

static void
DeleteConnection(
    ConnectionData* cdata	/* Instance data for the connection */
) {
    if (cdata->collationSizes != NULL) {
	ckfree((char*) cdata->collationSizes);
    }
    if (cdata->mysqlPtr != NULL) {
	mysql_close(cdata->mysqlPtr);
    }
    DecrPerInterpRefCount(cdata->pidata);
    ckfree((char*) cdata);
}

/*
 *-----------------------------------------------------------------------------
 *
 * CloneConnection --
 *
 *	Attempts to clone an MYSQL connection's metadata.
 *
 * Results:
 *	Returns the new metadata
 *
 * At present, we don't attempt to clone connections - it's not obvious
 * that such an action would ever even make sense.  Instead, we return NULL
 * to indicate that the metadata should not be cloned. (Note that this
 * action isn't right, either. What *is* right is to indicate that the object
 * is not clonable, but the API gives us no way to do that.
 *
 *-----------------------------------------------------------------------------
 */

static int
CloneConnection(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    ClientData metadata,	/* Metadata to be cloned */
    ClientData* newMetaData	/* Where to put the cloned metadata */
) {
    Tcl_SetObjResult(interp,
		     Tcl_NewStringObj("MYSQL connections are not clonable", -1));
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * NewStatement --
 *
 *	Creates an empty object to hold statement data.
 *
 * Results:
 *	Returns a pointer to the newly-created object.
 *
 *-----------------------------------------------------------------------------
 */

static StatementData*
NewStatement(
    ConnectionData* cdata	/* Instance data for the connection */
) {
    StatementData* sdata = (StatementData*) ckalloc(sizeof(StatementData));
    sdata->refCount = 1;
    sdata->cdata = cdata;
    IncrConnectionRefCount(cdata);
    sdata->subVars = Tcl_NewObj();
    Tcl_IncrRefCount(sdata->subVars);
    sdata->params = NULL;
    sdata->nativeSql = NULL;
    sdata->stmtPtr = NULL;
    sdata->metadataPtr = NULL;
    sdata->columnNames = NULL;
    sdata->flags = 0;
    return sdata;
}

/*
 *-----------------------------------------------------------------------------
 *
 * AllocAndPrepareStatement --
 *
 *	Allocate space for a MySQL prepared statement, and prepare the
 *	statement.
 *
 * Results:
 *	Returns the statement handle if successful, and NULL on failure.
 *
 * Side effects:
 *	Prepares the statement.
 *	Stores error message and error code in the interpreter on failure.
 *
 *-----------------------------------------------------------------------------
 */

static MYSQL_STMT*
AllocAndPrepareStatement(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    StatementData* sdata	/* Statement data */
) {
    ConnectionData* cdata = sdata->cdata;
				/* Connection data */
    MYSQL_STMT* stmtPtr;	/* Statement handle */
    const char* nativeSqlStr;	/* Native SQL statement to prepare */
    int nativeSqlLen;		/* Length of the statement */

    /* Allocate space for the prepared statement */

    stmtPtr = mysql_stmt_init(cdata->mysqlPtr);
    /*
     * MySQL allows only one writable cursor open at a time, and
     * the default cursor type is writable. Make all our cursors
     * read-only to avoid 'Commands out of sync' errors.
     */

    if (stmtPtr == NULL) {
	TransferMysqlError(interp, cdata->mysqlPtr);
    } else {

	/* Prepare the statement */

	nativeSqlStr = Tcl_GetStringFromObj(sdata->nativeSql, &nativeSqlLen);
	if (mysql_stmt_prepare(stmtPtr, nativeSqlStr, nativeSqlLen)) {
	    TransferMysqlStmtError(interp, stmtPtr);
	    mysql_stmt_close(stmtPtr);
	    stmtPtr = NULL;
	}
    }
    return stmtPtr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ResultDescToTcl --
 *
 *	Converts a MySQL result description for return as a Tcl list.
 *
 * Results:
 *	Returns a Tcl object holding the result description
 *
 * If any column names are duplicated, they are disambiguated by
 * appending '#n' where n increments once for each occurrence of the
 * column name.
 *
 *-----------------------------------------------------------------------------
 */

static Tcl_Obj*
ResultDescToTcl(
    MYSQL_RES* result,		/* Result set description */
    int flags			/* Flags governing the conversion */
) {
    Tcl_Obj* retval = Tcl_NewObj();
    Tcl_HashTable names;	/* Hash table to resolve name collisions */
    Tcl_Obj* nameObj;		/* Name of a result column */
    int new;			/* Flag == 1 if a result column is unique */
    Tcl_HashEntry* entry;	/* Hash table entry for a column name */
    int count;			/* Number used to disambiguate a column name */

    Tcl_InitHashTable(&names, TCL_STRING_KEYS);
    if (result != NULL) {
	unsigned int fieldCount = mysql_num_fields(result);
	MYSQL_FIELD* fields = mysql_fetch_fields(result);
	unsigned int i;
	char numbuf[16];
	for (i = 0; i < fieldCount; ++i) {
	    MYSQL_FIELD* field = MysqlFieldIndex(fields, i);
	    nameObj = Tcl_NewStringObj(field->name, field->name_length);
	    Tcl_IncrRefCount(nameObj);
	    entry = Tcl_CreateHashEntry(&names, field->name, &new);
	    count = 1;
	    while (!new) {
		count = PTR2INT(Tcl_GetHashValue(entry));
		++count;
		Tcl_SetHashValue(entry, INT2PTR(count));
		sprintf(numbuf, "#%d", count);
		Tcl_AppendToObj(nameObj, numbuf, -1);
		entry = Tcl_CreateHashEntry(&names, Tcl_GetString(nameObj),
					    &new);
	    }
	    Tcl_SetHashValue(entry, INT2PTR(count));
	    Tcl_ListObjAppendElement(NULL, retval, nameObj);
	    Tcl_DecrRefCount(nameObj);
	}
    }
    Tcl_DeleteHashTable(&names);
    return retval;
}

/*
 *-----------------------------------------------------------------------------
 *
 * StatementConstructor --
 *
 *	C-level initialization for the object representing an MySQL prepared
 *	statement.
 *
 * Usage:
 *	statement new connection statementText
 *	statement create name connection statementText
 *
 * Parameters:
 *      connection -- the MySQL connection object
 *	statementText -- text of the statement to prepare.
 *
 * Results:
 *	Returns a standard Tcl result
 *
 * Side effects:
 *	Prepares the statement, and stores it (plus a reference to the
 *	connection) in instance metadata.
 *
 *-----------------------------------------------------------------------------
 */

static int
StatementConstructor(
    ClientData clientData,	/* Not used */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext context,	/* Object context  */
    int objc, 			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {

    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current statement object */
    int skip = Tcl_ObjectContextSkippedArgs(context);
				/* Number of args to skip before the
				 * payload arguments */
    Tcl_Object connectionObject;
				/* The database connection as a Tcl_Object */
    ConnectionData* cdata;	/* The connection object's data */
    StatementData* sdata;	/* The statement's object data */
    Tcl_Obj* tokens;		/* The tokens of the statement to be prepared */
    int tokenc;			/* Length of the 'tokens' list */
    Tcl_Obj** tokenv;		/* Exploded tokens from the list */
    Tcl_Obj* nativeSql;		/* SQL statement mapped to native form */
    char* tokenStr;		/* Token string */
    int tokenLen;		/* Length of a token */
    int nParams;		/* Number of parameters of the statement */
    int i;

    /* Find the connection object, and get its data. */

    thisObject = Tcl_ObjectContextObject(context);
    if (objc != skip+2) {
	Tcl_WrongNumArgs(interp, skip, objv, "connection statementText");
	return TCL_ERROR;
    }

    connectionObject = Tcl_GetObjectFromObj(interp, objv[skip]);
    if (connectionObject == NULL) {
	return TCL_ERROR;
    }
    cdata = (ConnectionData*) Tcl_ObjectGetMetadata(connectionObject,
						    &connectionDataType);
    if (cdata == NULL) {
	Tcl_AppendResult(interp, Tcl_GetString(objv[skip]),
			 " does not refer to a MySQL connection", NULL);
	return TCL_ERROR;
    }

    /*
     * Allocate an object to hold data about this statement
     */

    sdata = NewStatement(cdata);

    /* Tokenize the statement */

    tokens = Tdbc_TokenizeSql(interp, Tcl_GetString(objv[skip+1]));
    if (tokens == NULL) {
	goto freeSData;
    }
    Tcl_IncrRefCount(tokens);

    /*
     * Rewrite the tokenized statement to MySQL syntax. Reject the
     * statement if it is actually multiple statements.
     */

    if (Tcl_ListObjGetElements(interp, tokens, &tokenc, &tokenv) != TCL_OK) {
	goto freeTokens;
    }
    nativeSql = Tcl_NewObj();
    Tcl_IncrRefCount(nativeSql);
    for (i = 0; i < tokenc; ++i) {
	tokenStr = Tcl_GetStringFromObj(tokenv[i], &tokenLen);

	switch (tokenStr[0]) {
	case '$':
	case ':':
	case '@':
	    Tcl_AppendToObj(nativeSql, "?", 1);
	    Tcl_ListObjAppendElement(NULL, sdata->subVars,
				     Tcl_NewStringObj(tokenStr+1, tokenLen-1));
	    break;

	case ';':
	    Tcl_SetObjResult(interp,
			     Tcl_NewStringObj("tdbc::mysql"
					      " does not support semicolons "
					      "in statements", -1));
	    goto freeNativeSql;
	    break;

	default:
	    Tcl_AppendToObj(nativeSql, tokenStr, tokenLen);
	    break;

	}
    }
    sdata->nativeSql = nativeSql;
    Tcl_DecrRefCount(tokens);

    /* Prepare the statement */

    sdata->stmtPtr = AllocAndPrepareStatement(interp, sdata);
    if (sdata->stmtPtr == NULL) {
	goto freeSData;
    }

    /* Get result set metadata */

    sdata->metadataPtr = mysql_stmt_result_metadata(sdata->stmtPtr);
    if (mysql_stmt_errno(sdata->stmtPtr)) {
	TransferMysqlStmtError(interp, sdata->stmtPtr);
	goto freeSData;
    }
    sdata->columnNames = ResultDescToTcl(sdata->metadataPtr, 0);
    Tcl_IncrRefCount(sdata->columnNames);

    Tcl_ListObjLength(NULL, sdata->subVars, &nParams);
    sdata->params = (ParamData*) ckalloc(nParams * sizeof(ParamData));
    for (i = 0; i < nParams; ++i) {
	sdata->params[i].flags = PARAM_IN;
	sdata->params[i].dataType = MYSQL_TYPE_VARCHAR;
	sdata->params[i].precision = 0;
	sdata->params[i].scale = 0;
    }

    /* Attach the current statement data as metadata to the current object */

    Tcl_ObjectSetMetadata(thisObject, &statementDataType, (ClientData) sdata);
    return TCL_OK;

    /* On error, unwind all the resource allocations */

 freeNativeSql:
    Tcl_DecrRefCount(nativeSql);
 freeTokens:
    Tcl_DecrRefCount(tokens);
 freeSData:
    DecrStatementRefCount(sdata);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * StatementParamsMethod --
 *
 *	Lists the parameters in a MySQL statement.
 *
 * Usage:
 *	$statement params
 *
 * Results:
 *	Returns a standard Tcl result containing a dictionary. The keys
 *	of the dictionary are parameter names, and the values are parameter
 *	types, themselves expressed as dictionaries containing the keys,
 *	'name', 'direction', 'type', 'precision', 'scale' and 'nullable'.
 *
 *
 *-----------------------------------------------------------------------------
 */

static int
StatementParamsMethod(
    ClientData clientData,	/* Not used */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext context,	/* Object context  */
    int objc, 			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current statement object */
    StatementData* sdata	/* The current statement */
	= (StatementData*) Tcl_ObjectGetMetadata(thisObject,
						 &statementDataType);
    ConnectionData* cdata = sdata->cdata;
    PerInterpData* pidata = cdata->pidata; /* Per-interp data */
    Tcl_Obj** literals = pidata->literals; /* Literal pool */
    int nParams;		/* Number of parameters to the statement */
    Tcl_Obj* paramName;		/* Name of a parameter */
    Tcl_Obj* paramDesc;		/* Description of one parameter */
    Tcl_Obj* dataTypeName;	/* Name of a parameter's data type */
    Tcl_Obj* retVal;		/* Return value from this command */
    Tcl_HashEntry* typeHashEntry;
    int i;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    retVal = Tcl_NewObj();
    Tcl_ListObjLength(NULL, sdata->subVars, &nParams);
    for (i = 0; i < nParams; ++i) {
	paramDesc = Tcl_NewObj();
	Tcl_ListObjIndex(NULL, sdata->subVars, i, &paramName);
	Tcl_DictObjPut(NULL, paramDesc, literals[LIT_NAME], paramName);
	switch (sdata->params[i].flags & (PARAM_IN | PARAM_OUT)) {
	case PARAM_IN:
	    Tcl_DictObjPut(NULL, paramDesc, literals[LIT_DIRECTION],
			   literals[LIT_IN]);
	    break;
	case PARAM_OUT:
	    Tcl_DictObjPut(NULL, paramDesc, literals[LIT_DIRECTION],
			   literals[LIT_OUT]);
	    break;
	case PARAM_IN | PARAM_OUT:
	    Tcl_DictObjPut(NULL, paramDesc, literals[LIT_DIRECTION],
			   literals[LIT_INOUT]);
	    break;
	default:
	    break;
	}
	typeHashEntry =
	    Tcl_FindHashEntry(&(pidata->typeNumHash),
			      INT2PTR(sdata->params[i].dataType));
	if (typeHashEntry != NULL) {
	    dataTypeName = (Tcl_Obj*) Tcl_GetHashValue(typeHashEntry);
	    Tcl_DictObjPut(NULL, paramDesc, literals[LIT_TYPE], dataTypeName);
	}
	Tcl_DictObjPut(NULL, paramDesc, literals[LIT_PRECISION],
		       Tcl_NewIntObj(sdata->params[i].precision));
	Tcl_DictObjPut(NULL, paramDesc, literals[LIT_SCALE],
		       Tcl_NewIntObj(sdata->params[i].scale));
	Tcl_DictObjPut(NULL, retVal, paramName, paramDesc);
    }

    Tcl_SetObjResult(interp, retVal);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * StatementParamtypeMethod --
 *
 *	Defines a parameter type in a MySQL statement.
 *
 * Usage:
 *	$statement paramtype paramName ?direction? type ?precision ?scale??
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Updates the description of the given parameter.
 *
 *-----------------------------------------------------------------------------
 */

static int
StatementParamtypeMethod(
    ClientData clientData,	/* Not used */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext context,	/* Object context  */
    int objc, 			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {
    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current statement object */
    StatementData* sdata	/* The current statement */
	= (StatementData*) Tcl_ObjectGetMetadata(thisObject,
						 &statementDataType);
    static const struct {
	const char* name;
	int flags;
    } directions[] = {
	{ "in", 	PARAM_IN },
	{ "out",	PARAM_OUT },
	{ "inout",	PARAM_IN | PARAM_OUT },
	{ NULL,		0 }
    };
    int direction;
    int typeNum;		/* Data type number of a parameter */
    int precision;		/* Data precision */
    int scale;			/* Data scale */

    int nParams;		/* Number of parameters to the statement */
    const char* paramName;	/* Name of the parameter being set */
    Tcl_Obj* targetNameObj;	/* Name of the ith parameter in the statement */
    const char* targetName;	/* Name of a candidate parameter in the
				 * statement */
    int matchCount = 0;		/* Number of parameters matching the name */
    Tcl_Obj* errorObj;		/* Error message */

    int i;

    /* Check parameters */

    if (objc < 4) {
	goto wrongNumArgs;
    }

    i = 3;
    if (Tcl_GetIndexFromObjStruct(interp, objv[i], directions,
				  sizeof(directions[0]), "direction",
				  TCL_EXACT, &direction) != TCL_OK) {
	direction = PARAM_IN;
	Tcl_ResetResult(interp);
    } else {
	++i;
    }
    if (i >= objc) goto wrongNumArgs;
    if (Tcl_GetIndexFromObjStruct(interp, objv[i], dataTypes,
				  sizeof(dataTypes[0]), "SQL data type",
				  TCL_EXACT, &typeNum) == TCL_OK) {
	++i;
    } else {
	return TCL_ERROR;
    }
    if (i < objc) {
	if (Tcl_GetIntFromObj(interp, objv[i], &precision) == TCL_OK) {
	    ++i;
	} else {
	    return TCL_ERROR;
	}
    }
    if (i < objc) {
	if (Tcl_GetIntFromObj(interp, objv[i], &scale) == TCL_OK) {
	    ++i;
	} else {
	    return TCL_ERROR;
	}
    }
    if (i != objc) {
	goto wrongNumArgs;
    }

    /* Look up parameters by name. */

    Tcl_ListObjLength(NULL, sdata->subVars, &nParams);
    paramName = Tcl_GetString(objv[2]);
    for (i = 0; i < nParams; ++i) {
	Tcl_ListObjIndex(NULL, sdata->subVars, i, &targetNameObj);
	targetName = Tcl_GetString(targetNameObj);
	if (!strcmp(paramName, targetName)) {
	    ++matchCount;
	    sdata->params[i].flags = direction;
	    sdata->params[i].dataType = dataTypes[typeNum].num;
	    sdata->params[i].precision = precision;
	    sdata->params[i].scale = scale;
	}
    }
    if (matchCount == 0) {
	errorObj = Tcl_NewStringObj("unknown parameter \"", -1);
	Tcl_AppendToObj(errorObj, paramName, -1);
	Tcl_AppendToObj(errorObj, "\": must be ", -1);
	for (i = 0; i < nParams; ++i) {
	    Tcl_ListObjIndex(NULL, sdata->subVars, i, &targetNameObj);
	    Tcl_AppendObjToObj(errorObj, targetNameObj);
	    if (i < nParams-2) {
		Tcl_AppendToObj(errorObj, ", ", -1);
	    } else if (i == nParams-2) {
		Tcl_AppendToObj(errorObj, " or ", -1);
	    }
	}
	Tcl_SetObjResult(interp, errorObj);
	return TCL_ERROR;
    }

    return TCL_OK;

 wrongNumArgs:
    Tcl_WrongNumArgs(interp, 2, objv,
		     "name ?direction? type ?precision ?scale??");
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeleteStatementMetadata, DeleteStatement --
 *
 *	Cleans up when a MySQL statement is no longer required.
 *
 * Side effects:
 *	Frees all resources associated with the statement.
 *
 *-----------------------------------------------------------------------------
 */

static void
DeleteStatementMetadata(
    ClientData clientData	/* Instance data for the connection */
) {
    DecrStatementRefCount((StatementData*)clientData);
}
static void
DeleteStatement(
    StatementData* sdata	/* Metadata for the statement */
) {
    if (sdata->columnNames != NULL) {
	Tcl_DecrRefCount(sdata->columnNames);
    }
    if (sdata->metadataPtr != NULL) {
	mysql_free_result(sdata->metadataPtr);
    }
    if (sdata->stmtPtr != NULL) {
	mysql_stmt_close(sdata->stmtPtr);
    }
    if (sdata->nativeSql != NULL) {
	Tcl_DecrRefCount(sdata->nativeSql);
    }
    if (sdata->params != NULL) {
	ckfree((char*)sdata->params);
    }
    Tcl_DecrRefCount(sdata->subVars);
    DecrConnectionRefCount(sdata->cdata);
    ckfree((char*)sdata);
}

/*
 *-----------------------------------------------------------------------------
 *
 * CloneStatement --
 *
 *	Attempts to clone a MySQL statement's metadata.
 *
 * Results:
 *	Returns the new metadata
 *
 * At present, we don't attempt to clone statements - it's not obvious
 * that such an action would ever even make sense.  Instead, we return NULL
 * to indicate that the metadata should not be cloned. (Note that this
 * action isn't right, either. What *is* right is to indicate that the object
 * is not clonable, but the API gives us no way to do that.
 *
 *-----------------------------------------------------------------------------
 */

static int
CloneStatement(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    ClientData metadata,	/* Metadata to be cloned */
    ClientData* newMetaData	/* Where to put the cloned metadata */
) {
    Tcl_SetObjResult(interp,
		     Tcl_NewStringObj("MySQL statements are not clonable", -1));
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ResultSetConstructor --
 *
 *	Constructs a new result set.
 *
 * Usage:
 *	$resultSet new statement ?dictionary?
 *	$resultSet create name statement ?dictionary?
 *
 * Parameters:
 *	statement -- Statement handle to which this resultset belongs
 *	dictionary -- Dictionary containing the substitutions for named
 *		      parameters in the given statement.
 *
 * Results:
 *	Returns a standard Tcl result.  On error, the interpreter result
 *	contains an appropriate message.
 *
 *-----------------------------------------------------------------------------
 */

static int
ResultSetConstructor(
    ClientData clientData,	/* Not used */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext context,	/* Object context  */
    int objc, 			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {

    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current result set object */
    int skip = Tcl_ObjectContextSkippedArgs(context);
				/* Number of args to skip */
    Tcl_Object statementObject;	/* The current statement object */
    ConnectionData* cdata;	/* The MySQL connection object's data */
    StatementData* sdata;	/* The statement object's data */
    ResultSetData* rdata;	/* THe result set object's data */
    int nParams;		/* The parameter count on the statement */
    int nBound;			/* Number of parameters bound so far */
    Tcl_Obj* paramNameObj;	/* Name of the current parameter */
    const char* paramName;	/* Name of the current parameter */
    Tcl_Obj* paramValObj;	/* Value of the current parameter */
    const char* paramValStr;	/* String value of the current parameter */
    char* bufPtr;		/* Pointer to the parameter buffer */
    int len;			/* Length of a bound parameter */
    int nColumns;		/* Number of columns in the result set */
    MYSQL_FIELD* fields = NULL;	/* Description of columns of the result set */
    MYSQL_BIND* resultBindings;	/* Bindings of the columns of the result set */
    unsigned long* resultLengths;
				/* Lengths of the columns of the result set */
    int i;

    /* Check parameter count */

    if (objc != skip+1 && objc != skip+2) {
	Tcl_WrongNumArgs(interp, skip, objv, "statement ?dictionary?");
	return TCL_ERROR;
    }

    /* Initialize the base classes */

    Tcl_ObjectContextInvokeNext(interp, context, skip, objv, skip);

    /* Find the statement object, and get the statement data */

    statementObject = Tcl_GetObjectFromObj(interp, objv[skip]);
    if (statementObject == NULL) {
	return TCL_ERROR;
    }
    sdata = (StatementData*) Tcl_ObjectGetMetadata(statementObject,
						   &statementDataType);
    if (sdata == NULL) {
	Tcl_AppendResult(interp, Tcl_GetString(objv[skip]),
			 " does not refer to a MySQL statement", NULL);
	return TCL_ERROR;
    }
    Tcl_ListObjLength(NULL, sdata->columnNames, &nColumns);
    cdata = sdata->cdata;

    /*
     * If there is no transaction in progress, turn on auto-commit so that
     * this statement will execute directly.
     */

    if ((cdata->flags & (CONN_FLAG_IN_XCN | CONN_FLAG_AUTOCOMMIT)) == 0) {
	if (mysql_autocommit(cdata->mysqlPtr, 1)) {
	    TransferMysqlError(interp, cdata->mysqlPtr);
	    return TCL_ERROR;
	}
	cdata->flags |= CONN_FLAG_AUTOCOMMIT;
    }

    /* Allocate an object to hold data about this result set */

    rdata = (ResultSetData*) ckalloc(sizeof(ResultSetData));
    rdata->refCount = 1;
    rdata->sdata = sdata;
    rdata->stmtPtr = NULL;
    rdata->paramValues = NULL;
    rdata->paramBindings = NULL;
    rdata->paramLengths = NULL;
    rdata->rowCount = 0;
    rdata->resultErrors = (my_bool*) ckalloc(nColumns * sizeof(my_bool));
    rdata->resultNulls = (my_bool*) ckalloc(nColumns * sizeof(my_bool));
    resultLengths = rdata->resultLengths = (unsigned long*)
	ckalloc(nColumns * sizeof(unsigned long));
    rdata->resultBindings = resultBindings = MysqlBindAlloc(nColumns);
    IncrStatementRefCount(sdata);
    Tcl_ObjectSetMetadata(thisObject, &resultSetDataType, (ClientData) rdata);

    /* Make bindings for all the result columns. Defer binding variable
     * length fields until first execution. */

    if (nColumns > 0) {
	fields = mysql_fetch_fields(sdata->metadataPtr);
    }
    for (i = 0; i < nColumns; ++i) {
	MYSQL_FIELD* field = MysqlFieldIndex(fields, i);
	switch (field->type) {

	case MYSQL_TYPE_FLOAT:
	case MYSQL_TYPE_DOUBLE:
	    MysqlBindSetBufferType(resultBindings, i,  MYSQL_TYPE_DOUBLE);
	    MysqlBindAllocBuffer(resultBindings, i, sizeof(double));
	    resultLengths[i] = sizeof(double);
	    break;

	case MYSQL_TYPE_BIT:
	    MysqlBindSetBufferType(resultBindings, i,  MYSQL_TYPE_BIT);
	    MysqlBindAllocBuffer(resultBindings, i, field->length);
	    resultLengths[i] = field->length;
	    break;

	case MYSQL_TYPE_LONGLONG:
	    MysqlBindSetBufferType(resultBindings, i,  MYSQL_TYPE_LONGLONG);
	    MysqlBindAllocBuffer(resultBindings, i, sizeof(Tcl_WideInt));
	    resultLengths[i] = sizeof(Tcl_WideInt);
	    break;

	case MYSQL_TYPE_TINY:
	case MYSQL_TYPE_SHORT:
	case MYSQL_TYPE_INT24:
	case MYSQL_TYPE_LONG:
	    MysqlBindSetBufferType(resultBindings, i,  MYSQL_TYPE_LONG);
	    MysqlBindAllocBuffer(resultBindings, i, sizeof(int));
	    resultLengths[i] = sizeof(int);
	    break;

	default:
	    MysqlBindSetBufferType(resultBindings, i,  MYSQL_TYPE_STRING);
	    MysqlBindAllocBuffer(resultBindings, i, 0);
	    resultLengths[i] = 0;
	    break;
	}
	MysqlBindSetLength(resultBindings, i, rdata->resultLengths + i);
	rdata->resultNulls[i] = 0;
	MysqlBindSetIsNull(resultBindings, i, rdata->resultNulls + i);
	rdata->resultErrors[i] = 0;
	MysqlBindSetError(resultBindings, i, rdata->resultErrors + i);
    }

    /*
     * Find a statement handle that we can use to execute the SQL code.
     * If the main statement handle associated with the statement
     * is idle, we can use it.  Otherwise, we have to allocate and
     * prepare a fresh one.
     */

    if (sdata->flags & STMT_FLAG_BUSY) {
	rdata->stmtPtr = AllocAndPrepareStatement(interp, sdata);
	if (rdata->stmtPtr == NULL) {
	    return TCL_ERROR;
	}
    } else {
	rdata->stmtPtr = sdata->stmtPtr;
	sdata->flags |= STMT_FLAG_BUSY;
    }

    /* Allocate the parameter bindings */

    Tcl_ListObjLength(NULL, sdata->subVars, &nParams);
    rdata->paramValues = Tcl_NewObj();
    Tcl_IncrRefCount(rdata->paramValues);
    rdata->paramBindings = MysqlBindAlloc(nParams);
    rdata->paramLengths = (unsigned long*) ckalloc(nParams
						   * sizeof(unsigned long));
    for (nBound = 0; nBound < nParams; ++nBound) {
	MysqlBindSetBufferType(rdata->paramBindings, nBound, MYSQL_TYPE_NULL);
    }

    /* Bind the substituted parameters */

    for (nBound = 0; nBound < nParams; ++nBound) {
	Tcl_ListObjIndex(NULL, sdata->subVars, nBound, &paramNameObj);
	paramName = Tcl_GetString(paramNameObj);
	if (objc == skip+2) {

	    /* Param from a dictionary */

	    if (Tcl_DictObjGet(interp, objv[skip+1],
			       paramNameObj, &paramValObj) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {

	    /* Param from a variable */

	    paramValObj = Tcl_GetVar2Ex(interp, paramName, NULL,
					TCL_LEAVE_ERR_MSG);
	}

	/*
	 * At this point, paramValObj contains the parameter to bind.
	 * Convert the parameters to the appropriate data types for
	 * MySQL's prepared statement interface, and bind them.
	 */

	if (paramValObj != NULL) {
	    switch (sdata->params[nBound].dataType & 0xffff) {

	    case MYSQL_TYPE_NEWDECIMAL:
	    case MYSQL_TYPE_DECIMAL:
		if (sdata->params[nBound].scale == 0) {
		    if (sdata->params[nBound].precision < 10) {
			goto smallinteger;
		    } else if (sdata->params[nBound].precision < 19) {
			goto biginteger;
		    } else {
			goto charstring;
		    }
		} else if (sdata->params[nBound].precision < 17) {
		    goto real;
		} else {
		    goto charstring;
		}

	    case MYSQL_TYPE_FLOAT:
	    case MYSQL_TYPE_DOUBLE:
	    real:
		MysqlBindSetBufferType(rdata->paramBindings, nBound,
				       MYSQL_TYPE_DOUBLE);
		bufPtr = MysqlBindAllocBuffer(rdata->paramBindings,
					      nBound, sizeof(double));
		rdata->paramLengths[nBound] = sizeof(double);
		MysqlBindSetLength(rdata->paramBindings, nBound,
				   &(rdata->paramLengths[nBound]));
		if (Tcl_GetDoubleFromObj(interp, paramValObj,
					 (double*) bufPtr) != TCL_OK) {
		    return TCL_ERROR;
		}
		break;

	    case MYSQL_TYPE_BIT:
	    case MYSQL_TYPE_LONGLONG:
	    biginteger:
		MysqlBindSetBufferType(rdata->paramBindings, nBound,
				       MYSQL_TYPE_LONGLONG);
		bufPtr = MysqlBindAllocBuffer(rdata->paramBindings, nBound,
					      sizeof(Tcl_WideInt));
		rdata->paramLengths[nBound] = sizeof(Tcl_WideInt);
		MysqlBindSetLength(rdata->paramBindings, nBound,
				   &(rdata->paramLengths[nBound]));
		if (Tcl_GetWideIntFromObj(interp, paramValObj,
					  (Tcl_WideInt*) bufPtr) != TCL_OK) {
		    return TCL_ERROR;
		}
		break;

	    case MYSQL_TYPE_TINY:
	    case MYSQL_TYPE_SHORT:
	    case MYSQL_TYPE_INT24:
	    case MYSQL_TYPE_LONG:
	    smallinteger:
		MysqlBindSetBufferType(rdata->paramBindings, nBound,
				       MYSQL_TYPE_LONG);
		bufPtr = MysqlBindAllocBuffer(rdata->paramBindings, nBound,
					      sizeof(int));
		rdata->paramLengths[nBound] = sizeof(int);
		MysqlBindSetLength(rdata->paramBindings, nBound,
				   &(rdata->paramLengths[nBound]));
		if (Tcl_GetIntFromObj(interp, paramValObj,
				      (int*) bufPtr) != TCL_OK) {
		    return TCL_ERROR;
		}
		break;

	    default:
	    charstring:
		Tcl_ListObjAppendElement(NULL, rdata->paramValues, paramValObj);
		if (sdata->params[nBound].dataType & IS_BINARY) {
		    MysqlBindSetBufferType(rdata->paramBindings, nBound,
					   MYSQL_TYPE_BLOB);
		    paramValStr = (char*)
			Tcl_GetByteArrayFromObj(paramValObj, &len);
		} else {
		    MysqlBindSetBufferType(rdata->paramBindings, nBound,
					   MYSQL_TYPE_STRING);
		    paramValStr = Tcl_GetStringFromObj(paramValObj, &len);
		}
		bufPtr = MysqlBindAllocBuffer(rdata->paramBindings, nBound,
					      len+1);
		memcpy(bufPtr, paramValStr, len);
		rdata->paramLengths[nBound] = len;
		MysqlBindSetLength(rdata->paramBindings, nBound,
				   &(rdata->paramLengths[nBound]));
		break;

	    }
	} else {
	    MysqlBindSetBufferType(rdata->paramBindings, nBound,
				   MYSQL_TYPE_NULL);
	}
    }

    /* Execute the statement */

    /*
     * It is tempting to conserve client memory here by omitting
     * the call to 'mysql_stmt_store_result', but doing so causes
     * 'calls out of sync' errors when attempting to prepare a
     * statement while a result set is open. Certain of these errors
     * can, in turn, be avoided by using mysql_stmt_set_attr
     * and turning on "CURSOR_MODE_READONLY", but that, in turn
     * causes the server summarily to disconnect the client in
     * some tests.
     */

    if (mysql_stmt_bind_param(rdata->stmtPtr, rdata->paramBindings)
	|| ((nColumns > 0) && mysql_stmt_bind_result(rdata->stmtPtr,
						     resultBindings))
	|| mysql_stmt_execute(rdata->stmtPtr)
	|| mysql_stmt_store_result(rdata->stmtPtr) ) {
	TransferMysqlStmtError(interp, sdata->stmtPtr);
	return TCL_ERROR;
    }

    /* Determine and store the row count */

    rdata->rowCount = mysql_stmt_affected_rows(sdata->stmtPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ResultSetColumnsMethod --
 *
 *	Retrieves the list of columns from a result set.
 *
 * Usage:
 *	$resultSet columns
 *
 * Results:
 *	Returns the count of columns
 *
 *-----------------------------------------------------------------------------
 */

static int
ResultSetColumnsMethod(
    ClientData clientData,	/* Not used */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext context,	/* Object context  */
    int objc, 			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {

    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current result set object */
    ResultSetData* rdata = (ResultSetData*)
	Tcl_ObjectGetMetadata(thisObject, &resultSetDataType);
    StatementData* sdata = (StatementData*) rdata->sdata;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "?pattern?");
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, sdata->columnNames);

    return TCL_OK;

}

/*
 *-----------------------------------------------------------------------------
 *
 * ResultSetNextrowMethod --
 *
 *	Retrieves the next row from a result set.
 *
 * Usage:
 *	$resultSet nextrow ?-as lists|dicts? ?--? variableName
 *
 * Options:
 *	-as	Selects the desired form for returning the results.
 *
 * Parameters:
 *	variableName -- Variable in which the results are to be returned
 *
 * Results:
 *	Returns a standard Tcl result.  The interpreter result is 1 if there
 *	are more rows remaining, and 0 if no more rows remain.
 *
 * Side effects:
 *	Stores in the given variable either a list or a dictionary
 *	containing one row of the result set.
 *
 *-----------------------------------------------------------------------------
 */

static int
ResultSetNextrowMethod(
    ClientData clientData,	/* Not used */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext context,	/* Object context  */
    int objc, 			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {

    int lists = PTR2INT(clientData);
				/* Flag == 1 if lists are to be returned,
				 * 0 if dicts are to be returned */

    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current result set object */
    ResultSetData* rdata = (ResultSetData*)
	Tcl_ObjectGetMetadata(thisObject, &resultSetDataType);
				/* Data pertaining to the current result set */
    StatementData* sdata = (StatementData*) rdata->sdata;
				/* Statement that yielded the result set */
    ConnectionData* cdata = (ConnectionData*) sdata->cdata;
				/* Connection that opened the statement */
    PerInterpData* pidata = (PerInterpData*) cdata->pidata;
				/* Per interpreter data */
    Tcl_Obj** literals = pidata->literals;
				/* Literal pool */

    int nColumns = 0;		/* Number of columns in the result set */
    Tcl_Obj* colName;		/* Name of the current column */
    Tcl_Obj* resultRow;		/* Row of the result set under construction */

    Tcl_Obj* colObj;		/* Column obtained from the row */
    int status = TCL_ERROR;	/* Status return from this command */
    MYSQL_FIELD* fields;	/* Fields of the result set */
    MYSQL_BIND* resultBindings = rdata->resultBindings;
				/* Descriptions of the results */
    unsigned long* resultLengths = rdata->resultLengths;
				/* String lengths of the results */
    my_bool* resultNulls = rdata->resultNulls;
				/* Indicators that the results are null */
    void* bufPtr;		/* Pointer to a result's buffer */
    unsigned char byte;		/* One byte extracted from a bit field */
    Tcl_WideInt bitVal;		/* Value of a bit field */
    int mysqlStatus;		/* Status return from MySQL */
    int i;
    unsigned int j;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "varName");
	return TCL_ERROR;
    }


    /* Get the column names in the result set. */

    Tcl_ListObjLength(NULL, sdata->columnNames, &nColumns);
    if (nColumns == 0) {
	Tcl_SetObjResult(interp, literals[LIT_0]);
	return TCL_OK;
    }

    resultRow = Tcl_NewObj();
    Tcl_IncrRefCount(resultRow);

    /*
     * Try to rebind the result set before doing the next fetch
     */

    fields = mysql_fetch_fields(sdata->metadataPtr);
    if (mysql_stmt_bind_result(rdata->stmtPtr, resultBindings)) {
	goto cleanup;
    }

    /* Fetch the row to determine sizes. */

    mysqlStatus = mysql_stmt_fetch(rdata->stmtPtr);
    if (mysqlStatus != 0 && mysqlStatus != MYSQL_DATA_TRUNCATED) {
	if (mysqlStatus == MYSQL_NO_DATA) {
	    Tcl_SetObjResult(interp, literals[LIT_0]);
	    status = TCL_OK;
	}
	goto cleanup;
    }

    /* Retrieve one column at a time. */

    for (i = 0; i < nColumns; ++i) {
	MYSQL_FIELD* field = MysqlFieldIndex(fields, i);
	colObj = NULL;
	if (!resultNulls[i]) {
	    if (resultLengths[i]
		> MysqlBindGetBufferLength(resultBindings, i)) {
		MysqlBindFreeBuffer(resultBindings, i);
		MysqlBindAllocBuffer(resultBindings, i, resultLengths[i] + 1);
		if (mysql_stmt_fetch_column(rdata->stmtPtr,
					    MysqlBindIndex(resultBindings, i),
					    i, 0)) {
		    goto cleanup;
		}
	    }
	    bufPtr = MysqlBindGetBuffer(resultBindings, i);
	    switch (MysqlBindGetBufferType(resultBindings, i)) {

	    case MYSQL_TYPE_BIT:
		bitVal = 0;
		for (j = 0; j < resultLengths[i]; ++j) {
		    byte = ((unsigned char*) bufPtr)[resultLengths[i]-1-j];
		    bitVal |= (byte << (8*j));
		}
		colObj = Tcl_NewWideIntObj(bitVal);
		break;

	    case MYSQL_TYPE_DOUBLE:
		colObj = Tcl_NewDoubleObj(*(double*) bufPtr);
		break;

	    case MYSQL_TYPE_LONG:
		colObj = Tcl_NewIntObj(*(int*) bufPtr);
		break;

	    case MYSQL_TYPE_LONGLONG:
		colObj = Tcl_NewWideIntObj(*(Tcl_WideInt*) bufPtr);
		break;

	    default:
		if (field->charsetnr == 63) {
		    colObj = Tcl_NewByteArrayObj((unsigned char*) bufPtr,
						 resultLengths[i]);
		} else {
		    colObj = Tcl_NewStringObj((char*) bufPtr,
					      resultLengths[i]);
		}
		break;
	    }
	}

	if (lists) {
	    if (colObj == NULL) {
		colObj = literals[LIT_EMPTY];
	    }
	    Tcl_ListObjAppendElement(NULL, resultRow, colObj);
	} else {
	    if (colObj != NULL) {
		Tcl_ListObjIndex(NULL, sdata->columnNames, i, &colName);
		Tcl_DictObjPut(NULL, resultRow, colName, colObj);
	    }
	}
    }

    /* Save the row in the given variable */

    if (Tcl_SetVar2Ex(interp, Tcl_GetString(objv[2]), NULL,
		      resultRow, TCL_LEAVE_ERR_MSG) == NULL) {
	goto cleanup;
    }

    Tcl_SetObjResult(interp, literals[LIT_1]);
    status = TCL_OK;

 cleanup:
    if (status != TCL_OK) {
	TransferMysqlStmtError(interp, rdata->stmtPtr);
    }
    Tcl_DecrRefCount(resultRow);
    return status;

}

/*
 *-----------------------------------------------------------------------------
 *
 * ResultSetRowcountMethod --
 *
 *	Returns (if known) the number of rows affected by a MySQL statement.
 *
 * Usage:
 *	$resultSet rowcount
 *
 * Results:
 *	Returns a standard Tcl result giving the number of affected rows.
 *
 *-----------------------------------------------------------------------------
 */

static int
ResultSetRowcountMethod(
    ClientData clientData,	/* Not used */
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_ObjectContext context,	/* Object context  */
    int objc, 			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {

    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current result set object */
    ResultSetData* rdata = (ResultSetData*)
	Tcl_ObjectGetMetadata(thisObject, &resultSetDataType);
				/* Data pertaining to the current result set */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
		     Tcl_NewWideIntObj((Tcl_WideInt)(rdata->rowCount)));
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeleteResultSetMetadata, DeleteResultSet --
 *
 *	Cleans up when a MySQL result set is no longer required.
 *
 * Side effects:
 *	Frees all resources associated with the result set.
 *
 *-----------------------------------------------------------------------------
 */

static void
DeleteResultSetMetadata(
    ClientData clientData	/* Instance data for the connection */
) {
    DecrResultSetRefCount((ResultSetData*)clientData);
}
static void
DeleteResultSet(
    ResultSetData* rdata	/* Metadata for the result set */
) {
    StatementData* sdata = rdata->sdata;
    int i;
    int nParams;
    int nColumns;
    Tcl_ListObjLength(NULL, sdata->subVars, &nParams);
    Tcl_ListObjLength(NULL, sdata->columnNames, &nColumns);
    for (i = 0; i < nColumns; ++i) {
	MysqlBindFreeBuffer(rdata->resultBindings, i);
    }
    ckfree((char*)(rdata->resultBindings));
    ckfree((char*)(rdata->resultLengths));
    ckfree((char*)(rdata->resultNulls));
    ckfree((char*)(rdata->resultErrors));
    ckfree((char*)(rdata->paramLengths));
    if (rdata->paramBindings != NULL) {
	for (i = 0; i < nParams; ++i) {
	    if (MysqlBindGetBufferType(rdata->paramBindings, i)
		!= MYSQL_TYPE_NULL) {
		MysqlBindFreeBuffer(rdata->paramBindings, i);
	    }
	}
	ckfree((char*)(rdata->paramBindings));
    }
    if (rdata->paramValues != NULL) {
	Tcl_DecrRefCount(rdata->paramValues);
    }
    if (rdata->stmtPtr != NULL) {
	if (rdata->stmtPtr != sdata->stmtPtr) {
	    mysql_stmt_close(rdata->stmtPtr);
	} else {
	    sdata->flags &= ~ STMT_FLAG_BUSY;
	}
    }
    DecrStatementRefCount(rdata->sdata);
    ckfree((char*)rdata);
}

/*
 *-----------------------------------------------------------------------------
 *
 * CloneResultSet --
 *
 *	Attempts to clone a MySQL result set's metadata.
 *
 * Results:
 *	Returns the new metadata
 *
 * At present, we don't attempt to clone result sets - it's not obvious
 * that such an action would ever even make sense.  Instead, we throw an
 * error.
 *
 *-----------------------------------------------------------------------------
 */

static int
CloneResultSet(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    ClientData metadata,	/* Metadata to be cloned */
    ClientData* newMetaData	/* Where to put the cloned metadata */
) {
    Tcl_SetObjResult(interp,
		     Tcl_NewStringObj("MySQL result sets are not clonable",
				      -1));
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tdbcmysql_Init --
 *
 *	Initializes the TDBC-MYSQL bridge when this library is loaded.
 *
 * Side effects:
 *	Creates the ::tdbc::mysql namespace and the commands that reside in it.
 *	Initializes the MYSQL environment.
 *
 *-----------------------------------------------------------------------------
 */

extern DLLEXPORT int
Tdbcmysql_Init(
    Tcl_Interp* interp		/* Tcl interpreter */
) {
    PerInterpData* pidata;	/* Per-interpreter data for this package */
    Tcl_Obj* nameObj;		/* Name of a class or method being looked up */
    Tcl_Object curClassObject;  /* Tcl_Object representing the current class */
    Tcl_Class curClass;		/* Tcl_Class representing the current class */
    int i;

    /* Require all package dependencies */

    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    if (TclOOInitializeStubs(interp, "1.0") == NULL) {
	return TCL_ERROR;
    }
    if (Tdbc_InitStubs(interp) == NULL) {
	return TCL_ERROR;
    }

    /* Provide the current package */

    if (Tcl_PkgProvide(interp, "tdbc::mysql", PACKAGE_VERSION) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Create per-interpreter data for the package
     */

    pidata = (PerInterpData*) ckalloc(sizeof(PerInterpData));
    pidata->refCount = 1;
    for (i = 0; i < LIT__END; ++i) {
	pidata->literals[i] = Tcl_NewStringObj(LiteralValues[i], -1);
	Tcl_IncrRefCount(pidata->literals[i]);
    }
    Tcl_InitHashTable(&(pidata->typeNumHash), TCL_ONE_WORD_KEYS);
    for (i = 0; dataTypes[i].name != NULL; ++i) {
	int new;
	Tcl_HashEntry* entry =
	    Tcl_CreateHashEntry(&(pidata->typeNumHash),
				INT2PTR(dataTypes[i].num),
				&new);
	Tcl_Obj* nameObj = Tcl_NewStringObj(dataTypes[i].name, -1);
	Tcl_IncrRefCount(nameObj);
	Tcl_SetHashValue(entry, (ClientData) nameObj);
    }

    /*
     * Find the connection class, and attach an 'init' method to it.
     */

    nameObj = Tcl_NewStringObj("::tdbc::mysql::connection", -1);
    Tcl_IncrRefCount(nameObj);
    if ((curClassObject = Tcl_GetObjectFromObj(interp, nameObj)) == NULL) {
	Tcl_DecrRefCount(nameObj);
	return TCL_ERROR;
    }
    Tcl_DecrRefCount(nameObj);
    curClass = Tcl_GetObjectAsClass(curClassObject);

    /* Attach the constructor to the 'connection' class */

    Tcl_ClassSetConstructor(interp, curClass,
			    Tcl_NewMethod(interp, curClass, NULL, 1,
					  &ConnectionConstructorType,
					  (ClientData) pidata));

    /* Attach the methods to the 'connection' class */

    for (i = 0; ConnectionMethods[i] != NULL; ++i) {
	nameObj = Tcl_NewStringObj(ConnectionMethods[i]->name, -1);
	Tcl_IncrRefCount(nameObj);
	Tcl_NewMethod(interp, curClass, nameObj, 1, ConnectionMethods[i],
			   (ClientData) NULL);
	Tcl_DecrRefCount(nameObj);
    }

    /* Look up the 'statement' class */

    nameObj = Tcl_NewStringObj("::tdbc::mysql::statement", -1);
    Tcl_IncrRefCount(nameObj);
    if ((curClassObject = Tcl_GetObjectFromObj(interp, nameObj)) == NULL) {
	Tcl_DecrRefCount(nameObj);
	return TCL_ERROR;
    }
    Tcl_DecrRefCount(nameObj);
    curClass = Tcl_GetObjectAsClass(curClassObject);

    /* Attach the constructor to the 'statement' class */

    Tcl_ClassSetConstructor(interp, curClass,
			    Tcl_NewMethod(interp, curClass, NULL, 1,
					  &StatementConstructorType,
					  (ClientData) NULL));

    /* Attach the methods to the 'statement' class */

    for (i = 0; StatementMethods[i] != NULL; ++i) {
	nameObj = Tcl_NewStringObj(StatementMethods[i]->name, -1);
	Tcl_IncrRefCount(nameObj);
	Tcl_NewMethod(interp, curClass, nameObj, 1, StatementMethods[i],
			   (ClientData) NULL);
	Tcl_DecrRefCount(nameObj);
    }

    /* Look up the 'resultSet' class */

    nameObj = Tcl_NewStringObj("::tdbc::mysql::resultset", -1);
    Tcl_IncrRefCount(nameObj);
    if ((curClassObject = Tcl_GetObjectFromObj(interp, nameObj)) == NULL) {
	Tcl_DecrRefCount(nameObj);
	return TCL_ERROR;
    }
    Tcl_DecrRefCount(nameObj);
    curClass = Tcl_GetObjectAsClass(curClassObject);

    /* Attach the constructor to the 'resultSet' class */

    Tcl_ClassSetConstructor(interp, curClass,
			    Tcl_NewMethod(interp, curClass, NULL, 1,
					  &ResultSetConstructorType,
					  (ClientData) NULL));

    /* Attach the methods to the 'resultSet' class */

    for (i = 0; ResultSetMethods[i] != NULL; ++i) {
	nameObj = Tcl_NewStringObj(ResultSetMethods[i]->name, -1);
	Tcl_IncrRefCount(nameObj);
	Tcl_NewMethod(interp, curClass, nameObj, 1, ResultSetMethods[i],
			   (ClientData) NULL);
	Tcl_DecrRefCount(nameObj);
    }
    nameObj = Tcl_NewStringObj("nextlist", -1);
    Tcl_IncrRefCount(nameObj);
    Tcl_NewMethod(interp, curClass, nameObj, 1, &ResultSetNextrowMethodType,
		  (ClientData) 1);
    Tcl_DecrRefCount(nameObj);
    nameObj = Tcl_NewStringObj("nextdict", -1);
    Tcl_IncrRefCount(nameObj);
    Tcl_NewMethod(interp, curClass, nameObj, 1, &ResultSetNextrowMethodType,
		  (ClientData) 0);
    Tcl_DecrRefCount(nameObj);

    /*
     * Initialize the MySQL library if this is the first interp using it
     */

    Tcl_MutexLock(&mysqlMutex);
    if (mysqlRefCount == 0) {
	if ((mysqlLoadHandle = MysqlInitStubs(interp)) == NULL) {
	    Tcl_MutexUnlock(&mysqlMutex);
	    return TCL_ERROR;
	}
	mysql_library_init(0, NULL, NULL);
	mysqlClientVersion = mysql_get_client_version();
    }
    ++mysqlRefCount;
    Tcl_MutexUnlock(&mysqlMutex);

    /*
     * TODO: mysql_thread_init, and keep a TSD reference count of users.
     */

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeletePerInterpData --
 *
 *	Delete per-interpreter data when the MYSQL package is finalized
 *
 * Side effects:
 *	Releases the (presumably last) reference on the environment handle,
 *	cleans up the literal pool, and deletes the per-interp data structure.
 *
 *-----------------------------------------------------------------------------
 */

static void
DeletePerInterpData(
    PerInterpData* pidata	/* Data structure to clean up */
) {
    int i;

    Tcl_HashSearch search;
    Tcl_HashEntry *entry;
    for (entry = Tcl_FirstHashEntry(&(pidata->typeNumHash), &search);
	 entry != NULL;
	 entry = Tcl_NextHashEntry(&search)) {
	Tcl_Obj* nameObj = (Tcl_Obj*) Tcl_GetHashValue(entry);
	Tcl_DecrRefCount(nameObj);
    }
    Tcl_DeleteHashTable(&(pidata->typeNumHash));

    for (i = 0; i < LIT__END; ++i) {
	Tcl_DecrRefCount(pidata->literals[i]);
    }
    ckfree((char *) pidata);

    /*
     * TODO: decrease thread refcount and mysql_thread_end if need be
     */

    Tcl_MutexLock(&mysqlMutex);
    if (--mysqlRefCount == 0) {
	mysql_library_end();
	Tcl_FSUnloadFile(NULL, mysqlLoadHandle);
    }
    Tcl_MutexUnlock(&mysqlMutex);
}
