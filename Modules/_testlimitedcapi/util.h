#define NULLABLE(x) do {                    \
        if (x == Py_None) {                 \
            x = NULL;                       \
        }                                   \
    } while (0);

#define RETURN_INT(value) do {              \
        int _ret = (value);                 \
        if (_ret == -1) {                   \
            assert(PyErr_Occurred());       \
            return NULL;                    \
        }                                   \
        assert(!PyErr_Occurred());          \
        return PyLong_FromLong(_ret);       \
    } while (0)

#define RETURN_SIZE(value) do {             \
        Py_ssize_t _ret = (value);          \
        if (_ret == -1) {                   \
            assert(PyErr_Occurred());       \
            return NULL;                    \
        }                                   \
        assert(!PyErr_Occurred());          \
        return PyLong_FromSsize_t(_ret);    \
    } while (0)

/* Marker to check that pointer value was set. */
static const char uninitialized[] = "uninitialized";
#define UNINITIALIZED_PTR ((void *)uninitialized)
/* Marker to check that Py_ssize_t value was set. */
#define UNINITIALIZED_SIZE ((Py_ssize_t)236892191)
/* Marker to check that integer value was set. */
#define UNINITIALIZED_INT (63256717)
