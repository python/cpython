/*
 *-----------------------------------------------------------------------------
 *
 * tdbcpostgres.c --
 *
 *	C code for the driver to interface TDBC and Postgres
 *
 * Copyright (c) 2009 by Slawomir Cygan.
 * Copyright (c) 2010 by Kevin B. Kenny.
 *
 * Please refer to the file, 'license.terms' for the conditions on
 * redistribution of this file and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_DEPRECATE
#endif

#include <tcl.h>
#include <tclOO.h>
#include <tdbc.h>

#include <stdio.h>
#include <string.h>

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#include "int2ptr_ptr2int.h"

#ifdef USE_NATIVE_POSTGRES
#  include <libpq-fe.h>
#else
#  include "fakepq.h"
#endif

/* Include the files needed to locate htons() and htonl() */

#ifdef _WIN32
typedef int int32_t;
typedef short int16_t;
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winsock2.h>
#  ifdef _MSC_VER
#    pragma comment (lib, "ws2_32")
#  endif
#else
#  include <netinet/in.h>
#endif

#ifdef _MSC_VER
#  define snprintf _snprintf
#endif

/* Static data contained within this file */

static Tcl_Mutex pgMutex;	/* Mutex protecting per-process structures */
static int pgRefCount = 0;	/* Reference count for the PG load handle */
static Tcl_LoadHandle pgLoadHandle = NULL;
				/* Load handle of the PG library */

/* Pool of literal values used to avoid excess Tcl_NewStringObj calls */

static const char *const LiteralValues[] = {
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

/* Object IDs for the Postgres data types */

#define UNTYPEDOID	0
#define BYTEAOID	17
#define INT8OID		20
#define INT2OID         21
#define INT4OID         23
#define TEXTOID		25
#define FLOAT4OID	700
#define FLOAT8OID	701
#define BPCHAROID	1042
#define VARCHAROID      1043
#define DATEOID		1082
#define TIMEOID		1083
#define TIMESTAMPOID	1114
#define BITOID		1560
#define NUMERICOID      1700

typedef struct PostgresDataType {
    const char* name;		/* Type name */
    Oid oid;			/* Type number */
} PostgresDataType;
static const PostgresDataType dataTypes[] = {
    { "NULL",	    UNTYPEDOID},
    { "smallint",   INT2OID },
    { "integer",    INT4OID },
    { "tinyint",    INT2OID },
    { "float",	    FLOAT8OID },
    { "real",	    FLOAT4OID },
    { "double",	    FLOAT8OID },
    { "timestamp",  TIMESTAMPOID },
    { "bigint",	    INT8OID },
    { "date",	    DATEOID },
    { "time",	    TIMEOID },
    { "bit",	    BITOID },
    { "numeric",    NUMERICOID },
    { "decimal",    NUMERICOID },
    { "text",	    TEXTOID },
    { "varbinary",  BYTEAOID },
    { "varchar",    VARCHAROID } ,
    { "char",	    BPCHAROID },
    { NULL, 0 }
};

/* Configuration options for Postgres connections */

/* Data types of configuration options */

enum OptType {
    TYPE_STRING,		/* Arbitrary character string */
    TYPE_PORT,			/* Port number */
    TYPE_ENCODING,		/* Encoding name */
    TYPE_ISOLATION,		/* Transaction isolation level */
    TYPE_READONLY,		/* Read-only indicator */
};

/* Locations of the string options in the string array */

enum OptStringIndex {
    INDX_HOST, INDX_HOSTA, INDX_PORT, INDX_DB, INDX_USER,
    INDX_PASS, INDX_OPT, INDX_TTY, INDX_SERV, INDX_TOUT,
    INDX_SSLM, INDX_RSSL, INDX_KERB,
    INDX_MAX
};

/* Names of string options for Postgres PGconnectdb() */

static const char *const optStringNames[] = {
    "host", "hostaddr", "port", "dbname", "user",
    "password", "options", "tty", "service", "connect_timeout",
    "sslmode", "requiressl", "krbsrvname"
};

/* Flags in the configuration table */

#define CONN_OPT_FLAG_MOD 0x1	/* Configuration value changable at runtime */
#define CONN_OPT_FLAG_ALIAS 0x2	/* Configuration option is an alias */

/*
 * Relay functions to allow Stubbed functions in the configuration options
 * table.
 */

static char* _PQdb(const PGconn* conn) { return PQdb(conn); }
static char* _PQhost(const PGconn* conn) { return PQhost(conn); }
static char* _PQoptions(const PGconn* conn) { return PQoptions(conn); }
static char* _PQpass(const PGconn* conn) { return PQpass(conn); }
static char* _PQport(const PGconn* conn) { return PQport(conn); }
static char* _PQuser(const PGconn* conn) { return PQuser(conn); }
static char* _PQtty(const PGconn* conn) { return PQtty(conn); }

/* Table of configuration options */

static const struct {
    const char * name;		    /* Option name */
    enum OptType type;		    /* Option data type */
    int info;		    	    /* Option index or flag value */
    int flags;			    /* Flags - modifiable; SSL related;
				     * is an alias */
    char *(*queryF)(const PGconn*); /* Function used to determine the
				     * option value */
} ConnOptions [] = {
    { "-host",	   TYPE_STRING,    INDX_HOST,	0,		     _PQhost},
    { "-hostaddr", TYPE_STRING,    INDX_HOSTA,	0,		     _PQhost},
    { "-port",	   TYPE_PORT,      INDX_PORT,	0,		     _PQport},
    { "-database", TYPE_STRING,    INDX_DB,	0,		     _PQdb},
    { "-db",	   TYPE_STRING,    INDX_DB,	CONN_OPT_FLAG_ALIAS, _PQdb},
    { "-user",	   TYPE_STRING,    INDX_USER,	0,		     _PQuser},
    { "-password", TYPE_STRING,    INDX_PASS,	0,		     _PQpass},
    { "-options",  TYPE_STRING,    INDX_OPT,	0,		     _PQoptions},
    { "-tty",	   TYPE_STRING,    INDX_TTY,	0,		     _PQtty},
    { "-service",  TYPE_STRING,    INDX_SERV,	0,		     NULL},
    { "-timeout",  TYPE_STRING,    INDX_TOUT,	0,		     NULL},
    { "-sslmode",  TYPE_STRING,    INDX_SSLM,	0,		     NULL},
    { "-requiressl", TYPE_STRING,  INDX_RSSL,	0,		     NULL},
    { "-krbsrvname", TYPE_STRING,  INDX_KERB,	0,		     NULL},
    { "-encoding", TYPE_ENCODING,  0,		CONN_OPT_FLAG_MOD,   NULL},
    { "-isolation", TYPE_ISOLATION, 0,		CONN_OPT_FLAG_MOD,   NULL},
    { "-readonly", TYPE_READONLY,  0,		CONN_OPT_FLAG_MOD,   NULL},
    { NULL,	   TYPE_STRING,		   0,		0,		     NULL}
};

/*
 * Structure that holds per-interpreter data for the Postgres package.
 *
 *	This structure is reference counted, because it cannot be destroyed
 *	until all connections, statements and result sets that refer to
 *	it are destroyed.
 */

typedef struct PerInterpData {
    int refCount;		    /* Reference count */
    Tcl_Obj* literals[LIT__END];    /* Literal pool */
    Tcl_HashTable typeNumHash;	    /* Lookup table for type numbers */
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
 * Structure that carries the data for a Postgres connection
 *
 * 	This structure is reference counted, to enable deferring its
 *	destruction until the last statement or result set that refers
 *	to it is destroyed.
 */

typedef struct ConnectionData {
    int refCount;		/* Reference count. */
    PerInterpData* pidata;	/* Per-interpreter data */
    PGconn* pgPtr;		/* Postgres  connection handle */
    int stmtCounter;		/* Counter for naming statements */
    int flags;
    int isolation;		/* Current isolation level */
    int readOnly;		/* Read only connection indicator */
    char * savedOpts[INDX_MAX];  /* Saved configuration options */
} ConnectionData;

/*
 * Flags for the state of an POSTGRES connection
 */

#define CONN_FLAG_IN_XCN	0x1 	/* Transaction is in progress */

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
 * Structure that carries the data for a Postgres prepared statement.
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
    Tcl_Obj* nativeSql;		/* Native SQL statement to pass into
				 * Postgres */
    char* stmtName;		/* Name identyfing the statement */
    Tcl_Obj* columnNames;	/* Column names in the result set */

    struct ParamData *params;	/* Attributes of parameters */
    int nParams;		/* Number of parameters */
    Oid* paramDataTypes;	/* Param data types list */
    int paramTypesChanged;	/* Indicator of changed param types */
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
    int precision;		/* Size of the expected data */
    int scale;			/* Digits after decimal point of the
				 * expected data */
} ParamData;

#define PARAM_KNOWN	1<<0	/* Something is known about the parameter */
#define PARAM_IN 	1<<1	/* Parameter is an input parameter */
#define PARAM_OUT 	1<<2	/* Parameter is an output parameter */
				/* (Both bits are set if parameter is
				 * an INOUT parameter) */

/*
 * Structure describing a Postgres result set.  The object that the Tcl
 * API terms a "result set" actually has to be represented by a Postgres
 * "statement", since a Postgres statement can have only one set of results
 * at any given time.
 */

typedef struct ResultSetData {
    int refCount;		/* Reference count */
    StatementData* sdata;	/* Statement that generated this result set */
    PGresult* execResult;	/* Structure containing result of prepared statement execution */
    char* stmtName;		/* Name identyfing the statement */
    int rowCount;		/* Number of already retreived rows */
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


/* Tables of isolation levels: Tcl, SQL and Postgres C API */

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

/* Static functions defined within this file */

static int DeterminePostgresMajorVersion(Tcl_Interp* interp,
					 ConnectionData* cdata,
					 int* versionPtr);
static void DummyNoticeProcessor(void*, const PGresult*);
static int ExecSimpleQuery(Tcl_Interp* interp, PGconn * pgPtr,
			   const char * query, PGresult** resOut);
static void TransferPostgresError(Tcl_Interp* interp, PGconn * pgPtr);
static int TransferResultError(Tcl_Interp* interp, PGresult * res);

static Tcl_Obj* QueryConnectionOption(ConnectionData* cdata,
				      Tcl_Interp* interp,
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
static int ConnectionRollbackMethod(ClientData clientData, Tcl_Interp* interp,
				    Tcl_ObjectContext context,
				    int objc, Tcl_Obj *const objv[]);
static int ConnectionTablesMethod(ClientData clientData, Tcl_Interp* interp,
				  Tcl_ObjectContext context,
				  int objc, Tcl_Obj *const objv[]);
static void DeleteConnectionMetadata(ClientData clientData);
static void DeleteConnection(ConnectionData* cdata);
static int CloneConnection(Tcl_Interp* interp, ClientData oldClientData,
			   ClientData* newClientData);

static char* GenStatementName(ConnectionData* cdata);
static void UnallocateStatement(PGconn* pgPtr, char* stmtName);
static StatementData* NewStatement(ConnectionData* cdata);
static PGresult* PrepareStatement(Tcl_Interp* interp,
				  StatementData* sdata, char* stmtName);
static Tcl_Obj* ResultDescToTcl(PGresult* resultDesc, int flags);
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
    "columns",			/* name */
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

const static Tcl_MethodType ConnectionRollbackMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
				/* version */
    "rollback",			/* name */
    ConnectionRollbackMethod,	/* callProc */
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
    &ConnectionRollbackMethodType,
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

/*
 *-----------------------------------------------------------------------------
 *
 * DummyNoticeReceiver --
 *
 *	Ignores warnings and notices from the PostgreSQL client library
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * This procedure does precisely nothing.
 *
 *-----------------------------------------------------------------------------
 */

static void
DummyNoticeProcessor(void* clientData,
		     const PGresult* message)
{
}

/*
 *-----------------------------------------------------------------------------
 *
 * ExecSimpleQuery --
 *
 *	Executes given query.
 *
 * Results:
 *	TCL_OK on success or the error was non fatal otherwise TCL_ERROR .
 *
 * Side effects:
 *	Sets the interpreter result and error code appropiately to
 *	query execution process. Optionally, when res parameter is
 *	not NULL and the execution is successful, it returns the
 *	PGResult * struct by this parameter. This struct should be
 *	freed with PQclear() when no longer needed.
 *
 *-----------------------------------------------------------------------------
 */

static int ExecSimpleQuery(
    Tcl_Interp* interp,		/* Tcl interpreter */
    PGconn * pgPtr,		/* Connection handle */
    const char * query,		/* Query to execute */
    PGresult** resOut		/* Optional handle to result struct */
) {
    PGresult * res; 		/* Query result */

    /* Execute the query */

    res = PQexec(pgPtr, query);

    /* Return error if the query was unsuccessful */

    if (res == NULL) {
	TransferPostgresError(interp, pgPtr);
	return TCL_ERROR;
    }
    if (TransferResultError(interp, res) != TCL_OK) {
	PQclear(res);
	return TCL_ERROR;
    }

    /* Transfer query result to the caller */

    if (resOut != NULL) {
	*resOut = res;
    } else {
	PQclear(res);
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * TransferPostgresError --
 *
 *	Obtains the connection related error message from the Postgres
 *	client library and transfers them into the Tcl interpreter.
 *	Unfortunately we cannot get error number or SQL state in
 *	connection context.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *	Sets the interpreter result and error code to describe the SQL
 *	connection error.
 *
 *-----------------------------------------------------------------------------
 */

static void
TransferPostgresError(
    Tcl_Interp* interp,		/* Tcl interpreter */
    PGconn* pgPtr		/* Postgres connection handle */
) {
    Tcl_Obj* errorCode = Tcl_NewObj();
    Tcl_ListObjAppendElement(NULL, errorCode, Tcl_NewStringObj("TDBC", -1));
    Tcl_ListObjAppendElement(NULL, errorCode,
			     Tcl_NewStringObj("GENERAL_ERROR", -1));
    Tcl_ListObjAppendElement(NULL, errorCode,
			     Tcl_NewStringObj("HY000", -1));
    Tcl_ListObjAppendElement(NULL, errorCode, Tcl_NewStringObj("POSTGRES", -1));
    Tcl_ListObjAppendElement(NULL, errorCode,
			     Tcl_NewWideIntObj(-1));
    Tcl_SetObjErrorCode(interp, errorCode);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(PQerrorMessage(pgPtr), -1));
}

/*
 *-----------------------------------------------------------------------------
 *
 * TransferPostgresError --
 *
 *	Check if there is any error related to given PGresult object.
 *	If there was an error, it obtains error message, SQL state
 *	and error number from the Postgres client library and transfers
 *	thenm into the Tcl interpreter.
 *
 * Results:
 *	TCL_OK if no error exists or the error was non fatal,
 *	otherwise TCL_ERROR is returned
 *
 * Side effects:
 *
 *	Sets the interpreter result and error code to describe the SQL
 *	connection error.
 *
 *-----------------------------------------------------------------------------
 */

static int TransferResultError(
    Tcl_Interp* interp,
    PGresult * res
) {
    ExecStatusType error = PQresultStatus(res);
    const char* sqlstate;
    if (error == PGRES_BAD_RESPONSE
	|| error == PGRES_EMPTY_QUERY
	|| error == PGRES_NONFATAL_ERROR
	|| error == PGRES_FATAL_ERROR) {
	Tcl_Obj* errorCode = Tcl_NewObj();
	Tcl_ListObjAppendElement(NULL, errorCode, Tcl_NewStringObj("TDBC", -1));

	sqlstate = PQresultErrorField(res, PG_DIAG_SQLSTATE);
	if (sqlstate == NULL) {
	    sqlstate = "HY000";
	}
	Tcl_ListObjAppendElement(NULL, errorCode,
		Tcl_NewStringObj(Tdbc_MapSqlState(sqlstate), -1));
	Tcl_ListObjAppendElement(NULL, errorCode,
		Tcl_NewStringObj(sqlstate, -1));
	Tcl_ListObjAppendElement(NULL, errorCode,
				 Tcl_NewStringObj("POSTGRES", -1));
	Tcl_ListObjAppendElement(NULL, errorCode,
		Tcl_NewWideIntObj(error));
	Tcl_SetObjErrorCode(interp, errorCode);
	if (error == PGRES_EMPTY_QUERY) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("empty query", -1));
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		PQresultErrorField(res, PG_DIAG_MESSAGE_PRIMARY), -1));
	}
    }
    if (error == PGRES_BAD_RESPONSE
	|| error == PGRES_EMPTY_QUERY
	|| error == PGRES_FATAL_ERROR) {
	return TCL_ERROR;
    } else {
	return TCL_OK;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeterminePostgresMajorVersion --
 *
 *	Determine the major version of the PostgreSQL server at the
 *	other end of a connection.
 *
 * Results:
 *	Returns a standard Tcl error code.
 *
 * Side effects:
 *	Stores the version number in '*versionPtr' if successful.
 *
 *-----------------------------------------------------------------------------
 */

static int
DeterminePostgresMajorVersion(Tcl_Interp* interp,
				/* Tcl interpreter */
			      ConnectionData* cdata,
				/* Connection data */
			      int* versionPtr)
				/* OUTPUT: PostgreSQL server version */
{
    PGresult* res;		/* Result of a Postgres query */
    int status = TCL_ERROR;	/* Status return */
    char* versionStr;		/* Version information from server */

    if (ExecSimpleQuery(interp, cdata->pgPtr,
			"SELECT version()", &res) == TCL_OK) {
	versionStr = PQgetvalue(res, 0, 0);
	if (sscanf(versionStr, " PostgreSQL %d", versionPtr) == 1) {
	    status = TCL_OK;
	} else {
	    Tcl_Obj* result = Tcl_NewStringObj("unable to parse PostgreSQL "
					       "version: \"", -1);
	    Tcl_AppendToObj(result, versionStr, -1);
	    Tcl_AppendToObj(result, "\"", -1);
	    Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY000",
			     "POSTGRES", "-1", NULL);
	}
	PQclear(res);
    }
    return status;
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
    PerInterpData* pidata = cdata->pidata; /* Per-interpreter data */
    Tcl_Obj** literals = pidata->literals;
    char * value;		/* Return value as C string */

    /* Suppress attempts to query the password */
    if (ConnOptions[optionNum].info == INDX_PASS) {
	return Tcl_NewObj();
    }
    if (ConnOptions[optionNum].type == TYPE_ENCODING) {
	value = (char* )pg_encoding_to_char(PQclientEncoding(cdata->pgPtr));
	return Tcl_NewStringObj(value, -1);
    }

    if (ConnOptions[optionNum].type == TYPE_ISOLATION) {
	if (cdata->isolation == ISOL_NONE) {
	    PGresult * res;
	    char * isoName;
	    int i = 0;

	    /* The isolation level wasn't set - get default value */

	    if (ExecSimpleQuery(interp, cdata->pgPtr,
		    "SHOW default_transaction_isolation", &res) != TCL_OK) {
		return NULL;
	    }
	    value = PQgetvalue(res, 0, 0);
	    isoName = (char*) ckalloc(strlen(value) + 1);
	    strcpy(isoName, value);
	    PQclear(res);

	    /* get rid of space */

	    while (isoName[i] != ' ' && isoName[i] != '\0') {
		i+=1;
	    }
	    if (isoName[i] == ' ') {
		while (isoName[i] != '\0') {
		    isoName[i] = isoName[i+1];
		    i+=1;
		}
	    }

	    /* Search for isolation level name in predefined table */

	    i=0;
	    while (TclIsolationLevels[i] != NULL
		   && strcmp(isoName, TclIsolationLevels[i])) {
		i += 1;
	    }
	    ckfree(isoName);
	    if (TclIsolationLevels[i] != NULL) {
		cdata->isolation = i;
	    } else {
		return NULL;
	    }
	}
	return Tcl_NewStringObj(
		TclIsolationLevels[cdata->isolation], -1);
    }

    if (ConnOptions[optionNum].type == TYPE_READONLY) {
	if (cdata->readOnly == 0) {
	    return literals[LIT_0];
	} else {
	    return literals[LIT_1];
	}
    }

    if (ConnOptions[optionNum].queryF != NULL) {
	value = ConnOptions[optionNum].queryF(cdata->pgPtr);
	if (value != NULL) {
	    return Tcl_NewStringObj(value, -1);
	}
    }
    if (ConnOptions[optionNum].type == TYPE_STRING &&
	    ConnOptions[optionNum].info != -1) {
	/* Fallback: get value saved ealier */
	value = cdata->savedOpts[ConnOptions[optionNum].info];
	if (value != NULL) {
	    return Tcl_NewStringObj(value, -1);
	}
    }
    return literals[LIT_EMPTY];
}

/*
 *-----------------------------------------------------------------------------
 *
 * ConfigureConnection --
 *
 *	Applies configuration settings to a Postrgre connection.
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
    int optionIndex;		/* Index of the current option in
				 * ConnOptions */
    int optionValue;		/* Integer value of the current option */
    int i;
    size_t j;
    char portval[10];		/* String representation of port number */
    char * encoding = NULL;	/* Selected encoding name */
    int isolation = ISOL_NONE;	/* Isolation level */
    int readOnly = -1;		/* Read only indicator */
#define CONNINFO_LEN 1000
    char connInfo[CONNINFO_LEN]; /* Configuration string for PQconnectdb() */

    Tcl_Obj* retval;
    Tcl_Obj* optval;
    int vers;			/* PostgreSQL major version */

    if (cdata->pgPtr != NULL) {

	/* Query configuration options on an existing connection */

	if (objc == skip) {
	    /* Return all options as a dict */
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

	    /* Return one option value */

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

    /* In all cases number of parameters must be even */
    if ((objc-skip) % 2 != 0) {
	Tcl_WrongNumArgs(interp, skip, objv, "?-option value?...");
	return TCL_ERROR;
    }

    /* Extract options from the command line */

    for (i = 0; i < INDX_MAX; ++i) {
	cdata->savedOpts[i] = NULL;
    }
    for (i = skip; i < objc; i += 2) {

	/* Unknown option */

	if (Tcl_GetIndexFromObjStruct(interp, objv[i], (void*) ConnOptions,
				      sizeof(ConnOptions[0]), "option",
				      0, &optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}

	/* Unmodifiable option */

	if (cdata->pgPtr != NULL && !(ConnOptions[optionIndex].flags
					 & CONN_OPT_FLAG_MOD)) {
	    Tcl_Obj* msg = Tcl_NewStringObj("\"", -1);
	    Tcl_AppendObjToObj(msg, objv[i]);
	    Tcl_AppendToObj(msg, "\" option cannot be changed dynamically",
			    -1);
	    Tcl_SetObjResult(interp, msg);
	    Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY000",
			     "POSTGRES", "-1", NULL);
	    return TCL_ERROR;
	}

	/* Record option value */

	switch (ConnOptions[optionIndex].type) {
	case TYPE_STRING:
	    cdata->savedOpts[ConnOptions[optionIndex].info] =
		Tcl_GetString(objv[i+1]);
	    break;
	case TYPE_ENCODING:
	    encoding = Tcl_GetString(objv[i+1]);
	    break;
	case TYPE_ISOLATION:
	    if (Tcl_GetIndexFromObjStruct(interp, objv[i+1], TclIsolationLevels,
				    sizeof(char *), "isolation level", TCL_EXACT, &isolation)
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
				 "POSTGRES", "-1", NULL);
		return TCL_ERROR;
	    }
	    sprintf(portval, "%d", optionValue);
	    cdata->savedOpts[INDX_PORT] = portval;
	    break;
	case TYPE_READONLY:
	    if (Tcl_GetBooleanFromObj(interp, objv[i+1], &readOnly)
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	}
    }

    if (cdata->pgPtr == NULL) {
	j=0;
	connInfo[0] = '\0';
	for (i=0; i<INDX_MAX; i+=1) {
	    if (cdata->savedOpts[i] != NULL ) {
		/* TODO escape values */
		strncpy(&connInfo[j], optStringNames[i], CONNINFO_LEN - j);
		j+=strlen(optStringNames[i]);
		strncpy(&connInfo[j], " = '", CONNINFO_LEN - j);
		j+=strlen(" = '");
		strncpy(&connInfo[j], cdata->savedOpts[i], CONNINFO_LEN - j);
		j+=strlen(cdata->savedOpts[i]);
		strncpy(&connInfo[j], "' ", CONNINFO_LEN - j);
		j+=strlen("' ");
	    }
	}
	cdata->pgPtr = PQconnectdb(connInfo);
	if (cdata->pgPtr == NULL) {
	    Tcl_SetObjResult(interp,
			     Tcl_NewStringObj("PQconnectdb() failed, "
					      "propably out of memory.", -1));
	    Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HY001",
			     "POSTGRES", "NULL", NULL);
	    return TCL_ERROR;
	}

	if (PQstatus(cdata->pgPtr) != CONNECTION_OK) {
	    TransferPostgresError(interp, cdata->pgPtr);
	    return TCL_ERROR;
	}
	PQsetNoticeProcessor(cdata->pgPtr, DummyNoticeProcessor, NULL);
    }

    /* Character encoding */

    if (encoding != NULL ) {
	if (PQsetClientEncoding(cdata->pgPtr, encoding) != 0) {
	    TransferPostgresError(interp, cdata->pgPtr);
	    return TCL_ERROR;
	}
    }

    /* Transaction isolation level */

    if (isolation != ISOL_NONE) {
	if (ExecSimpleQuery(interp, cdata->pgPtr,
		    SqlIsolationLevels[isolation], NULL) != TCL_OK) {
	    return TCL_ERROR;
	}
	cdata->isolation = isolation;
    }

    /* Readonly indicator */

    if (readOnly != -1) {
	if (readOnly == 0) {
	    if (ExecSimpleQuery(interp, cdata->pgPtr,
			"SET TRANSACTION READ WRITE", NULL) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    if (ExecSimpleQuery(interp, cdata->pgPtr,
			"SET TRANSACTION READ ONLY", NULL) != TCL_OK) {
		return TCL_ERROR;
	    }

	}
	cdata->readOnly = readOnly;
    }

    /* Determine the PostgreSQL version in use */

    if (DeterminePostgresMajorVersion(interp, cdata, &vers) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * On PostgreSQL 9.0 and later, change 'bytea_output' to the
     * backward-compatible 'escape' setting, so that the code in
     * ResultSetNextrowMethod will retrieve byte array values correctly
     * on either 8.x or 9.x servers.
     */
    if (vers >= 9) {
	if (ExecSimpleQuery(interp, cdata->pgPtr,
			    "SET bytea_output = 'escape'", NULL) != TCL_OK) {
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
 *	Constructor for ::tdbc::postgres::connection, which represents a
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
				/* Per-interp data for the POSTGRES package */
    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current object */
    int skip = Tcl_ObjectContextSkippedArgs(context);
				/* The number of leading arguments to skip */
    ConnectionData* cdata;	/* Per-connection data */

    /* Hang client data on this connection */

    cdata = (ConnectionData*) ckalloc(sizeof(ConnectionData));
    memset(cdata, 0, sizeof(ConnectionData));
    cdata->refCount = 1;
    cdata->pidata = pidata;
    cdata->pgPtr = NULL;
    cdata->stmtCounter = 0;
    cdata->flags = 0;
    cdata->isolation = ISOL_NONE;
    cdata->readOnly = 0;
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
 *	Method that requests that following operations on an POSTGRES
 *	connection be executed as an atomic transaction.
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
	Tcl_SetObjResult(interp, Tcl_NewStringObj("Postgres does not support "
						  "nested transactions", -1));
	Tcl_SetErrorCode(interp, "TDBC", "GENERAL_ERROR", "HYC00",
			 "POSTGRES", "-1", NULL);
	return TCL_ERROR;
    }
   cdata->flags |= CONN_FLAG_IN_XCN;

   /* Execute begin trasnaction block command */

   return ExecSimpleQuery(interp, cdata->pgPtr, "BEGIN", NULL);
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
			 "POSTGRES", "-1", NULL);
	return TCL_ERROR;
    }

    cdata->flags &= ~ CONN_FLAG_IN_XCN;

    /* Execute commit SQL command */
    return ExecSimpleQuery(interp, cdata->pgPtr, "COMMIT", NULL);
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
    PGresult* res,* resType;	/* Results of libpq call */
    char* columnName;		/* Name of the column */
    Oid typeOid;		/* Oid of column type */
    Tcl_Obj* retval;		/* List of table names */
    Tcl_Obj* attrs;		/* Attributes of the column */
    Tcl_Obj* name;		/* Name of a column */
    Tcl_Obj* sqlQuery = Tcl_NewStringObj("SELECT * FROM ", -1);
				/* Query used */

    Tcl_IncrRefCount(sqlQuery);

    /* Check parameters */

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "table ?pattern?");
	return TCL_ERROR;
    }

    /* Check if table exists by retreiving one row.
     * The result wille be later used to determine column types (oids) */
    Tcl_AppendObjToObj(sqlQuery, objv[2]);

    if (ExecSimpleQuery(interp, cdata->pgPtr, Tcl_GetString(sqlQuery),
			&resType) != TCL_OK) {
        Tcl_DecrRefCount(sqlQuery);
	return TCL_ERROR;
    }

    Tcl_DecrRefCount(sqlQuery);

    /* Retreive column attributes */

    sqlQuery = Tcl_NewStringObj("SELECT "
	"  column_name,"
	"  numeric_precision,"
	"  character_maximum_length,"
	"  numeric_scale,"
	"  is_nullable"
	"  FROM information_schema.columns"
	"  WHERE table_name='", -1);
    Tcl_IncrRefCount(sqlQuery);
    Tcl_AppendObjToObj(sqlQuery, objv[2]);

    if (objc == 4) {
	Tcl_AppendToObj(sqlQuery,"' AND column_name LIKE '", -1);
	Tcl_AppendObjToObj(sqlQuery, objv[3]);
    }
    Tcl_AppendToObj(sqlQuery,"'", -1);

    if (ExecSimpleQuery(interp, cdata->pgPtr,
			Tcl_GetString(sqlQuery), &res) != TCL_OK) {
        Tcl_DecrRefCount(sqlQuery);
	PQclear(resType);
	return TCL_ERROR;
    } else {
	int i, j;
	retval = Tcl_NewObj();
	Tcl_IncrRefCount(retval);
	for (i = 0; i < PQntuples(res); i += 1) {
	    attrs = Tcl_NewObj();

	    /* 0 is column_name column number */
	    columnName = PQgetvalue(res, i, 0);
	    name = Tcl_NewStringObj(columnName, -1);

	    Tcl_DictObjPut(NULL, attrs, literals[LIT_NAME], name);

	    /* Get the type name, by retrieving type oid */

	    j = PQfnumber(resType, columnName);
	    if (j >= 0) {
		typeOid = PQftype(resType, j);

		/* TODO: bsearch or sthing */
		j = 0 ;
		while (dataTypes[j].name != NULL
		       && dataTypes[j].oid != typeOid) {
		    j+=1;
		}
		if ( dataTypes[j].name != NULL) {
		    Tcl_DictObjPut(NULL, attrs, literals[LIT_TYPE],
				   Tcl_NewStringObj(dataTypes[j].name, -1));
		}
	    }

	    /* 1 is numeric_precision column number */

	    if (!PQgetisnull(res, i, 1)) {
		Tcl_DictObjPut(NULL, attrs, literals[LIT_PRECISION],
			Tcl_NewStringObj(PQgetvalue(res, i, 1), -1));
	    } else {

		/* 2 is character_maximum_length column number */

		if (!PQgetisnull(res, i, 2)) {
		    Tcl_DictObjPut(NULL, attrs, literals[LIT_PRECISION],
			    Tcl_NewStringObj(PQgetvalue(res, i, 2), -1));
		}
	    }

	    /* 3 is character_maximum_length column number */

	    if (!PQgetisnull(res, i, 3)) {

		/* This is for numbers */

		Tcl_DictObjPut(NULL, attrs, literals[LIT_SCALE],
			Tcl_NewStringObj(PQgetvalue(res, i, 3), -1));
	    }

	    /* 4 is is_nullable column number */

	    Tcl_DictObjPut(NULL, attrs, literals[LIT_NULLABLE],
		    Tcl_NewWideIntObj(strcmp("YES",
			    PQgetvalue(res, i, 4)) == 0));
	    Tcl_DictObjPut(NULL, retval, name, attrs);
	}

	Tcl_DecrRefCount(sqlQuery);
	Tcl_SetObjResult(interp, retval);
	Tcl_DecrRefCount(retval);
	PQclear(resType);
	PQclear(res);
	return TCL_OK;
    }
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
			 "POSTGRES", "-1", NULL);
	return TCL_ERROR;
    }

    cdata->flags &= ~CONN_FLAG_IN_XCN;

    /* Send end transaction SQL command */
    return ExecSimpleQuery(interp, cdata->pgPtr, "ROLLBACK", NULL);
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
    PGresult* res;		/* Result of libpq call */
    char * field;		/* Field value from SQL result */
    Tcl_Obj* retval;		/* List of table names */
    Tcl_Obj* sqlQuery = Tcl_NewStringObj("SELECT tablename"
					 " FROM pg_tables"
					 " WHERE  schemaname = 'public'",
					 -1);
				/* SQL query for table list */
    int i;
    Tcl_IncrRefCount(sqlQuery);

    /* Check parameters */

    if (objc < 2 || objc > 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    if (objc == 3) {

	/* Pattern string is given */

	Tcl_AppendToObj(sqlQuery, " AND  tablename LIKE '", -1);
	Tcl_AppendObjToObj(sqlQuery, objv[2]);
	Tcl_AppendToObj(sqlQuery, "'", -1);
    }

    /* Retrieve the table list */

    if (ExecSimpleQuery(interp, cdata ->pgPtr, Tcl_GetString(sqlQuery),
			&res) != TCL_OK) {
	Tcl_DecrRefCount(sqlQuery);
	return TCL_ERROR;
    }
    Tcl_DecrRefCount(sqlQuery);

    /* Iterate through the tuples and make the Tcl result */

    retval = Tcl_NewObj();
    for (i = 0; i < PQntuples(res); i+=1) {
	if (!PQgetisnull(res, i, 0)) {
	    field = PQgetvalue(res, i, 0);
	    if (field) {
		Tcl_ListObjAppendElement(NULL, retval,
			Tcl_NewStringObj(field, -1));
		Tcl_ListObjAppendElement(NULL, retval, literals[LIT_EMPTY]);
	    }
	}
    }
    PQclear(res);

    Tcl_SetObjResult(interp, retval);
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
    if (cdata->pgPtr != NULL) {
	PQfinish(cdata->pgPtr);
    }
    DecrPerInterpRefCount(cdata->pidata);
    ckfree(cdata);
}

/*
 *-----------------------------------------------------------------------------
 *
 * CloneConnection --
 *
 *	Attempts to clone an Postgres connection's metadata.
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
		     Tcl_NewStringObj("Postgres connections are not clonable",
				      -1));
    return TCL_ERROR;
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
 *	down POSTGRES when it is no longer required.
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
 *	Callback executed when any of the POSTGRES client methods is cloned.
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
 * GenStatementName --
 *
 *	Generates a unique name for a Postgre  statement
 *
 * Results:
 *	Null terminated, free-able,  string containg the name.
 *
 *-----------------------------------------------------------------------------
 */

static char*
GenStatementName(
    ConnectionData* cdata	/* Instance data for the connection */
) {
    char stmtName[30];
    char* retval;
    cdata->stmtCounter += 1;
    snprintf(stmtName, 30, "statement%d", cdata->stmtCounter);
    retval = (char *)ckalloc(strlen(stmtName) + 1);
    strcpy(retval, stmtName);
    return retval;
}

/*
 *-----------------------------------------------------------------------------
 *
 * UnallocateStatement --
 *
 *	Tries tu unallocate prepared statement using SQL query. No
 *	errors are reported on failure.
 *
 * Results:
 *	Nothing.
 *
 *-----------------------------------------------------------------------------
 */

static void
UnallocateStatement(
	PGconn * pgPtr,	    /* Connection handle */
	char* stmtName	    /* Statement name */
) {
    Tcl_Obj * sqlQuery = Tcl_NewStringObj("DEALLOCATE ", -1);
    Tcl_IncrRefCount(sqlQuery);
    Tcl_AppendToObj(sqlQuery, stmtName, -1);
    PQclear(PQexec(pgPtr, Tcl_GetString(sqlQuery)));
    Tcl_DecrRefCount(sqlQuery);
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
    memset(sdata, 0, sizeof(StatementData));
    sdata->refCount = 1;
    sdata->cdata = cdata;
    IncrConnectionRefCount(cdata);
    sdata->subVars = Tcl_NewObj();
    Tcl_IncrRefCount(sdata->subVars);
    sdata->params = NULL;
    sdata->paramDataTypes = NULL;
    sdata->nativeSql = NULL;
    sdata->columnNames = NULL;
    sdata->flags = 0;
    sdata->stmtName = GenStatementName(cdata);
    sdata->paramTypesChanged = 0;

    return sdata;
}

/*
 *-----------------------------------------------------------------------------
 *
 * PrepareStatement --
 *
 *	Prepare a PostgreSQL statement. When stmtName equals to
 *	NULL, statement name is taken from sdata strucure.
 *
 * Results:
 *	Returns the Posgres result object if successful, and NULL on failure.
 *
 * Side effects:
 *	Prepares the statement.
 *	Stores error message and error code in the interpreter on failure.
 *
 *-----------------------------------------------------------------------------
 */

static PGresult*
PrepareStatement(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    StatementData* sdata,	/* Statement data */
    char * stmtName		/* Overriding name of the statement */
) {
    ConnectionData* cdata = sdata->cdata;
				/* Connection data */
    const char* nativeSqlStr;	/* Native SQL statement to prepare */
    int nativeSqlLen;		/* Length of the statement */
    PGresult* res;		/* result of statement preparing*/
    PGresult* res2;
    int i;

    if (stmtName == NULL) {
	stmtName = sdata->stmtName;
    }

    /*
     * Prepare the statement. Rather than giving parameter types, try
     * to let PostgreSQL infer all of them.
     */

    nativeSqlStr = Tcl_GetStringFromObj(sdata->nativeSql, &nativeSqlLen);
    res = PQprepare(cdata->pgPtr, stmtName, nativeSqlStr, 0, NULL);
    if (res == NULL) {
        TransferPostgresError(interp, cdata->pgPtr);
	return NULL;
    }

    /*
     * Report on what parameter types were inferred.
     */

    res2 = PQdescribePrepared(cdata->pgPtr, stmtName);
    if (res2 == NULL) {
	TransferPostgresError(interp, cdata->pgPtr);
	PQclear(res);
	return NULL;
    }
    for (i = 0; i < PQnparams(res2); ++i) {
	sdata->paramDataTypes[i] = PQparamtype(res2, i);
	sdata->params[i].precision = 0;
	sdata->params[i].scale = 0;
    }
    PQclear(res2);

    return res;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ResultDescToTcl --
 *
 *	Converts a Postgres result description for return as a Tcl list.
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
    PGresult* result,		/* Result set description */
    int flags			/* Flags governing the conversion */
) {
    Tcl_Obj* retval = Tcl_NewObj();
    Tcl_HashTable names;	/* Hash table to resolve name collisions */
    char * fieldName;
    Tcl_InitHashTable(&names, TCL_STRING_KEYS);
    if (result != NULL) {
	unsigned int fieldCount = PQnfields(result);
	unsigned int i;
	char numbuf[16];
	for (i = 0; i < fieldCount; ++i) {
	    int isNew;
	    int count = 1;
	    Tcl_Obj* nameObj;
	    Tcl_HashEntry* entry;
	    fieldName = PQfname(result, i);
	    nameObj = Tcl_NewStringObj(fieldName, -1);
	    Tcl_IncrRefCount(nameObj);
	    entry =
		Tcl_CreateHashEntry(&names, fieldName, &isNew);
	    while (!isNew) {
	        count = PTR2INT(Tcl_GetHashValue(entry));
		++count;
		Tcl_SetHashValue(entry, INT2PTR(count));
		sprintf(numbuf, "#%d", count);
		Tcl_AppendToObj(nameObj, numbuf, -1);
		entry = Tcl_CreateHashEntry(&names, Tcl_GetString(nameObj),
					    &isNew);
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
 *	C-level initialization for the object representing an Postgres prepared
 *	statement.
 *
 * Usage:
 *	statement new connection statementText
 *	statement create name connection statementText
 *
 * Parameters:
 *      connection -- the Postgres connection object
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
    PGresult* res;		/* Temporary result of libpq calls */
    char tmpstr[30];		/* Temporary array for strings */
    int i,j;


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
			 " does not refer to a Postgres connection", NULL);
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
     * Rewrite the tokenized statement to Postgres syntax. Reject the
     * statement if it is actually multiple statements.
     */

    if (Tcl_ListObjGetElements(interp, tokens, &tokenc, &tokenv) != TCL_OK) {
	goto freeTokens;
    }
    nativeSql = Tcl_NewObj();
    Tcl_IncrRefCount(nativeSql);
    j=0;

    for (i = 0; i < tokenc; ++i) {
	tokenStr = Tcl_GetStringFromObj(tokenv[i], &tokenLen);

	switch (tokenStr[0]) {
	case '$':
	case ':':
	    /*
	     * A PostgreSQL cast is not a parameter!
	     */
	    if (tokenStr[0] == ':' && tokenStr[1] == tokenStr[0]) {
		Tcl_AppendToObj(nativeSql, tokenStr, tokenLen);
		break;
	    }

	    j+=1;
	    snprintf(tmpstr, 30, "$%d", j);
	    Tcl_AppendToObj(nativeSql, tmpstr, -1);
	    Tcl_ListObjAppendElement(NULL, sdata->subVars,
				     Tcl_NewStringObj(tokenStr+1, tokenLen-1));
	    break;

	case ';':
	    Tcl_SetObjResult(interp,
			     Tcl_NewStringObj("tdbc::postgres"
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

    Tcl_ListObjLength(NULL, sdata->subVars, &sdata->nParams);
    sdata->params = (ParamData*) ckalloc(sdata->nParams * sizeof(ParamData));
    memset(sdata->params, 0, sdata->nParams * sizeof(ParamData));
    sdata->paramDataTypes = (Oid*) ckalloc(sdata->nParams * sizeof(Oid));
    memset(sdata->paramDataTypes, 0, sdata->nParams * sizeof(Oid));
    for (i = 0; i < sdata->nParams; ++i) {
	sdata->params[i].flags = PARAM_IN;
	sdata->paramDataTypes[i] = UNTYPEDOID ;
	sdata->params[i].precision = 0;
	sdata->params[i].scale = 0;
    }

    /* Prepare the statement */

    res = PrepareStatement(interp, sdata, NULL);
    if (res == NULL) {
	goto freeSData;
    }
    if (TransferResultError(interp, res) != TCL_OK) {
	PQclear(res);
	goto freeSData;
    }

    PQclear(res);

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
 *	Lists the parameters in a Postgres statement.
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
    for (i = 0; i < sdata->nParams; ++i) {
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
			      INT2PTR(sdata->paramDataTypes[i]));
	if (typeHashEntry != NULL) {
	    dataTypeName = (Tcl_Obj*) Tcl_GetHashValue(typeHashEntry);
	    Tcl_DictObjPut(NULL, paramDesc, literals[LIT_TYPE], dataTypeName);
	}
	Tcl_DictObjPut(NULL, paramDesc, literals[LIT_PRECISION],
		       Tcl_NewWideIntObj(sdata->params[i].precision));
	Tcl_DictObjPut(NULL, paramDesc, literals[LIT_SCALE],
		       Tcl_NewWideIntObj(sdata->params[i].scale));
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
 *	Defines a parameter type in a Postgres statement.
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

    paramName = Tcl_GetString(objv[2]);
    for (i = 0; i < sdata->nParams; ++i) {
	Tcl_ListObjIndex(NULL, sdata->subVars, i, &targetNameObj);
	targetName = Tcl_GetString(targetNameObj);
	if (!strcmp(paramName, targetName)) {
	    ++matchCount;
	    sdata->params[i].flags = direction;
	    if (sdata->paramDataTypes[i] != dataTypes[typeNum].oid) {
		sdata->paramTypesChanged = 1;
	    }
	    sdata->paramDataTypes[i] = dataTypes[typeNum].oid;
	    sdata->params[i].precision = precision;
	    sdata->params[i].scale = scale;
	}
    }
    if (matchCount == 0) {
	errorObj = Tcl_NewStringObj("unknown parameter \"", -1);
	Tcl_AppendToObj(errorObj, paramName, -1);
	Tcl_AppendToObj(errorObj, "\": must be ", -1);
	for (i = 0; i < sdata->nParams; ++i) {
	    Tcl_ListObjIndex(NULL, sdata->subVars, i, &targetNameObj);
	    Tcl_AppendObjToObj(errorObj, targetNameObj);
	    if (i < sdata->nParams-2) {
		Tcl_AppendToObj(errorObj, ", ", -1);
	    } else if (i == sdata->nParams-2) {
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
 *	Cleans up when a Postgres statement is no longer required.
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
    if (sdata->stmtName != NULL) {
	UnallocateStatement(sdata->cdata->pgPtr, sdata->stmtName);
	ckfree(sdata->stmtName);
    }
    if (sdata->nativeSql != NULL) {
	Tcl_DecrRefCount(sdata->nativeSql);
    }
    if (sdata->params != NULL) {
	ckfree(sdata->params);
    }
    if (sdata->paramDataTypes != NULL) {
	ckfree(sdata->paramDataTypes);
    }
    Tcl_DecrRefCount(sdata->subVars);
    DecrConnectionRefCount(sdata->cdata);
    ckfree(sdata);
}

/*
 *-----------------------------------------------------------------------------
 *
 * CloneStatement --
 *
 *	Attempts to clone a Postgres statement's metadata.
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
		     Tcl_NewStringObj("Postgres statements are not clonable",
				      -1));
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
    ConnectionData* cdata;	/* The Postgres connection object's data */

    StatementData* sdata;	/* The statement object's data */
    ResultSetData* rdata;	/* THe result set object's data */
    Tcl_Obj* paramNameObj;	/* Name of the current parameter */
    const char* paramName;	/* Name of the current parameter */
    Tcl_Obj* paramValObj;	/* Value of the current parameter */

    const char** paramValues;	/* Table of values */
    int* paramLengths;		/* Table of parameter lengths */
    int* paramFormats;		/* Table of parameter formats
				 * (binary or string) */
    char* paramNeedsFreeing;	/* Flags for whether a parameter needs
				 * its memory released */
    Tcl_Obj** paramTempObjs;	/* Temporary parameter objects allocated
				 * to canonicalize numeric parameter values */

    PGresult* res;		/* Temporary result */
    int i;
    int status = TCL_ERROR;	/* Return status */

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
			 " does not refer to a Postgres statement", NULL);
	return TCL_ERROR;
    }
    cdata = sdata->cdata;

    rdata = (ResultSetData*) ckalloc(sizeof(ResultSetData));
    memset(rdata, 0, sizeof(ResultSetData));
    rdata->refCount = 1;
    rdata->sdata = sdata;
    rdata->stmtName = NULL;
    rdata->execResult = NULL;
    rdata->rowCount = 0;
    IncrStatementRefCount(sdata);
    Tcl_ObjectSetMetadata(thisObject, &resultSetDataType, (ClientData) rdata);

    /*
     * Find a statement handle that we can use to execute the SQL code.
     * If the main statement handle associated with the statement
     * is idle, we can use it.  Otherwise, we have to allocate and
     * prepare a fresh one.
     */

    if (sdata->flags & STMT_FLAG_BUSY) {
	rdata->stmtName = GenStatementName(cdata);
	res = PrepareStatement(interp, sdata, rdata->stmtName);
	if (res == NULL) {
	    return TCL_ERROR;
	}
	if (TransferResultError(interp, res) != TCL_OK) {
	    PQclear(res);
	    return TCL_ERROR;
	}
	PQclear(res);
    } else {
	rdata->stmtName = sdata->stmtName;
	sdata->flags |= STMT_FLAG_BUSY;

	/* We need to check if parameter types changed since the
	 * statement was prepared. If so, the statement is no longer
	 * usable, so we prepare it once again */

	if (sdata->paramTypesChanged) {
	    UnallocateStatement(cdata->pgPtr, sdata->stmtName);
	    ckfree(sdata->stmtName);
	    sdata->stmtName = GenStatementName(cdata);
	    rdata->stmtName = sdata->stmtName;
	    res = PrepareStatement(interp, sdata, NULL);
	    if (res == NULL) {
		return TCL_ERROR;
	    }
	    if (TransferResultError(interp, res) != TCL_OK) {
		PQclear(res);
		return TCL_ERROR;
	    }
	    PQclear(res);
	    sdata->paramTypesChanged = 0;
	}
    }

    paramValues = (const char**) ckalloc(sdata->nParams * sizeof(char* ));
    paramLengths = (int*) ckalloc(sdata->nParams * sizeof(int*));
    paramFormats = (int*) ckalloc(sdata->nParams * sizeof(int*));
    paramNeedsFreeing = (char *)ckalloc(sdata->nParams);
    paramTempObjs = (Tcl_Obj**) ckalloc(sdata->nParams * sizeof(Tcl_Obj*));

    memset(paramNeedsFreeing, 0, sdata->nParams);
    for (i = 0; i < sdata->nParams; i++) {
	paramTempObjs[i] = NULL;
    }

    for (i=0; i<sdata->nParams; i++) {
	Tcl_ListObjIndex(NULL, sdata->subVars, i, &paramNameObj);
	paramName = Tcl_GetString(paramNameObj);
	if (objc == skip+2) {
	    /* Param from a dictionary */

	    if (Tcl_DictObjGet(interp, objv[skip+1],
			       paramNameObj, &paramValObj) != TCL_OK) {
		goto freeParamTables;
	    }
	} else {
	    /* Param from a variable */

	    paramValObj = Tcl_GetVar2Ex(interp, paramName, NULL,
					TCL_LEAVE_ERR_MSG);

	}
	/* At this point, paramValObj contains the parameter value */
	if (paramValObj != NULL) {
	    char * bufPtr;
	    int32_t tmp32;
	    int16_t tmp16;

	    switch (sdata->paramDataTypes[i]) {
	    case INT2OID:
		bufPtr = (char *)ckalloc(sizeof(int));
		if (Tcl_GetIntFromObj(interp, paramValObj,
				      (int*) bufPtr) != TCL_OK) {
		    goto freeParamTables;
		}
		paramValues[i] = (char *)ckalloc(sizeof(int16_t));
		paramNeedsFreeing[i] = 1;
		tmp16 = *(int*) bufPtr;
		ckfree(bufPtr);
		*(int16_t*)(paramValues[i])=htons(tmp16);
		paramFormats[i] = 1;
		paramLengths[i] = sizeof(int16_t);
		break;

	    case INT4OID:
		bufPtr = (char *)ckalloc(sizeof(long));
		if (Tcl_GetLongFromObj(interp, paramValObj,
				       (long*) bufPtr) != TCL_OK) {
		    goto freeParamTables;
		}
		paramValues[i] = (char *)ckalloc(sizeof(int32_t));
		paramNeedsFreeing[i] = 1;
		tmp32 = *(long*) bufPtr;
		ckfree(bufPtr);
		*((int32_t*)(paramValues[i]))=htonl(tmp32);
		paramFormats[i] = 1;
		paramLengths[i] = sizeof(int32_t);
		break;

		/*
		 * With INT8, FLOAT4, FLOAT8, and NUMERIC, we will be passing
		 * the parameter as text, but it may not be in a canonical
		 * format, because Tcl will recognize binary, octal, and hex
		 * constants where Postgres will not. Begin by extracting
		 * wide int, float, or bignum from the parameter. If that
		 * succeeds, reconvert the result to text to canonicalize
		 * it, and send that text over.
		 */

	    case INT8OID:
	    case NUMERICOID:
		{
		    Tcl_WideInt val;
		    if (Tcl_GetWideIntFromObj(NULL, paramValObj, &val)
			== TCL_OK) {
			paramTempObjs[i] = Tcl_NewWideIntObj(val);
			Tcl_IncrRefCount(paramTempObjs[i]);
			paramFormats[i] = 0;
			paramValues[i] =
			    Tcl_GetStringFromObj(paramTempObjs[i],
						 &paramLengths[i]);
		    } else {
			goto convertString;
				/* If Tcl can't parse it, let SQL try */
		    }
		}
		break;

	    case FLOAT4OID:
	    case FLOAT8OID:
		{
		    double val;
		    if (Tcl_GetDoubleFromObj(NULL, paramValObj, &val)
			== TCL_OK) {
			paramTempObjs[i] = Tcl_NewDoubleObj(val);
			Tcl_IncrRefCount(paramTempObjs[i]);
			paramFormats[i] = 0;
			paramValues[i] =
			    Tcl_GetStringFromObj(paramTempObjs[i],
						 &paramLengths[i]);
		    } else {
			goto convertString;
				/* If Tcl can't parse it, let SQL try */
		    }
		}
		break;

	    case BYTEAOID:
		paramFormats[i] = 1;
		paramValues[i] =
		    (char*)Tcl_GetByteArrayFromObj(paramValObj,
						   &paramLengths[i]);
		break;

	    default:
	    convertString:
		paramFormats[i] = 0;
		paramValues[i] = Tcl_GetStringFromObj(paramValObj,
						      &paramLengths[i]);
		break;
	    }
	} else {
	    paramValues[i] = NULL;
	    paramFormats[i] = 0;
	}
    }

    /* Execute the statement */
    rdata->execResult = PQexecPrepared(cdata->pgPtr, rdata->stmtName,
				       sdata->nParams, paramValues,
				       paramLengths, paramFormats, 0);
    if (TransferResultError(interp, rdata->execResult) != TCL_OK) {
	goto freeParamTables;
    }

    sdata->columnNames = ResultDescToTcl(rdata->execResult, 0);
    Tcl_IncrRefCount(sdata->columnNames);
    status = TCL_OK;

    /* Clean up allocated memory */

 freeParamTables:
    for (i = 0; i < sdata->nParams; ++i) {
	if (paramNeedsFreeing[i]) {
	    ckfree(paramValues[i]);
	}
	if (paramTempObjs[i] != NULL) {
	    Tcl_DecrRefCount(paramTempObjs[i]);
	}
    }

    ckfree(paramValues);
    ckfree(paramLengths);
    ckfree(paramFormats);
    ckfree(paramNeedsFreeing);
    ckfree(paramTempObjs);

    return status;

}

/*
 *-----------------------------------------------------------------------------
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

    Tcl_SetObjResult(interp, (sdata->columnNames));

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

    int nColumns = 0;		/* Number of columns in the result set */
    Tcl_Obj* colObj;		/* Column obtained from the row */
    Tcl_Obj* colName;		/* Name of the current column */
    Tcl_Obj* resultRow;		/* Row of the result set under construction */

    int status = TCL_ERROR;	/* Status return from this command */

    char * buffer;		/* buffer containing field value */
    int buffSize;		/* size of buffer containing field value */
    int i;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "varName");
	return TCL_ERROR;
    }

    /* Check if row counter haven't already rech the last row */
    if (rdata->rowCount >= PQntuples(rdata->execResult)) {
	Tcl_SetObjResult(interp, literals[LIT_0]);
	return TCL_OK;
    }

    /* Get the column names in the result set. */

    Tcl_ListObjLength(NULL, sdata->columnNames, &nColumns);
    if (nColumns == 0) {
	Tcl_SetObjResult(interp, literals[LIT_0]);
	return TCL_OK;
    }

    resultRow = Tcl_NewObj();
    Tcl_IncrRefCount(resultRow);

    /* Retrieve one column at a time. */
    for (i = 0; i < nColumns; ++i) {
	colObj = NULL;
	if (PQgetisnull(rdata->execResult, rdata->rowCount, i) == 0) {
	    buffSize = PQgetlength(rdata->execResult, rdata->rowCount, i);
	    buffer = PQgetvalue(rdata->execResult, rdata->rowCount, i);
	    if (PQftype(rdata->execResult, i) == BYTEAOID) {
		/*
		 * Postgres returns backslash-escape sequences for
		 * binary data. Substitute them away.
		 */
		Tcl_Obj* toSubst;
		toSubst = Tcl_NewStringObj(buffer, buffSize);
		Tcl_IncrRefCount(toSubst);
		colObj = Tcl_SubstObj(interp, toSubst, TCL_SUBST_BACKSLASHES);
		Tcl_DecrRefCount(toSubst);
	    } else {
		colObj = Tcl_NewStringObj((char*)buffer, buffSize);
	    }
	}

	if (lists) {
	    if (colObj == NULL) {
		colObj = Tcl_NewObj();
	    }
	    Tcl_ListObjAppendElement(NULL, resultRow, colObj);
	} else {
	    if (colObj != NULL) {
		Tcl_ListObjIndex(NULL, sdata->columnNames, i, &colName);
		Tcl_DictObjPut(NULL, resultRow, colName, colObj);
	    }
	}
    }

    /* Advance to the next row */
    rdata->rowCount += 1;

    /* Save the row in the given variable */

    if (Tcl_SetVar2Ex(interp, Tcl_GetString(objv[2]), NULL,
		      resultRow, TCL_LEAVE_ERR_MSG) == NULL) {
	goto cleanup;
    }

    Tcl_SetObjResult(interp, literals[LIT_1]);
    status = TCL_OK;

cleanup:
    Tcl_DecrRefCount(resultRow);
    return status;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeleteResultSetMetadata, DeleteResultSet --
 *
 *	Cleans up when a Postgres result set is no longer required.
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

    if (rdata->stmtName != NULL) {
	if (rdata->stmtName != sdata->stmtName) {
	    UnallocateStatement(sdata->cdata->pgPtr, rdata->stmtName);
	    ckfree(rdata->stmtName);
	} else {
	    sdata->flags &= ~ STMT_FLAG_BUSY;
	}
    }
    if (rdata->execResult != NULL) {
	PQclear(rdata->execResult);
    }
    DecrStatementRefCount(rdata->sdata);
    ckfree(rdata);
}

/*
 *-----------------------------------------------------------------------------
 *
 * CloneResultSet --
 *
 *	Attempts to clone a PostreSQL result set's metadata.
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
		     Tcl_NewStringObj("Postgres result sets are not clonable",
				      -1));
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ResultSetRowcountMethod --
 *
 *	Returns (if known) the number of rows affected by a Postgres statement.
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
    char * nTuples;
    Tcl_Object thisObject = Tcl_ObjectContextObject(context);
				/* The current result set object */
    ResultSetData* rdata = (ResultSetData*)
	Tcl_ObjectGetMetadata(thisObject, &resultSetDataType);
				/* Data pertaining to the current result set */
    StatementData* sdata = rdata->sdata;
				/* The current statement */
    ConnectionData* cdata = sdata->cdata;
    PerInterpData* pidata = cdata->pidata; /* Per-interp data */
    Tcl_Obj** literals = pidata->literals; /* Literal pool */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    nTuples = PQcmdTuples(rdata->execResult);
    if (strlen(nTuples) == 0) {
	Tcl_SetObjResult(interp, literals[LIT_0]);
    } else {
	Tcl_SetObjResult(interp,
	    Tcl_NewStringObj(nTuples, -1));
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tdbcpostgres_Init --
 *
 *	Initializes the TDBC-POSTGRES bridge when this library is loaded.
 *
 * Side effects:
 *	Creates the ::tdbc::postgres namespace and the commands that reside in it.
 *	Initializes the POSTGRES environment.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
DLLEXPORT int
Tdbcpostgres_Init(
    Tcl_Interp* interp		/* Tcl interpreter */
) {

    PerInterpData* pidata;	/* Per-interpreter data for this package */
    Tcl_Obj* nameObj;		/* Name of a class or method being looked up */
    Tcl_Object curClassObject;  /* Tcl_Object representing the current class */
    Tcl_Class curClass;		/* Tcl_Class representing the current class */
    int i;

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

    if (Tcl_PkgProvideEx(interp, "tdbc::postgres", PACKAGE_VERSION, NULL) != TCL_OK) {
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
	int isNew;
	Tcl_HashEntry* entry =
	    Tcl_CreateHashEntry(&(pidata->typeNumHash),
				INT2PTR(dataTypes[i].oid),
				&isNew);
	Tcl_Obj* nameObj = Tcl_NewStringObj(dataTypes[i].name, -1);
	Tcl_IncrRefCount(nameObj);
	Tcl_SetHashValue(entry, (ClientData) nameObj);
    }


    /*
     * Find the connection class, and attach an 'init' method to it.
     */

    nameObj = Tcl_NewStringObj("::tdbc::postgres::connection", -1);
	Tcl_IncrRefCount(nameObj);
    if ((curClassObject = Tcl_GetObjectFromObj(interp, nameObj)) == NULL) {
	Tcl_DecrRefCount(nameObj);
	return TCL_ERROR;
    }
    Tcl_DecrRefCount(nameObj);
    curClass = Tcl_GetObjectAsClass(curClassObject);
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

    nameObj = Tcl_NewStringObj("::tdbc::postgres::statement", -1);
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

    nameObj = Tcl_NewStringObj("::tdbc::postgres::resultset", -1);
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
     * Initialize the PostgreSQL library if this is the first interp using it.
     */

    Tcl_MutexLock(&pgMutex);
    if (pgRefCount == 0) {
	if ((pgLoadHandle = PostgresqlInitStubs(interp)) == NULL) {
	    Tcl_MutexUnlock(&pgMutex);
	    return TCL_ERROR;
	}
    }
    ++pgRefCount;
    Tcl_MutexUnlock(&pgMutex);

    return TCL_OK;
}
#ifdef __cplusplus
}
#endif  /* __cplusplus */

/*
 *-----------------------------------------------------------------------------
 *
 * DeletePerInterpData --
 *
 *	Delete per-interpreter data when the POSTGRES package is finalized
 *
 * Side effects:
 *
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
   ckfree(pidata);

   Tcl_MutexLock(&pgMutex);
   if (--pgRefCount == 0) {
       Tcl_FSUnloadFile(NULL, pgLoadHandle);
       pgLoadHandle = NULL;
   }
   Tcl_MutexUnlock(&pgMutex);

}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * End:
 */
