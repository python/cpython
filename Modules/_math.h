double _Py_expm1(double x);

#ifdef HAVE_EXPM1
#define m_expm1 expm1
#else
/* if the system doesn't have expm1, use the substitute
   function defined in Modules/_math.c. */
#define m_expm1 _Py_expm1
#endif
