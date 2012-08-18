double _Py_acosh(double x);
double _Py_asinh(double x);
double _Py_atanh(double x);
double _Py_expm1(double x);
double _Py_log1p(double x);

#ifdef HAVE_ACOSH
#define m_acosh acosh
#else
/* if the system doesn't have acosh, use the substitute
   function defined in Modules/_math.c. */
#define m_acosh _Py_acosh
#endif

#ifdef HAVE_ASINH
#define m_asinh asinh
#else
/* if the system doesn't have asinh, use the substitute
   function defined in Modules/_math.c. */
#define m_asinh _Py_asinh
#endif

#ifdef HAVE_ATANH
#define m_atanh atanh
#else
/* if the system doesn't have atanh, use the substitute
   function defined in Modules/_math.c. */
#define m_atanh _Py_atanh
#endif

#ifdef HAVE_EXPM1
#define m_expm1 expm1
#else
/* if the system doesn't have expm1, use the substitute
   function defined in Modules/_math.c. */
#define m_expm1 _Py_expm1
#endif

/* Use the substitute from _math.c on all platforms:
   it includes workarounds for buggy handling of zeros. */
#define m_log1p _Py_log1p
