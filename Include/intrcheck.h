
#ifndef Py_INTRCHECK_H
#define Py_INTRCHECK_H
#ifdef __cplusplus
extern "C" {
#endif

extern DL_IMPORT(int) PyOS_InterruptOccurred(void);
extern DL_IMPORT(void) PyOS_InitInterrupts(void);
DL_IMPORT(void) PyOS_AfterFork(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTRCHECK_H */
