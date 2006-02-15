#ifndef Py_STRTOD_H
#define Py_STRTOD_H

#ifdef __cplusplus
extern "C" {
#endif


PyAPI_FUNC(double) PyOS_ascii_strtod(const char *str, char **ptr);
PyAPI_FUNC(double) PyOS_ascii_atof(const char *str);
PyAPI_FUNC(char *) PyOS_ascii_formatd(char *buffer, size_t buf_len,  const char *format, double d);


#ifdef __cplusplus
}
#endif

#endif /* !Py_STRTOD_H */
