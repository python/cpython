/* Poor-man's template.  Macros used:
   TESTNAME     name of the test (like test_long_api_inner)
   TYPENAME     the signed type (like long)
   F_S_TO_PY    convert signed to pylong; TYPENAME -> PyObject*
   F_PY_TO_S    convert pylong to signed; PyObject* -> TYPENAME
   F_U_TO_PY    convert unsigned to pylong; unsigned TYPENAME -> PyObject*
   F_PY_TO_U    convert pylong to unsigned; PyObject* -> unsigned TYPENAME
*/

static PyObject *
TESTNAME(PyObject *error(const char*))
{
    const int NBITS = sizeof(TYPENAME) * 8;
    unsigned TYPENAME base;
    PyObject *pyresult;
    int i;

    /* Note:  This test lets PyObjects leak if an error is raised.  Since
       an error should never be raised, leaks are impossible <wink>. */

    /* Test native -> PyLong -> native roundtrip identity.
     * Generate all powers of 2, and test them and their negations,
     * plus the numbers +-1 off from them.
     */
    base = 1;
    for (i = 0;
         i < NBITS + 1;  /* on last, base overflows to 0 */
         ++i, base <<= 1)
    {
        int j;
        for (j = 0; j < 6; ++j) {
            TYPENAME in, out;
            unsigned TYPENAME uin, uout;

            /* For 0, 1, 2 use base; for 3, 4, 5 use -base */
            uin = j < 3 ? base : 0U - base;

            /* For 0 & 3, subtract 1.
             * For 1 & 4, leave alone.
             * For 2 & 5, add 1.
             */
            uin += (unsigned TYPENAME)(TYPENAME)(j % 3 - 1);

            pyresult = F_U_TO_PY(uin);
            if (pyresult == NULL)
                return error(
                 "unsigned unexpected null result");

            uout = F_PY_TO_U(pyresult);
            if (uout == (unsigned TYPENAME)-1 && PyErr_Occurred())
                return error(
                    "unsigned unexpected -1 result");
            if (uout != uin)
                return error(
                    "unsigned output != input");
            UNBIND(pyresult);

            in = (TYPENAME)uin;
            pyresult = F_S_TO_PY(in);
            if (pyresult == NULL)
                return error(
                    "signed unexpected null result");

            out = F_PY_TO_S(pyresult);
            if (out == (TYPENAME)-1 && PyErr_Occurred())
                return error(
                    "signed unexpected -1 result");
            if (out != in)
                return error(
                    "signed output != input");
            UNBIND(pyresult);
        }
    }

    /* Overflow tests.  The loop above ensured that all limit cases that
     * should not overflow don't overflow, so all we need to do here is
     * provoke one-over-the-limit cases (not exhaustive, but sharp).
     */
    {
        PyObject *one, *x, *y;
        TYPENAME out;
        unsigned TYPENAME uout;

        one = PyLong_FromLong(1);
        if (one == NULL)
            return error(
                "unexpected NULL from PyLong_FromLong");

        /* Unsigned complains about -1? */
        x = PyNumber_Negative(one);
        if (x == NULL)
            return error(
                "unexpected NULL from PyNumber_Negative");

        uout = F_PY_TO_U(x);
        if (uout != (unsigned TYPENAME)-1 || !PyErr_Occurred())
            return error(
                "PyLong_AsUnsignedXXX(-1) didn't complain");
        if (!PyErr_ExceptionMatches(PyExc_OverflowError))
            return error(
                "PyLong_AsUnsignedXXX(-1) raised "
                "something other than OverflowError");
        PyErr_Clear();
        UNBIND(x);

        /* Unsigned complains about 2**NBITS? */
        y = PyLong_FromLong((long)NBITS);
        if (y == NULL)
            return error(
                "unexpected NULL from PyLong_FromLong");

        x = PyNumber_Lshift(one, y); /* 1L << NBITS, == 2**NBITS */
        UNBIND(y);
        if (x == NULL)
            return error(
                "unexpected NULL from PyNumber_Lshift");

        uout = F_PY_TO_U(x);
        if (uout != (unsigned TYPENAME)-1 || !PyErr_Occurred())
            return error(
                "PyLong_AsUnsignedXXX(2**NBITS) didn't "
                "complain");
        if (!PyErr_ExceptionMatches(PyExc_OverflowError))
            return error(
                "PyLong_AsUnsignedXXX(2**NBITS) raised "
                "something other than OverflowError");
        PyErr_Clear();

        /* Signed complains about 2**(NBITS-1)?
           x still has 2**NBITS. */
        y = PyNumber_Rshift(x, one); /* 2**(NBITS-1) */
        UNBIND(x);
        if (y == NULL)
            return error(
                "unexpected NULL from PyNumber_Rshift");

        out = F_PY_TO_S(y);
        if (out != (TYPENAME)-1 || !PyErr_Occurred())
            return error(
                "PyLong_AsXXX(2**(NBITS-1)) didn't "
                "complain");
        if (!PyErr_ExceptionMatches(PyExc_OverflowError))
            return error(
                "PyLong_AsXXX(2**(NBITS-1)) raised "
                "something other than OverflowError");
        PyErr_Clear();

        /* Signed complains about -2**(NBITS-1)-1?;
           y still has 2**(NBITS-1). */
        x = PyNumber_Negative(y);  /* -(2**(NBITS-1)) */
        UNBIND(y);
        if (x == NULL)
            return error(
                "unexpected NULL from PyNumber_Negative");

        y = PyNumber_Subtract(x, one); /* -(2**(NBITS-1))-1 */
        UNBIND(x);
        if (y == NULL)
            return error(
                "unexpected NULL from PyNumber_Subtract");

        out = F_PY_TO_S(y);
        if (out != (TYPENAME)-1 || !PyErr_Occurred())
            return error(
                "PyLong_AsXXX(-2**(NBITS-1)-1) didn't "
                "complain");
        if (!PyErr_ExceptionMatches(PyExc_OverflowError))
            return error(
                "PyLong_AsXXX(-2**(NBITS-1)-1) raised "
                "something other than OverflowError");
        PyErr_Clear();
        UNBIND(y);

        Py_XDECREF(x);
        Py_XDECREF(y);
        Py_DECREF(one);
    }

    /* Test F_PY_TO_{S,U} on non-pylong input. This should raise a TypeError. */
    {
        TYPENAME out;
        unsigned TYPENAME uout;

        Py_INCREF(Py_None);

        out = F_PY_TO_S(Py_None);
        if (out != (TYPENAME)-1 || !PyErr_Occurred())
            return error("PyLong_AsXXX(None) didn't complain");
        if (!PyErr_ExceptionMatches(PyExc_TypeError))
            return error("PyLong_AsXXX(None) raised "
                         "something other than TypeError");
        PyErr_Clear();

        uout = F_PY_TO_U(Py_None);
        if (uout != (unsigned TYPENAME)-1 || !PyErr_Occurred())
            return error("PyLong_AsXXX(None) didn't complain");
        if (!PyErr_ExceptionMatches(PyExc_TypeError))
            return error("PyLong_AsXXX(None) raised "
                         "something other than TypeError");
        PyErr_Clear();

        Py_DECREF(Py_None);
    }

    return Py_NewRef(Py_None);
}
