#ifndef Py_ASSERT_H
#define Py_ASSERT_H
#ifdef __cplusplus
extern "C" {
#endif


#ifdef MPW /* This is for MPW's File command */

#define assert(e) { if (!(e)) { printf("### Python: Assertion failed:\n\
    File %s; Line %d\n", __FILE__, __LINE__); abort(); } }
#else
#define assert(e) { if (!(e)) { printf("Assertion failed\n"); abort(); } }
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_ASSERT_H */
