
/* Parser-tokenizer link interface */

#ifndef Py_PARSETOK_H
#define Py_PARSETOK_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int error;
    char *filename;
    int lineno;
    int offset;
    char *text;
    int token;
    int expected;
} perrdetail;

extern DL_IMPORT(node *) PyParser_ParseString(char *, grammar *, int,
                                              perrdetail *);
extern DL_IMPORT(node *) PyParser_ParseFile (FILE *, char *, grammar *, int,
                                             char *, char *, perrdetail *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PARSETOK_H */
