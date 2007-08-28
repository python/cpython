/*
    string_format.h -- implementation of string.format().

    It uses the Objects/stringlib conventions, so that it can be
    compiled for both unicode and string objects.
*/


/* Defines for more efficiently reallocating the string buffer */
#define INITIAL_SIZE_INCREMENT 100
#define SIZE_MULTIPLIER 2
#define MAX_SIZE_INCREMENT  3200


/************************************************************************/
/***********   Global data structures and forward declarations  *********/
/************************************************************************/

/*
   A SubString consists of the characters between two string or
   unicode pointers.
*/
typedef struct {
    STRINGLIB_CHAR *ptr;
    STRINGLIB_CHAR *end;
} SubString;


/* forward declaration for recursion */
static PyObject *
build_string(SubString *input, PyObject *args, PyObject *kwargs,
             int *recursion_level);



/************************************************************************/
/**************************  Utility  functions  ************************/
/************************************************************************/

/* fill in a SubString from a pointer and length */
Py_LOCAL_INLINE(void)
SubString_init(SubString *str, STRINGLIB_CHAR *p, Py_ssize_t len)
{
    str->ptr = p;
    if (p == NULL)
        str->end = NULL;
    else
        str->end = str->ptr + len;
}

Py_LOCAL_INLINE(PyObject *)
SubString_new_object(SubString *str)
{
    return STRINGLIB_NEW(str->ptr, str->end - str->ptr);
}

/************************************************************************/
/***********    Output string management functions       ****************/
/************************************************************************/

typedef struct {
    STRINGLIB_CHAR *ptr;
    STRINGLIB_CHAR *end;
    PyObject *obj;
    Py_ssize_t size_increment;
} OutputString;

/* initialize an OutputString object, reserving size characters */
static int
output_initialize(OutputString *output, Py_ssize_t size)
{
    output->obj = STRINGLIB_NEW(NULL, size);
    if (output->obj == NULL)
        return 0;

    output->ptr = STRINGLIB_STR(output->obj);
    output->end = STRINGLIB_LEN(output->obj) + output->ptr;
    output->size_increment = INITIAL_SIZE_INCREMENT;

    return 1;
}

/*
    output_extend reallocates the output string buffer.
    It returns a status:  0 for a failed reallocation,
    1 for success.
*/

static int
output_extend(OutputString *output, Py_ssize_t count)
{
    STRINGLIB_CHAR *startptr = STRINGLIB_STR(output->obj);
    Py_ssize_t curlen = output->ptr - startptr;
    Py_ssize_t maxlen = curlen + count + output->size_increment;

    if (STRINGLIB_RESIZE(&output->obj, maxlen) < 0)
        return 0;
    startptr = STRINGLIB_STR(output->obj);
    output->ptr = startptr + curlen;
    output->end = startptr + maxlen;
    if (output->size_increment < MAX_SIZE_INCREMENT)
        output->size_increment *= SIZE_MULTIPLIER;
    return 1;
}

/*
    output_data dumps characters into our output string
    buffer.

    In some cases, it has to reallocate the string.

    It returns a status:  0 for a failed reallocation,
    1 for success.
*/
static int
output_data(OutputString *output, const STRINGLIB_CHAR *s, Py_ssize_t count)
{
    if ((count > output->end - output->ptr) && !output_extend(output, count))
        return 0;
    memcpy(output->ptr, s, count * sizeof(STRINGLIB_CHAR));
    output->ptr += count;
    return 1;
}

/************************************************************************/
/***********  Format string parsing -- integers and identifiers *********/
/************************************************************************/

static Py_ssize_t
get_integer(const SubString *str)
{
    Py_ssize_t accumulator = 0;
    Py_ssize_t digitval;
    Py_ssize_t oldaccumulator;
    STRINGLIB_CHAR *p;

    /* empty string is an error */
    if (str->ptr >= str->end)
        return -1;

    for (p = str->ptr; p < str->end; p++) {
        digitval = STRINGLIB_TODECIMAL(*p);
        if (digitval < 0)
            return -1;
        /*
           This trick was copied from old Unicode format code.  It's cute,
           but would really suck on an old machine with a slow divide
           implementation.  Fortunately, in the normal case we do not
           expect too many digits.
        */
        oldaccumulator = accumulator;
        accumulator *= 10;
        if ((accumulator+10)/10 != oldaccumulator+1) {
            PyErr_Format(PyExc_ValueError,
                         "Too many decimal digits in format string");
            return -1;
        }
        accumulator += digitval;
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
    PyObject *idx_obj = PyInt_FromSsize_t(idx);
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

    /* pointer to where we are inside field_name */
    STRINGLIB_CHAR *ptr;
} FieldNameIterator;


static int
FieldNameIterator_init(FieldNameIterator *self, STRINGLIB_CHAR *ptr,
                       Py_ssize_t len)
{
    SubString_init(&self->str, ptr, len);
    self->ptr = self->str.ptr;
    return 1;
}

static int
_FieldNameIterator_attr(FieldNameIterator *self, SubString *name)
{
    STRINGLIB_CHAR c;

    name->ptr = self->ptr;

    /* return everything until '.' or '[' */
    while (self->ptr < self->str.end) {
        switch (c = *self->ptr++) {
        case '[':
        case '.':
            /* backup so that we this character will be seen next time */
            self->ptr--;
            break;
        default:
            continue;
        }
        break;
    }
    /* end of string is okay */
    name->end = self->ptr;
    return 1;
}

static int
_FieldNameIterator_item(FieldNameIterator *self, SubString *name)
{
    STRINGLIB_CHAR c;

    name->ptr = self->ptr;

    /* return everything until ']' */
    while (self->ptr < self->str.end) {
        switch (c = *self->ptr++) {
        case ']':
            break;
        default:
            continue;
        }
        break;
    }
    /* end of string is okay */
    /* don't include the ']' */
    name->end = self->ptr-1;
    return 1;
}

/* returns 0 on error, 1 on non-error termination, and 2 if it returns a value */
static int
FieldNameIterator_next(FieldNameIterator *self, int *is_attribute,
                       Py_ssize_t *name_idx, SubString *name)
{
    /* check at end of input */
    if (self->ptr >= self->str.end)
        return 1;

    switch (*self->ptr++) {
    case '.':
        *is_attribute = 1;
        if (_FieldNameIterator_attr(self, name) == 0) {
            return 0;
        }
        *name_idx = -1;
        break;
    case '[':
        *is_attribute = 0;
        if (_FieldNameIterator_item(self, name) == 0) {
            return 0;
        }
        *name_idx = get_integer(name);
        break;
    default:
        /* interal error, can't get here */
        assert(0);
        return 0;
    }

    /* empty string is an error */
    if (name->ptr == name->end) {
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
field_name_split(STRINGLIB_CHAR *ptr, Py_ssize_t len, SubString *first,
                 Py_ssize_t *first_idx, FieldNameIterator *rest)
{
    STRINGLIB_CHAR c;
    STRINGLIB_CHAR *p = ptr;
    STRINGLIB_CHAR *end = ptr + len;

    /* find the part up until the first '.' or '[' */
    while (p < end) {
        switch (c = *p++) {
        case '[':
        case '.':
            /* backup so that we this character is available to the
               "rest" iterator */
            p--;
            break;
        default:
            continue;
        }
        break;
    }

    /* set up the return values */
    SubString_init(first, ptr, p - ptr);
    FieldNameIterator_init(rest, p, end - p);

    /* see if "first" is an integer, in which case it's used as an index */
    *first_idx = get_integer(first);

    /* zero length string is an error */
    if (first->ptr >= first->end) {
        PyErr_SetString(PyExc_ValueError, "empty field name");
        goto error;
    }

    return 1;
error:
    return 0;
}


/*
    get_field_object returns the object inside {}, before the
    format_spec.  It handles getindex and getattr lookups and consumes
    the entire input string.
*/
static PyObject *
get_field_object(SubString *input, PyObject *args, PyObject *kwargs)
{
    PyObject *obj = NULL;
    int ok;
    int is_attribute;
    SubString name;
    SubString first;
    Py_ssize_t index;
    FieldNameIterator rest;

    if (!field_name_split(input->ptr, input->end - input->ptr, &first,
                          &index, &rest)) {
        goto error;
    }

    if (index == -1) {
        /* look up in kwargs */
        PyObject *key = SubString_new_object(&first);
        if (key == NULL)
            goto error;
        if ((kwargs == NULL) || (obj = PyDict_GetItem(kwargs, key)) == NULL) {
            PyErr_SetString(PyExc_ValueError, "Keyword argument not found "
                            "in format string");
            Py_DECREF(key);
            goto error;
        }
        Py_DECREF(key);
        Py_INCREF(obj);
    }
    else {
        /* look up in args */
        obj = PySequence_GetItem(args, index);
        if (obj == NULL) {
            /* translate IndexError to a ValueError */
            PyErr_SetString(PyExc_ValueError, "Not enough positional arguments "
                            "in format string");
            goto error;
        }
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

    format() does the actual calling of the objects __format__ method.
*/


/* returns fieldobj.__format__(format_spec) */
static PyObject *
format(PyObject *fieldobj, SubString *format_spec)
{
    static PyObject *format_str = NULL;
    PyObject *meth;
    PyObject *spec = NULL;
    PyObject *result = NULL;

    /* Initialize cached value */
    if (format_str == NULL) {
        /* Initialize static variable needed by _PyType_Lookup */
        format_str = PyUnicode_FromString("__format__");
        if (format_str == NULL)
            return NULL;
    }

    /* Make sure the type is initialized.  float gets initialized late */
    if (Py_Type(fieldobj)->tp_dict == NULL)
        if (PyType_Ready(Py_Type(fieldobj)) < 0)
            return NULL;

    /* we need to create an object out of the pointers we have */
    spec = SubString_new_object(format_spec);
    if (spec == NULL)
        goto done;

    /* Find the (unbound!) __format__ method (a borrowed reference) */
    meth = _PyType_Lookup(Py_Type(fieldobj), format_str);
    if (meth == NULL) {
        PyErr_Format(PyExc_TypeError,
                     "Type %.100s doesn't define __format__",
                     Py_Type(fieldobj)->tp_name);
        goto done;
    }

    /* And call it, binding it to the value */
    result = PyObject_CallFunctionObjArgs(meth, fieldobj, spec, NULL);
    if (result == NULL)
        goto done;

    if (!STRINGLIB_CHECK(result)) {
        PyErr_SetString(PyExc_TypeError,
                        "__format__ method did not return "
                        STRINGLIB_TYPE_NAME);
        Py_DECREF(result);
        result = NULL;
        goto done;
    }

done:
    Py_XDECREF(spec);
    return result;
}

/*
    render_field calls fieldobj.__format__(format_spec) method, and
    appends to the output.
*/
static int
render_field(PyObject *fieldobj, SubString *format_spec, OutputString *output)
{
    int ok = 0;
    PyObject *result = format(fieldobj, format_spec);

    if (result == NULL)
        goto done;

    ok = output_data(output,
                     STRINGLIB_STR(result), STRINGLIB_LEN(result));
done:
    Py_XDECREF(result);
    return ok;
}

static int
parse_field(SubString *str, SubString *field_name, SubString *format_spec,
            STRINGLIB_CHAR *conversion)
{
    STRINGLIB_CHAR c = 0;

    /* initialize these, as they may be empty */
    *conversion = '\0';
    SubString_init(format_spec, NULL, 0);

    /* search for the field name.  it's terminated by the end of the
       string, or a ':' or '!' */
    field_name->ptr = str->ptr;
    while (str->ptr < str->end) {
        switch (c = *(str->ptr++)) {
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
        field_name->end = str->ptr-1;

        /* the format specifier is the rest of the string */
        format_spec->ptr = str->ptr;
        format_spec->end = str->end;

        /* see if there's a conversion specifier */
        if (c == '!') {
            /* there must be another character present */
            if (format_spec->ptr >= format_spec->end) {
                PyErr_SetString(PyExc_ValueError,
                                "end of format while looking for conversion "
                                "specifier");
                return 0;
            }
            *conversion = *(format_spec->ptr++);

            /* if there is another character, it must be a colon */
            if (format_spec->ptr < format_spec->end) {
                c = *(format_spec->ptr++);
                if (c != ':') {
                    PyErr_SetString(PyExc_ValueError,
                                    "expected ':' after format specifier");
                    return 0;
                }
            }
        }

        return 1;

    }
    else {
        /* end of string, there's no format_spec or conversion */
        field_name->end = str->ptr;
        return 1;
    }
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
    int in_markup;
} MarkupIterator;

static int
MarkupIterator_init(MarkupIterator *self, STRINGLIB_CHAR *ptr, Py_ssize_t len)
{
    SubString_init(&self->str, ptr, len);
    self->in_markup = 0;
    return 1;
}

/* returns 0 on error, 1 on non-error termination, and 2 if it got a
   string (or something to be expanded) */
static int
MarkupIterator_next(MarkupIterator *self, int *is_markup, SubString *literal,
                    SubString *field_name, SubString *format_spec,
                    STRINGLIB_CHAR *conversion,
                    int *format_spec_needs_expanding)
{
    int at_end;
    STRINGLIB_CHAR c = 0;
    STRINGLIB_CHAR *start;
    int count;
    Py_ssize_t len;

    *format_spec_needs_expanding = 0;

    /* no more input, end of iterator */
    if (self->str.ptr >= self->str.end)
        return 1;

    *is_markup = self->in_markup;
    start = self->str.ptr;

    if (self->in_markup) {

        /* prepare for next iteration */
        self->in_markup = 0;

        /* this is markup, find the end of the string by counting nested
           braces.  note that this prohibits escaped braces, so that
           format_specs cannot have braces in them. */
        count = 1;

        /* we know we can't have a zero length string, so don't worry
           about that case */
        while (self->str.ptr < self->str.end) {
            switch (c = *(self->str.ptr++)) {
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
                    literal->ptr = start;
                    literal->end = self->str.ptr-1;

                    if (parse_field(literal, field_name, format_spec,
                                    conversion) == 0)
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
    else {
        /* literal text, read until the end of string, an escaped { or },
           or an unescaped { */
        while (self->str.ptr < self->str.end) {
            switch (c = *(self->str.ptr++)) {
            case '{':
            case '}':
                self->in_markup = 1;
                break;
            default:
                continue;
            }
            break;
        }

        at_end = self->str.ptr >= self->str.end;
        len = self->str.ptr - start;

        if ((c == '}') && (at_end || (c != *self->str.ptr))) {
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
            if (c == *self->str.ptr) {
                /* escaped } or {, skip it in the input */
                self->str.ptr++;
                self->in_markup = 0;
            }
            else
                len--;
        }

        /* this is just plain text, return it */
        literal->ptr = start;
        literal->end = start + len;
        return 2;
    }
}


/* do the !r or !s conversion on obj */
static PyObject *
do_conversion(PyObject *obj, STRINGLIB_CHAR conversion)
{
    /* XXX in pre-3.0, do we need to convert this to unicode, since it
       might have returned a string? */
    switch (conversion) {
    case 'r':
        return PyObject_Repr(obj);
    case 's':
        return PyObject_Unicode(obj);
    default:
        PyErr_Format(PyExc_ValueError,
                     "Unknown converion specifier %c",
                     conversion);
        return NULL;
    }
}

/* given:

   {field_name!conversion:format_spec}

   compute the result and write it to output.
   format_spec_needs_expanding is an optimization.  if it's false,
   just output the string directly, otherwise recursively expand the
   format_spec string. */

static int
output_markup(SubString *field_name, SubString *format_spec,
              int format_spec_needs_expanding, STRINGLIB_CHAR conversion,
              OutputString *output, PyObject *args, PyObject *kwargs,
              int *recursion_level)
{
    PyObject *tmp = NULL;
    PyObject *fieldobj = NULL;
    SubString expanded_format_spec;
    SubString *actual_format_spec;
    int result = 0;

    /* convert field_name to an object */
    fieldobj = get_field_object(field_name, args, kwargs);
    if (fieldobj == NULL)
        goto done;

    if (conversion != '\0') {
        tmp = do_conversion(fieldobj, conversion);
        if (tmp == NULL)
            goto done;

        /* do the assignment, transferring ownership: fieldobj = tmp */
        Py_DECREF(fieldobj);
        fieldobj = tmp;
        tmp = NULL;
    }

    /* if needed, recurively compute the format_spec */
    if (format_spec_needs_expanding) {
        tmp = build_string(format_spec, args, kwargs, recursion_level);
        if (tmp == NULL)
            goto done;

        /* note that in the case we're expanding the format string,
           tmp must be kept around until after the call to
           render_field. */
        SubString_init(&expanded_format_spec,
                       STRINGLIB_STR(tmp), STRINGLIB_LEN(tmp));
        actual_format_spec = &expanded_format_spec;
    }
    else
        actual_format_spec = format_spec;

    if (render_field(fieldobj, actual_format_spec, output) == 0)
        goto done;

    result = 1;

done:
    Py_XDECREF(fieldobj);
    Py_XDECREF(tmp);

    return result;
}

/*
    do_markup is the top-level loop for the format() function.  It
    searches through the format string for escapes to markup codes, and
    calls other functions to move non-markup text to the output,
    and to perform the markup to the output.
*/
static int
do_markup(SubString *input, PyObject *args, PyObject *kwargs,
          OutputString *output, int *recursion_level)
{
    MarkupIterator iter;
    int is_markup;
    int format_spec_needs_expanding;
    int result;
    SubString str;
    SubString field_name;
    SubString format_spec;
    STRINGLIB_CHAR conversion;

    MarkupIterator_init(&iter, input->ptr, input->end - input->ptr);
    while ((result = MarkupIterator_next(&iter, &is_markup, &str, &field_name,
                                         &format_spec, &conversion,
                                         &format_spec_needs_expanding)) == 2) {
        if (is_markup) {
            if (!output_markup(&field_name, &format_spec,
                               format_spec_needs_expanding, conversion, output,
                               args, kwargs, recursion_level))
                return 0;
        }
        else
            if (!output_data(output, str.ptr, str.end-str.ptr))
                return 0;
    }
    return result;
}


/*
    build_string allocates the output string and then
    calls do_markup to do the heavy lifting.
*/
static PyObject *
build_string(SubString *input, PyObject *args, PyObject *kwargs,
             int *recursion_level)
{
    OutputString output;
    PyObject *result = NULL;
    Py_ssize_t count;

    output.obj = NULL; /* needed so cleanup code always works */

    /* check the recursion level */
    (*recursion_level)--;
    if (*recursion_level < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "Max string recursion exceeded");
        goto done;
    }

    /* initial size is the length of the format string, plus the size
       increment.  seems like a reasonable default */
    if (!output_initialize(&output,
                           input->end - input->ptr +
                           INITIAL_SIZE_INCREMENT))
        goto done;

    if (!do_markup(input, args, kwargs, &output, recursion_level)) {
        goto done;
    }

    count = output.ptr - STRINGLIB_STR(output.obj);
    if (STRINGLIB_RESIZE(&output.obj, count) < 0) {
        goto done;
    }

    /* transfer ownership to result */
    result = output.obj;
    output.obj = NULL;

done:
    (*recursion_level)++;
    Py_XDECREF(output.obj);
    return result;
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
    int recursion_level = 2;

    SubString_init(&input, STRINGLIB_STR(self), STRINGLIB_LEN(self));
    return build_string(&input, args, kwargs, &recursion_level);
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

    PyUnicodeObject *str;

    MarkupIterator it_markup;
} formatteriterobject;

static void
formatteriter_dealloc(formatteriterobject *it)
{
    Py_XDECREF(it->str);
    PyObject_FREE(it);
}

/* returns a tuple:
   (is_markup, literal, field_name, format_spec, conversion)
   if is_markup == True:
        literal is None
        field_name is the string before the ':'
        format_spec is the string after the ':'
        conversion is either None, or the string after the '!'
   if is_markup == False:
        literal is the literal string
        field_name is None
        format_spec is None
        conversion is None
*/
static PyObject *
formatteriter_next(formatteriterobject *it)
{
    SubString literal;
    SubString field_name;
    SubString format_spec;
    Py_UNICODE conversion;
    int is_markup;
    int format_spec_needs_expanding;
    int result = MarkupIterator_next(&it->it_markup, &is_markup, &literal,
                                     &field_name, &format_spec, &conversion,
                                     &format_spec_needs_expanding);

    /* all of the SubString objects point into it->str, so no
       memory management needs to be done on them */
    assert(0 <= result && result <= 2);
    if (result == 0 || result == 1)
        /* if 0, error has already been set, if 1, iterator is empty */
        return NULL;
    else {
        PyObject *is_markup_bool = NULL;
        PyObject *literal_str = NULL;
        PyObject *field_name_str = NULL;
        PyObject *format_spec_str = NULL;
        PyObject *conversion_str = NULL;
        PyObject *tuple = NULL;

        is_markup_bool = PyBool_FromLong(is_markup);
        if (!is_markup_bool)
            return NULL;

        if (is_markup) {
            /* field_name, format_spec, and conversion are returned */
            literal_str = Py_None;
            Py_INCREF(literal_str);

            field_name_str = SubString_new_object(&field_name);
            if (field_name_str == NULL)
                goto error;

            format_spec_str = SubString_new_object(&format_spec);
            if (format_spec_str == NULL)
                goto error;

            /* if the conversion is not specified, return a None,
               otherwise create a one length string with the
               conversion characater */
            if (conversion == '\0') {
                conversion_str = Py_None;
                Py_INCREF(conversion_str);
            }
            else
                conversion_str = PyUnicode_FromUnicode(&conversion,
                                                       1);
            if (conversion_str == NULL)
                goto error;
        }
        else {
            /* only literal is returned */
            literal_str = SubString_new_object(&literal);
            if (literal_str == NULL)
                goto error;

            field_name_str = Py_None;
            format_spec_str = Py_None;
            conversion_str = Py_None;

            Py_INCREF(field_name_str);
            Py_INCREF(format_spec_str);
            Py_INCREF(conversion_str);
        }
        tuple = PyTuple_Pack(5, is_markup_bool, literal_str,
                             field_name_str, format_spec_str,
                             conversion_str);
    error:
        Py_XDECREF(is_markup_bool);
        Py_XDECREF(literal_str);
        Py_XDECREF(field_name_str);
        Py_XDECREF(format_spec_str);
        Py_XDECREF(conversion_str);
        return tuple;
    }
}

static PyMethodDef formatteriter_methods[] = {
    {NULL,		NULL}		/* sentinel */
};

PyTypeObject PyFormatterIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "formatteriterator",		/* tp_name */
    sizeof(formatteriterobject),	/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)formatteriter_dealloc,	/* tp_dealloc */
    0,					/* tp_print */
    0,					/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
    0,					/* tp_call */
    0,					/* tp_str */
    PyObject_GenericGetAttr,		/* tp_getattro */
    0,					/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,			/* tp_flags */
    0,					/* tp_doc */
    0,					/* tp_traverse */
    0,					/* tp_clear */
    0,					/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    PyObject_SelfIter,			/* tp_iter */
    (iternextfunc)formatteriter_next,	/* tp_iternext */
    formatteriter_methods,		/* tp_methods */
    0,
};

/* unicode_formatter_parser is used to implement
   string.Formatter.vformat.  it parses a string and returns tuples
   describing the parsed elements.  It's a wrapper around
   stringlib/string_format.h's MarkupIterator */
static PyObject *
formatter_parser(PyUnicodeObject *self)
{
    formatteriterobject *it;

    it = PyObject_New(formatteriterobject, &PyFormatterIter_Type);
    if (it == NULL)
        return NULL;

    /* take ownership, give the object to the iterator */
    Py_INCREF(self);
    it->str = self;

    /* initialize the contained MarkupIterator */
    MarkupIterator_init(&it->it_markup,
                        PyUnicode_AS_UNICODE(self),
                        PyUnicode_GET_SIZE(self));

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

    PyUnicodeObject *str;

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
            goto error;

        /* either an integer or a string */
        if (idx != -1)
            obj = PyInt_FromSsize_t(idx);
        else
            obj = SubString_new_object(&name);
        if (obj == NULL)
            goto error;

        /* return a tuple of values */
        result = PyTuple_Pack(2, is_attr_obj, obj);
        if (result == NULL)
            goto error;

        return result;

    error:
        Py_XDECREF(result);
        Py_XDECREF(is_attr_obj);
        Py_XDECREF(obj);
        return NULL;
    }
    return NULL;
}

static PyMethodDef fieldnameiter_methods[] = {
    {NULL,		NULL}		/* sentinel */
};

static PyTypeObject PyFieldNameIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "fieldnameiterator",		/* tp_name */
    sizeof(fieldnameiterobject),	/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)fieldnameiter_dealloc,	/* tp_dealloc */
    0,					/* tp_print */
    0,					/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
    0,					/* tp_call */
    0,					/* tp_str */
    PyObject_GenericGetAttr,		/* tp_getattro */
    0,					/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,			/* tp_flags */
    0,					/* tp_doc */
    0,					/* tp_traverse */
    0,					/* tp_clear */
    0,					/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    PyObject_SelfIter,			/* tp_iter */
    (iternextfunc)fieldnameiter_next,	/* tp_iternext */
    fieldnameiter_methods,		/* tp_methods */
    0};

/* unicode_formatter_field_name_split is used to implement
   string.Formatter.vformat.  it takes an PEP 3101 "field name", and
   returns a tuple of (first, rest): "first", the part before the
   first '.' or '['; and "rest", an iterator for the rest of the field
   name.  it's a wrapper around stringlib/string_format.h's
   field_name_split.  The iterator it returns is a
   FieldNameIterator */
static PyObject *
formatter_field_name_split(PyUnicodeObject *self)
{
    SubString first;
    Py_ssize_t first_idx;
    fieldnameiterobject *it;

    PyObject *first_obj = NULL;
    PyObject *result = NULL;

    it = PyObject_New(fieldnameiterobject, &PyFieldNameIter_Type);
    if (it == NULL)
        return NULL;

    /* take ownership, give the object to the iterator.  this is
       just to keep the field_name alive */
    Py_INCREF(self);
    it->str = self;

    if (!field_name_split(STRINGLIB_STR(self),
                          STRINGLIB_LEN(self),
                          &first, &first_idx, &it->it_field))
        goto error;

    /* first becomes an integer, if possible; else a string */
    if (first_idx != -1)
        first_obj = PyInt_FromSsize_t(first_idx);
    else
        /* convert "first" into a string object */
        first_obj = SubString_new_object(&first);
    if (first_obj == NULL)
        goto error;

    /* return a tuple of values */
    result = PyTuple_Pack(2, first_obj, it);

error:
    Py_XDECREF(it);
    Py_XDECREF(first_obj);
    return result;
}
