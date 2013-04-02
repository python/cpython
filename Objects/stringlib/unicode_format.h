/*
    unicode_format.h -- implementation of str.format().
*/

/************************************************************************/
/***********   Global data structures and forward declarations  *********/
/************************************************************************/

/*
   A SubString consists of the characters between two string or
   unicode pointers.
*/
typedef struct {
    PyObject *str; /* borrowed reference */
    Py_ssize_t start, end;
} SubString;


typedef enum {
    ANS_INIT,
    ANS_AUTO,
    ANS_MANUAL
} AutoNumberState;   /* Keep track if we're auto-numbering fields */

/* Keeps track of our auto-numbering state, and which number field we're on */
typedef struct {
    AutoNumberState an_state;
    int an_field_number;
} AutoNumber;


/* forward declaration for recursion */
static PyObject *
build_string(SubString *input, PyObject *args, PyObject *kwargs,
             int recursion_depth, AutoNumber *auto_number);



/************************************************************************/
/**************************  Utility  functions  ************************/
/************************************************************************/

static void
AutoNumber_Init(AutoNumber *auto_number)
{
    auto_number->an_state = ANS_INIT;
    auto_number->an_field_number = 0;
}

/* fill in a SubString from a pointer and length */
Py_LOCAL_INLINE(void)
SubString_init(SubString *str, PyObject *s, Py_ssize_t start, Py_ssize_t end)
{
    str->str = s;
    str->start = start;
    str->end = end;
}

/* return a new string.  if str->str is NULL, return None */
Py_LOCAL_INLINE(PyObject *)
SubString_new_object(SubString *str)
{
    if (str->str == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    return PyUnicode_Substring(str->str, str->start, str->end);
}

/* return a new string.  if str->str is NULL, return None */
Py_LOCAL_INLINE(PyObject *)
SubString_new_object_or_empty(SubString *str)
{
    if (str->str == NULL) {
        return PyUnicode_New(0, 0);
    }
    return SubString_new_object(str);
}

/* Return 1 if an error has been detected switching between automatic
   field numbering and manual field specification, else return 0. Set
   ValueError on error. */
static int
autonumber_state_error(AutoNumberState state, int field_name_is_empty)
{
    if (state == ANS_MANUAL) {
        if (field_name_is_empty) {
            PyErr_SetString(PyExc_ValueError, "cannot switch from "
                            "manual field specification to "
                            "automatic field numbering");
            return 1;
        }
    }
    else {
        if (!field_name_is_empty) {
            PyErr_SetString(PyExc_ValueError, "cannot switch from "
                            "automatic field numbering to "
                            "manual field specification");
            return 1;
        }
    }
    return 0;
}


/************************************************************************/
/***********  Format string parsing -- integers and identifiers *********/
/************************************************************************/

static Py_ssize_t
get_integer(const SubString *str)
{
    Py_ssize_t accumulator = 0;
    Py_ssize_t digitval;
    Py_ssize_t i;

    /* empty string is an error */
    if (str->start >= str->end)
        return -1;

    for (i = str->start; i < str->end; i++) {
        digitval = Py_UNICODE_TODECIMAL(PyUnicode_READ_CHAR(str->str, i));
        if (digitval < 0)
            return -1;
        /*
           Detect possible overflow before it happens:

              accumulator * 10 + digitval > PY_SSIZE_T_MAX if and only if
              accumulator > (PY_SSIZE_T_MAX - digitval) / 10.
        */
        if (accumulator > (PY_SSIZE_T_MAX - digitval) / 10) {
            PyErr_Format(PyExc_ValueError,
                         "Too many decimal digits in format string");
            return -1;
        }
        accumulator = accumulator * 10 + digitval;
    }
    return accumulator;
}

/************************************************************************/
/******** Functions to get field objects and specification strings ******/
/************************************************************************/

/* do the equivalent of obj.name */
static PyObject *
getattr(PyObject *obj, SubString *name)
{
    PyObject *newobj;
    PyObject *str = SubString_new_object(name);
    if (str == NULL)
        return NULL;
    newobj = PyObject_GetAttr(obj, str);
    Py_DECREF(str);
    return newobj;
}

/* do the equivalent of obj[idx], where obj is a sequence */
static PyObject *
getitem_sequence(PyObject *obj, Py_ssize_t idx)
{
    return PySequence_GetItem(obj, idx);
}

/* do the equivalent of obj[idx], where obj is not a sequence */
static PyObject *
getitem_idx(PyObject *obj, Py_ssize_t idx)
{
    PyObject *newobj;
    PyObject *idx_obj = PyLong_FromSsize_t(idx);
    if (idx_obj == NULL)
        return NULL;
    newobj = PyObject_GetItem(obj, idx_obj);
    Py_DECREF(idx_obj);
    return newobj;
}

/* do the equivalent of obj[name] */
static PyObject *
getitem_str(PyObject *obj, SubString *name)
{
    PyObject *newobj;
    PyObject *str = SubString_new_object(name);
    if (str == NULL)
        return NULL;
    newobj = PyObject_GetItem(obj, str);
    Py_DECREF(str);
    return newobj;
}

typedef struct {
    /* the entire string we're parsing.  we assume that someone else
       is managing its lifetime, and that it will exist for the
       lifetime of the iterator.  can be empty */
    SubString str;

    /* index to where we are inside field_name */
    Py_ssize_t index;
} FieldNameIterator;


static int
FieldNameIterator_init(FieldNameIterator *self, PyObject *s,
                       Py_ssize_t start, Py_ssize_t end)
{
    SubString_init(&self->str, s, start, end);
    self->index = start;
    return 1;
}

static int
_FieldNameIterator_attr(FieldNameIterator *self, SubString *name)
{
    Py_UCS4 c;

    name->str = self->str.str;
    name->start = self->index;

    /* return everything until '.' or '[' */
    while (self->index < self->str.end) {
        c = PyUnicode_READ_CHAR(self->str.str, self->index++);
        switch (c) {
        case '[':
        case '.':
            /* backup so that we this character will be seen next time */
            self->index--;
            break;
        default:
            continue;
        }
        break;
    }
    /* end of string is okay */
    name->end = self->index;
    return 1;
}

static int
_FieldNameIterator_item(FieldNameIterator *self, SubString *name)
{
    int bracket_seen = 0;
    Py_UCS4 c;

    name->str = self->str.str;
    name->start = self->index;

    /* return everything until ']' */
    while (self->index < self->str.end) {
        c = PyUnicode_READ_CHAR(self->str.str, self->index++);
        switch (c) {
        case ']':
            bracket_seen = 1;
            break;
        default:
            continue;
        }
        break;
    }
    /* make sure we ended with a ']' */
    if (!bracket_seen) {
        PyErr_SetString(PyExc_ValueError, "Missing ']' in format string");
        return 0;
    }

    /* end of string is okay */
    /* don't include the ']' */
    name->end = self->index-1;
    return 1;
}

/* returns 0 on error, 1 on non-error termination, and 2 if it returns a value */
static int
FieldNameIterator_next(FieldNameIterator *self, int *is_attribute,
                       Py_ssize_t *name_idx, SubString *name)
{
    /* check at end of input */
    if (self->index >= self->str.end)
        return 1;

    switch (PyUnicode_READ_CHAR(self->str.str, self->index++)) {
    case '.':
        *is_attribute = 1;
        if (_FieldNameIterator_attr(self, name) == 0)
            return 0;
        *name_idx = -1;
        break;
    case '[':
        *is_attribute = 0;
        if (_FieldNameIterator_item(self, name) == 0)
            return 0;
        *name_idx = get_integer(name);
        if (*name_idx == -1 && PyErr_Occurred())
            return 0;
        break;
    default:
        /* Invalid character follows ']' */
        PyErr_SetString(PyExc_ValueError, "Only '.' or '[' may "
                        "follow ']' in format field specifier");
        return 0;
    }

    /* empty string is an error */
    if (name->start == name->end) {
        PyErr_SetString(PyExc_ValueError, "Empty attribute in format string");
        return 0;
    }

    return 2;
}


/* input: field_name
   output: 'first' points to the part before the first '[' or '.'
           'first_idx' is -1 if 'first' is not an integer, otherwise
                       it's the value of first converted to an integer
           'rest' is an iterator to return the rest
*/
static int
field_name_split(PyObject *str, Py_ssize_t start, Py_ssize_t end, SubString *first,
                 Py_ssize_t *first_idx, FieldNameIterator *rest,
                 AutoNumber *auto_number)
{
    Py_UCS4 c;
    Py_ssize_t i = start;
    int field_name_is_empty;
    int using_numeric_index;

    /* find the part up until the first '.' or '[' */
    while (i < end) {
        switch (c = PyUnicode_READ_CHAR(str, i++)) {
        case '[':
        case '.':
            /* backup so that we this character is available to the
               "rest" iterator */
            i--;
            break;
        default:
            continue;
        }
        break;
    }

    /* set up the return values */
    SubString_init(first, str, start, i);
    FieldNameIterator_init(rest, str, i, end);

    /* see if "first" is an integer, in which case it's used as an index */
    *first_idx = get_integer(first);
    if (*first_idx == -1 && PyErr_Occurred())
        return 0;

    field_name_is_empty = first->start >= first->end;

    /* If the field name is omitted or if we have a numeric index
       specified, then we're doing numeric indexing into args. */
    using_numeric_index = field_name_is_empty || *first_idx != -1;

    /* We always get here exactly one time for each field we're
       processing. And we get here in field order (counting by left
       braces). So this is the perfect place to handle automatic field
       numbering if the field name is omitted. */

    /* Check if we need to do the auto-numbering. It's not needed if
       we're called from string.Format routines, because it's handled
       in that class by itself. */
    if (auto_number) {
        /* Initialize our auto numbering state if this is the first
           time we're either auto-numbering or manually numbering. */
        if (auto_number->an_state == ANS_INIT && using_numeric_index)
            auto_number->an_state = field_name_is_empty ?
                ANS_AUTO : ANS_MANUAL;

        /* Make sure our state is consistent with what we're doing
           this time through. Only check if we're using a numeric
           index. */
        if (using_numeric_index)
            if (autonumber_state_error(auto_number->an_state,
                                       field_name_is_empty))
                return 0;
        /* Zero length field means we want to do auto-numbering of the
           fields. */
        if (field_name_is_empty)
            *first_idx = (auto_number->an_field_number)++;
    }

    return 1;
}


/*
    get_field_object returns the object inside {}, before the
    format_spec.  It handles getindex and getattr lookups and consumes
    the entire input string.
*/
static PyObject *
get_field_object(SubString *input, PyObject *args, PyObject *kwargs,
                 AutoNumber *auto_number)
{
    PyObject *obj = NULL;
    int ok;
    int is_attribute;
    SubString name;
    SubString first;
    Py_ssize_t index;
    FieldNameIterator rest;

    if (!field_name_split(input->str, input->start, input->end, &first,
                          &index, &rest, auto_number)) {
        goto error;
    }

    if (index == -1) {
        /* look up in kwargs */
        PyObject *key = SubString_new_object(&first);
        if (key == NULL)
            goto error;

        /* Use PyObject_GetItem instead of PyDict_GetItem because this
           code is no longer just used with kwargs. It might be passed
           a non-dict when called through format_map. */
        if ((kwargs == NULL) || (obj = PyObject_GetItem(kwargs, key)) == NULL) {
            PyErr_SetObject(PyExc_KeyError, key);
            Py_DECREF(key);
            goto error;
        }
        Py_DECREF(key);
    }
    else {
        /* If args is NULL, we have a format string with a positional field
           with only kwargs to retrieve it from. This can only happen when
           used with format_map(), where positional arguments are not
           allowed. */
        if (args == NULL) {
            PyErr_SetString(PyExc_ValueError, "Format string contains "
                            "positional fields");
            goto error;
        }

        /* look up in args */
        obj = PySequence_GetItem(args, index);
        if (obj == NULL)
            goto error;
    }

    /* iterate over the rest of the field_name */
    while ((ok = FieldNameIterator_next(&rest, &is_attribute, &index,
                                        &name)) == 2) {
        PyObject *tmp;

        if (is_attribute)
            /* getattr lookup "." */
            tmp = getattr(obj, &name);
        else
            /* getitem lookup "[]" */
            if (index == -1)
                tmp = getitem_str(obj, &name);
            else
                if (PySequence_Check(obj))
                    tmp = getitem_sequence(obj, index);
                else
                    /* not a sequence */
                    tmp = getitem_idx(obj, index);
        if (tmp == NULL)
            goto error;

        /* assign to obj */
        Py_DECREF(obj);
        obj = tmp;
    }
    /* end of iterator, this is the non-error case */
    if (ok == 1)
        return obj;
error:
    Py_XDECREF(obj);
    return NULL;
}

/************************************************************************/
/*****************  Field rendering functions  **************************/
/************************************************************************/

/*
    render_field() is the main function in this section.  It takes the
    field object and field specification string generated by
    get_field_and_spec, and renders the field into the output string.

    render_field calls fieldobj.__format__(format_spec) method, and
    appends to the output.
*/
static int
render_field(PyObject *fieldobj, SubString *format_spec, _PyUnicodeWriter *writer)
{
    int ok = 0;
    PyObject *result = NULL;
    PyObject *format_spec_object = NULL;
    int (*formatter) (_PyUnicodeWriter*, PyObject *, PyObject *, Py_ssize_t, Py_ssize_t) = NULL;
    int err;

    /* If we know the type exactly, skip the lookup of __format__ and just
       call the formatter directly. */
    if (PyUnicode_CheckExact(fieldobj))
        formatter = _PyUnicode_FormatAdvancedWriter;
    else if (PyLong_CheckExact(fieldobj))
        formatter = _PyLong_FormatAdvancedWriter;
    else if (PyFloat_CheckExact(fieldobj))
        formatter = _PyFloat_FormatAdvancedWriter;
    else if (PyComplex_CheckExact(fieldobj))
        formatter = _PyComplex_FormatAdvancedWriter;

    if (formatter) {
        /* we know exactly which formatter will be called when __format__ is
           looked up, so call it directly, instead. */
        err = formatter(writer, fieldobj, format_spec->str,
                        format_spec->start, format_spec->end);
        return (err == 0);
    }
    else {
        /* We need to create an object out of the pointers we have, because
           __format__ takes a string/unicode object for format_spec. */
        if (format_spec->str)
            format_spec_object = PyUnicode_Substring(format_spec->str,
                                                     format_spec->start,
                                                     format_spec->end);
        else
            format_spec_object = PyUnicode_New(0, 0);
        if (format_spec_object == NULL)
            goto done;

        result = PyObject_Format(fieldobj, format_spec_object);
    }
    if (result == NULL)
        goto done;

    if (_PyUnicodeWriter_WriteStr(writer, result) == -1)
        goto done;
    ok = 1;

done:
    Py_XDECREF(format_spec_object);
    Py_XDECREF(result);
    return ok;
}

static int
parse_field(SubString *str, SubString *field_name, SubString *format_spec,
            Py_UCS4 *conversion)
{
    /* Note this function works if the field name is zero length,
       which is good.  Zero length field names are handled later, in
       field_name_split. */

    Py_UCS4 c = 0;

    /* initialize these, as they may be empty */
    *conversion = '\0';
    SubString_init(format_spec, NULL, 0, 0);

    /* Search for the field name.  it's terminated by the end of
       the string, or a ':' or '!' */
    field_name->str = str->str;
    field_name->start = str->start;
    while (str->start < str->end) {
        switch ((c = PyUnicode_READ_CHAR(str->str, str->start++))) {
        case ':':
        case '!':
            break;
        default:
            continue;
        }
        break;
    }

    if (c == '!' || c == ':') {
        /* we have a format specifier and/or a conversion */
        /* don't include the last character */
        field_name->end = str->start-1;

        /* the format specifier is the rest of the string */
        format_spec->str = str->str;
        format_spec->start = str->start;
        format_spec->end = str->end;

        /* see if there's a conversion specifier */
        if (c == '!') {
            /* there must be another character present */
            if (format_spec->start >= format_spec->end) {
                PyErr_SetString(PyExc_ValueError,
                                "end of format while looking for conversion "
                                "specifier");
                return 0;
            }
            *conversion = PyUnicode_READ_CHAR(format_spec->str, format_spec->start++);

            /* if there is another character, it must be a colon */
            if (format_spec->start < format_spec->end) {
                c = PyUnicode_READ_CHAR(format_spec->str, format_spec->start++);
                if (c != ':') {
                    PyErr_SetString(PyExc_ValueError,
                                    "expected ':' after format specifier");
                    return 0;
                }
            }
        }
    }
    else
        /* end of string, there's no format_spec or conversion */
        field_name->end = str->start;

    return 1;
}

/************************************************************************/
/******* Output string allocation and escape-to-markup processing  ******/
/************************************************************************/

/* MarkupIterator breaks the string into pieces of either literal
   text, or things inside {} that need to be marked up.  it is
   designed to make it easy to wrap a Python iterator around it, for
   use with the Formatter class */

typedef struct {
    SubString str;
} MarkupIterator;

static int
MarkupIterator_init(MarkupIterator *self, PyObject *str,
                    Py_ssize_t start, Py_ssize_t end)
{
    SubString_init(&self->str, str, start, end);
    return 1;
}

/* returns 0 on error, 1 on non-error termination, and 2 if it got a
   string (or something to be expanded) */
static int
MarkupIterator_next(MarkupIterator *self, SubString *literal,
                    int *field_present, SubString *field_name,
                    SubString *format_spec, Py_UCS4 *conversion,
                    int *format_spec_needs_expanding)
{
    int at_end;
    Py_UCS4 c = 0;
    Py_ssize_t start;
    int count;
    Py_ssize_t len;
    int markup_follows = 0;

    /* initialize all of the output variables */
    SubString_init(literal, NULL, 0, 0);
    SubString_init(field_name, NULL, 0, 0);
    SubString_init(format_spec, NULL, 0, 0);
    *conversion = '\0';
    *format_spec_needs_expanding = 0;
    *field_present = 0;

    /* No more input, end of iterator.  This is the normal exit
       path. */
    if (self->str.start >= self->str.end)
        return 1;

    start = self->str.start;

    /* First read any literal text. Read until the end of string, an
       escaped '{' or '}', or an unescaped '{'.  In order to never
       allocate memory and so I can just pass pointers around, if
       there's an escaped '{' or '}' then we'll return the literal
       including the brace, but no format object.  The next time
       through, we'll return the rest of the literal, skipping past
       the second consecutive brace. */
    while (self->str.start < self->str.end) {
        switch (c = PyUnicode_READ_CHAR(self->str.str, self->str.start++)) {
        case '{':
        case '}':
            markup_follows = 1;
            break;
        default:
            continue;
        }
        break;
    }

    at_end = self->str.start >= self->str.end;
    len = self->str.start - start;

    if ((c == '}') && (at_end ||
                       (c != PyUnicode_READ_CHAR(self->str.str,
                                                 self->str.start)))) {
        PyErr_SetString(PyExc_ValueError, "Single '}' encountered "
                        "in format string");
        return 0;
    }
    if (at_end && c == '{') {
        PyErr_SetString(PyExc_ValueError, "Single '{' encountered "
                        "in format string");
        return 0;
    }
    if (!at_end) {
        if (c == PyUnicode_READ_CHAR(self->str.str, self->str.start)) {
            /* escaped } or {, skip it in the input.  there is no
               markup object following us, just this literal text */
            self->str.start++;
            markup_follows = 0;
        }
        else
            len--;
    }

    /* record the literal text */
    literal->str = self->str.str;
    literal->start = start;
    literal->end = start + len;

    if (!markup_follows)
        return 2;

    /* this is markup, find the end of the string by counting nested
       braces.  note that this prohibits escaped braces, so that
       format_specs cannot have braces in them. */
    *field_present = 1;
    count = 1;

    start = self->str.start;

    /* we know we can't have a zero length string, so don't worry
       about that case */
    while (self->str.start < self->str.end) {
        switch (c = PyUnicode_READ_CHAR(self->str.str, self->str.start++)) {
        case '{':
            /* the format spec needs to be recursively expanded.
               this is an optimization, and not strictly needed */
            *format_spec_needs_expanding = 1;
            count++;
            break;
        case '}':
            count--;
            if (count <= 0) {
                /* we're done.  parse and get out */
                SubString s;

                SubString_init(&s, self->str.str, start, self->str.start - 1);
                if (parse_field(&s, field_name, format_spec, conversion) == 0)
                    return 0;

                /* success */
                return 2;
            }
            break;
        }
    }

    /* end of string while searching for matching '}' */
    PyErr_SetString(PyExc_ValueError, "unmatched '{' in format");
    return 0;
}


/* do the !r or !s conversion on obj */
static PyObject *
do_conversion(PyObject *obj, Py_UCS4 conversion)
{
    /* XXX in pre-3.0, do we need to convert this to unicode, since it
       might have returned a string? */
    switch (conversion) {
    case 'r':
        return PyObject_Repr(obj);
    case 's':
        return PyObject_Str(obj);
    case 'a':
        return PyObject_ASCII(obj);
    default:
        if (conversion > 32 && conversion < 127) {
                /* It's the ASCII subrange; casting to char is safe
                   (assuming the execution character set is an ASCII
                   superset). */
                PyErr_Format(PyExc_ValueError,
                     "Unknown conversion specifier %c",
                     (char)conversion);
        } else
                PyErr_Format(PyExc_ValueError,
                     "Unknown conversion specifier \\x%x",
                     (unsigned int)conversion);
        return NULL;
    }
}

/* given:

   {field_name!conversion:format_spec}

   compute the result and write it to output.
   format_spec_needs_expanding is an optimization.  if it's false,
   just output the string directly, otherwise recursively expand the
   format_spec string.

   field_name is allowed to be zero length, in which case we
   are doing auto field numbering.
*/

static int
output_markup(SubString *field_name, SubString *format_spec,
              int format_spec_needs_expanding, Py_UCS4 conversion,
              _PyUnicodeWriter *writer, PyObject *args, PyObject *kwargs,
              int recursion_depth, AutoNumber *auto_number)
{
    PyObject *tmp = NULL;
    PyObject *fieldobj = NULL;
    SubString expanded_format_spec;
    SubString *actual_format_spec;
    int result = 0;

    /* convert field_name to an object */
    fieldobj = get_field_object(field_name, args, kwargs, auto_number);
    if (fieldobj == NULL)
        goto done;

    if (conversion != '\0') {
        tmp = do_conversion(fieldobj, conversion);
        if (tmp == NULL || PyUnicode_READY(tmp) == -1)
            goto done;

        /* do the assignment, transferring ownership: fieldobj = tmp */
        Py_DECREF(fieldobj);
        fieldobj = tmp;
        tmp = NULL;
    }

    /* if needed, recurively compute the format_spec */
    if (format_spec_needs_expanding) {
        tmp = build_string(format_spec, args, kwargs, recursion_depth-1,
                           auto_number);
        if (tmp == NULL || PyUnicode_READY(tmp) == -1)
            goto done;

        /* note that in the case we're expanding the format string,
           tmp must be kept around until after the call to
           render_field. */
        SubString_init(&expanded_format_spec, tmp, 0, PyUnicode_GET_LENGTH(tmp));
        actual_format_spec = &expanded_format_spec;
    }
    else
        actual_format_spec = format_spec;

    if (render_field(fieldobj, actual_format_spec, writer) == 0)
        goto done;

    result = 1;

done:
    Py_XDECREF(fieldobj);
    Py_XDECREF(tmp);

    return result;
}

/*
    do_markup is the top-level loop for the format() method.  It
    searches through the format string for escapes to markup codes, and
    calls other functions to move non-markup text to the output,
    and to perform the markup to the output.
*/
static int
do_markup(SubString *input, PyObject *args, PyObject *kwargs,
          _PyUnicodeWriter *writer, int recursion_depth, AutoNumber *auto_number)
{
    MarkupIterator iter;
    int format_spec_needs_expanding;
    int result;
    int field_present;
    SubString literal;
    SubString field_name;
    SubString format_spec;
    Py_UCS4 conversion;

    MarkupIterator_init(&iter, input->str, input->start, input->end);
    while ((result = MarkupIterator_next(&iter, &literal, &field_present,
                                         &field_name, &format_spec,
                                         &conversion,
                                         &format_spec_needs_expanding)) == 2) {
        if (literal.end != literal.start) {
            if (!field_present && iter.str.start == iter.str.end)
                writer->overallocate = 0;
            if (_PyUnicodeWriter_WriteSubstring(writer, literal.str,
                                                literal.start, literal.end) < 0)
                return 0;
        }

        if (field_present) {
            if (iter.str.start == iter.str.end)
                writer->overallocate = 0;
            if (!output_markup(&field_name, &format_spec,
                               format_spec_needs_expanding, conversion, writer,
                               args, kwargs, recursion_depth, auto_number))
                return 0;
        }
    }
    return result;
}


/*
    build_string allocates the output string and then
    calls do_markup to do the heavy lifting.
*/
static PyObject *
build_string(SubString *input, PyObject *args, PyObject *kwargs,
             int recursion_depth, AutoNumber *auto_number)
{
    _PyUnicodeWriter writer;
    Py_ssize_t minlen;

    /* check the recursion level */
    if (recursion_depth <= 0) {
        PyErr_SetString(PyExc_ValueError,
                        "Max string recursion exceeded");
        return NULL;
    }

    minlen = PyUnicode_GET_LENGTH(input->str) + 100;
    _PyUnicodeWriter_Init(&writer, minlen);

    if (!do_markup(input, args, kwargs, &writer, recursion_depth,
                   auto_number)) {
        _PyUnicodeWriter_Dealloc(&writer);
        return NULL;
    }

    return _PyUnicodeWriter_Finish(&writer);
}

/************************************************************************/
/*********** main routine ***********************************************/
/************************************************************************/

/* this is the main entry point */
static PyObject *
do_string_format(PyObject *self, PyObject *args, PyObject *kwargs)
{
    SubString input;

    /* PEP 3101 says only 2 levels, so that
       "{0:{1}}".format('abc', 's')            # works
       "{0:{1:{2}}}".format('abc', 's', '')    # fails
    */
    int recursion_depth = 2;

    AutoNumber auto_number;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    AutoNumber_Init(&auto_number);
    SubString_init(&input, self, 0, PyUnicode_GET_LENGTH(self));
    return build_string(&input, args, kwargs, recursion_depth, &auto_number);
}

static PyObject *
do_string_format_map(PyObject *self, PyObject *obj)
{
    return do_string_format(self, NULL, obj);
}


/************************************************************************/
/*********** formatteriterator ******************************************/
/************************************************************************/

/* This is used to implement string.Formatter.vparse().  It exists so
   Formatter can share code with the built in unicode.format() method.
   It's really just a wrapper around MarkupIterator that is callable
   from Python. */

typedef struct {
    PyObject_HEAD
    PyObject *str;
    MarkupIterator it_markup;
} formatteriterobject;

static void
formatteriter_dealloc(formatteriterobject *it)
{
    Py_XDECREF(it->str);
    PyObject_FREE(it);
}

/* returns a tuple:
   (literal, field_name, format_spec, conversion)

   literal is any literal text to output.  might be zero length
   field_name is the string before the ':'.  might be None
   format_spec is the string after the ':'.  mibht be None
   conversion is either None, or the string after the '!'
*/
static PyObject *
formatteriter_next(formatteriterobject *it)
{
    SubString literal;
    SubString field_name;
    SubString format_spec;
    Py_UCS4 conversion;
    int format_spec_needs_expanding;
    int field_present;
    int result = MarkupIterator_next(&it->it_markup, &literal, &field_present,
                                     &field_name, &format_spec, &conversion,
                                     &format_spec_needs_expanding);

    /* all of the SubString objects point into it->str, so no
       memory management needs to be done on them */
    assert(0 <= result && result <= 2);
    if (result == 0 || result == 1)
        /* if 0, error has already been set, if 1, iterator is empty */
        return NULL;
    else {
        PyObject *literal_str = NULL;
        PyObject *field_name_str = NULL;
        PyObject *format_spec_str = NULL;
        PyObject *conversion_str = NULL;
        PyObject *tuple = NULL;

        literal_str = SubString_new_object(&literal);
        if (literal_str == NULL)
            goto done;

        field_name_str = SubString_new_object(&field_name);
        if (field_name_str == NULL)
            goto done;

        /* if field_name is non-zero length, return a string for
           format_spec (even if zero length), else return None */
        format_spec_str = (field_present ?
                           SubString_new_object_or_empty :
                           SubString_new_object)(&format_spec);
        if (format_spec_str == NULL)
            goto done;

        /* if the conversion is not specified, return a None,
           otherwise create a one length string with the conversion
           character */
        if (conversion == '\0') {
            conversion_str = Py_None;
            Py_INCREF(conversion_str);
        }
        else
            conversion_str = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND,
                                                       &conversion, 1);
        if (conversion_str == NULL)
            goto done;

        tuple = PyTuple_Pack(4, literal_str, field_name_str, format_spec_str,
                             conversion_str);
    done:
        Py_XDECREF(literal_str);
        Py_XDECREF(field_name_str);
        Py_XDECREF(format_spec_str);
        Py_XDECREF(conversion_str);
        return tuple;
    }
}

static PyMethodDef formatteriter_methods[] = {
    {NULL,              NULL}           /* sentinel */
};

static PyTypeObject PyFormatterIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "formatteriterator",                /* tp_name */
    sizeof(formatteriterobject),        /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)formatteriter_dealloc,  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    (iternextfunc)formatteriter_next,   /* tp_iternext */
    formatteriter_methods,              /* tp_methods */
    0,
};

/* unicode_formatter_parser is used to implement
   string.Formatter.vformat.  it parses a string and returns tuples
   describing the parsed elements.  It's a wrapper around
   stringlib/string_format.h's MarkupIterator */
static PyObject *
formatter_parser(PyObject *ignored, PyObject *self)
{
    formatteriterobject *it;

    if (!PyUnicode_Check(self)) {
        PyErr_Format(PyExc_TypeError, "expected str, got %s", Py_TYPE(self)->tp_name);
        return NULL;
    }

    if (PyUnicode_READY(self) == -1)
        return NULL;

    it = PyObject_New(formatteriterobject, &PyFormatterIter_Type);
    if (it == NULL)
        return NULL;

    /* take ownership, give the object to the iterator */
    Py_INCREF(self);
    it->str = self;

    /* initialize the contained MarkupIterator */
    MarkupIterator_init(&it->it_markup, (PyObject*)self, 0, PyUnicode_GET_LENGTH(self));
    return (PyObject *)it;
}


/************************************************************************/
/*********** fieldnameiterator ******************************************/
/************************************************************************/


/* This is used to implement string.Formatter.vparse().  It parses the
   field name into attribute and item values.  It's a Python-callable
   wrapper around FieldNameIterator */

typedef struct {
    PyObject_HEAD
    PyObject *str;
    FieldNameIterator it_field;
} fieldnameiterobject;

static void
fieldnameiter_dealloc(fieldnameiterobject *it)
{
    Py_XDECREF(it->str);
    PyObject_FREE(it);
}

/* returns a tuple:
   (is_attr, value)
   is_attr is true if we used attribute syntax (e.g., '.foo')
              false if we used index syntax (e.g., '[foo]')
   value is an integer or string
*/
static PyObject *
fieldnameiter_next(fieldnameiterobject *it)
{
    int result;
    int is_attr;
    Py_ssize_t idx;
    SubString name;

    result = FieldNameIterator_next(&it->it_field, &is_attr,
                                    &idx, &name);
    if (result == 0 || result == 1)
        /* if 0, error has already been set, if 1, iterator is empty */
        return NULL;
    else {
        PyObject* result = NULL;
        PyObject* is_attr_obj = NULL;
        PyObject* obj = NULL;

        is_attr_obj = PyBool_FromLong(is_attr);
        if (is_attr_obj == NULL)
            goto done;

        /* either an integer or a string */
        if (idx != -1)
            obj = PyLong_FromSsize_t(idx);
        else
            obj = SubString_new_object(&name);
        if (obj == NULL)
            goto done;

        /* return a tuple of values */
        result = PyTuple_Pack(2, is_attr_obj, obj);

    done:
        Py_XDECREF(is_attr_obj);
        Py_XDECREF(obj);
        return result;
    }
}

static PyMethodDef fieldnameiter_methods[] = {
    {NULL,              NULL}           /* sentinel */
};

static PyTypeObject PyFieldNameIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "fieldnameiterator",                /* tp_name */
    sizeof(fieldnameiterobject),        /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)fieldnameiter_dealloc,  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    (iternextfunc)fieldnameiter_next,   /* tp_iternext */
    fieldnameiter_methods,              /* tp_methods */
    0};

/* unicode_formatter_field_name_split is used to implement
   string.Formatter.vformat.  it takes an PEP 3101 "field name", and
   returns a tuple of (first, rest): "first", the part before the
   first '.' or '['; and "rest", an iterator for the rest of the field
   name.  it's a wrapper around stringlib/string_format.h's
   field_name_split.  The iterator it returns is a
   FieldNameIterator */
static PyObject *
formatter_field_name_split(PyObject *ignored, PyObject *self)
{
    SubString first;
    Py_ssize_t first_idx;
    fieldnameiterobject *it;

    PyObject *first_obj = NULL;
    PyObject *result = NULL;

    if (!PyUnicode_Check(self)) {
        PyErr_Format(PyExc_TypeError, "expected str, got %s", Py_TYPE(self)->tp_name);
        return NULL;
    }

    if (PyUnicode_READY(self) == -1)
        return NULL;

    it = PyObject_New(fieldnameiterobject, &PyFieldNameIter_Type);
    if (it == NULL)
        return NULL;

    /* take ownership, give the object to the iterator.  this is
       just to keep the field_name alive */
    Py_INCREF(self);
    it->str = self;

    /* Pass in auto_number = NULL. We'll return an empty string for
       first_obj in that case. */
    if (!field_name_split((PyObject*)self, 0, PyUnicode_GET_LENGTH(self),
                          &first, &first_idx, &it->it_field, NULL))
        goto done;

    /* first becomes an integer, if possible; else a string */
    if (first_idx != -1)
        first_obj = PyLong_FromSsize_t(first_idx);
    else
        /* convert "first" into a string object */
        first_obj = SubString_new_object(&first);
    if (first_obj == NULL)
        goto done;

    /* return a tuple of values */
    result = PyTuple_Pack(2, first_obj, it);

done:
    Py_XDECREF(it);
    Py_XDECREF(first_obj);
    return result;
}
