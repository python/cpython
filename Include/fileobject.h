
/* File object interface */

#ifndef Py_FILEOBJECT_H
#define Py_FILEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	PyObject_HEAD
	FILE *f_fp;
	PyObject *f_name;
	PyObject *f_mode;
	int (*f_close)(FILE *);
	int f_softspace; /* Flag used by 'print' command */
	int f_binary; /* Flag which indicates whether the file is open
			 open in binary (1) or test (0) mode */
#ifdef WITH_UNIVERSAL_NEWLINES
	int f_univ_newline;	/* Handle any newline convention */
	int f_newlinetypes;	/* Types of newlines seen */
	int f_skipnextlf;	/* Skip next \n */
#endif
} PyFileObject;

extern DL_IMPORT(PyTypeObject) PyFile_Type;

#define PyFile_Check(op) PyObject_TypeCheck(op, &PyFile_Type)
#define PyFile_CheckExact(op) ((op)->ob_type == &PyFile_Type)

extern DL_IMPORT(PyObject *) PyFile_FromString(char *, char *);
extern DL_IMPORT(void) PyFile_SetBufSize(PyObject *, int);
extern DL_IMPORT(PyObject *) PyFile_FromFile(FILE *, char *, char *,
                                             int (*)(FILE *));
extern DL_IMPORT(FILE *) PyFile_AsFile(PyObject *);
extern DL_IMPORT(PyObject *) PyFile_Name(PyObject *);
extern DL_IMPORT(PyObject *) PyFile_GetLine(PyObject *, int);
extern DL_IMPORT(int) PyFile_WriteObject(PyObject *, PyObject *, int);
extern DL_IMPORT(int) PyFile_SoftSpace(PyObject *, int);
extern DL_IMPORT(int) PyFile_WriteString(const char *, PyObject *);
extern DL_IMPORT(int) PyObject_AsFileDescriptor(PyObject *);

/* The default encoding used by the platform file system APIs
   If non-NULL, this is different than the default encoding for strings
*/
extern DL_IMPORT(const char *) Py_FileSystemDefaultEncoding;

#ifdef WITH_UNIVERSAL_NEWLINES
/* Routines to replace fread() and fgets() which accept any of \r, \n
   or \r\n as line terminators.
*/
#define PY_STDIOTEXTMODE "b"
char *Py_UniversalNewlineFgets(char *, int, FILE*, PyObject *);
size_t Py_UniversalNewlineFread(char *, size_t, FILE *, PyObject *);
#else
#define PY_STDIOTEXTMODE ""
#define Py_UniversalNewlineFgets(buf, len, fp, obj) fgets((buf), (len), (fp))
#define Py_UniversalNewlineFread(buf, len, fp, obj)
		fread((buf), 1, (len), (fp))
#endif /* WITH_UNIVERSAL_NEWLINES */
#ifdef __cplusplus
}
#endif
#endif /* !Py_FILEOBJECT_H */
