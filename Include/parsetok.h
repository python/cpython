
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

#if 0
#define PyPARSE_YIELD_IS_KEYWORD	0x0001
#endif

extern DL_IMPORT(node *) PyParser_ParseString(char *, grammar *, int,
                                              perrdetail *);
extern DL_IMPORT(node *) PyParser_ParseFile (FILE *, char *, grammar *, int,
                                             char *, char *, perrdetail *);

extern DL_IMPORT(node *) PyParser_ParseStringFlags(char *, grammar *, int,
                                              perrdetail *, int);
extern DL_IMPORT(node *) PyParser_ParseFileFlags(FILE *, char *, grammar *,
						 int, char *, char *,
						 perrdetail *, int);

extern DL_IMPORT(node *) PyParser_ParseStringFlagsFilename(char *,
					      char *,
					      grammar *, int,
                                              perrdetail *, int);
#ifdef __cplusplus
}
#endif
#endif /* !Py_PARSETOK_H */
