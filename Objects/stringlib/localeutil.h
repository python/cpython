/* _PyUnicode_InsertThousandsGrouping() helper functions */

typedef struct {
    const char *grouping;
    char previous;
    Py_ssize_t i; /* Where we're currently pointing in grouping. */
} GroupGenerator;


static void
GroupGenerator_init(GroupGenerator *self, const char *grouping)
{
    self->grouping = grouping;
    self->i = 0;
    self->previous = 0;
}


/* Returns the next grouping, or 0 to signify end. */
static Py_ssize_t
GroupGenerator_next(GroupGenerator *self)
{
    /* Note that we don't really do much error checking here. If a
       grouping string contains just CHAR_MAX, for example, then just
       terminate the generator. That shouldn't happen, but at least we
       fail gracefully. */
    switch (self->grouping[self->i]) {
    case 0:
        return self->previous;
    case CHAR_MAX:
        /* Stop the generator. */
        return 0;
    default: {
        char ch = self->grouping[self->i];
        self->previous = ch;
        self->i++;
        return (Py_ssize_t)ch;
    }
    }
}


/* Fill in some digits, leading zeros, and thousands separator. All
   are optional, depending on when we're called. */
static void
InsertThousandsGrouping_fill(_PyUnicodeWriter *writer, Py_ssize_t *buffer_pos,
                             PyObject *digits, Py_ssize_t *digits_pos,
                             Py_ssize_t n_chars, Py_ssize_t n_zeros,
                             PyObject *thousands_sep, Py_ssize_t thousands_sep_len,
                             Py_UCS4 *maxchar)
{
    if (!writer) {
        /* if maxchar > 127, maxchar is already set */
        if (*maxchar == 127 && thousands_sep) {
            Py_UCS4 maxchar2 = PyUnicode_MAX_CHAR_VALUE(thousands_sep);
            *maxchar = Py_MAX(*maxchar, maxchar2);
        }
        return;
    }

    if (thousands_sep) {
        *buffer_pos -= thousands_sep_len;

        /* Copy the thousands_sep chars into the buffer. */
        _PyUnicode_FastCopyCharacters(writer->buffer, *buffer_pos,
                                      thousands_sep, 0,
                                      thousands_sep_len);
    }

    *buffer_pos -= n_chars;
    *digits_pos -= n_chars;
    _PyUnicode_FastCopyCharacters(writer->buffer, *buffer_pos,
                                  digits, *digits_pos,
                                  n_chars);

    if (n_zeros) {
        *buffer_pos -= n_zeros;
        enum PyUnicode_Kind kind = PyUnicode_KIND(writer->buffer);
        void *data = PyUnicode_DATA(writer->buffer);
        unicode_fill(kind, data, '0', *buffer_pos, n_zeros);
    }
}
