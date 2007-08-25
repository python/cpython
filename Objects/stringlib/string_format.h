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
/***********      Error handling and exception generation  **************/
/************************************************************************/

/*
    Most of our errors are value errors, because to Python, the
    format string is a "value".  Also, it's convenient to return
    a NULL when we are erroring out.

    XXX: need better error handling, per PEP 3101.
*/
static void *
SetError(const char *s)
{
    /* PyErr_Format always returns NULL */
    return PyErr_Format(PyExc_ValueError, "%s in format string", s);
}

/*
    check_input returns True if we still have characters
    left in the input string.

    XXX: make this function go away when better error handling is
    implemented.
*/
Py_LOCAL_INLINE(int)
check_input(SubString *input)
{
    if (input->ptr < input->end)
        return 1;
    PyErr_SetString(PyExc_ValueError,
                    "unterminated replacement field");
    return 0;
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

/*
    end_identifier returns true if a character marks
    the end of an identifier string.

    Although the PEP specifies that identifiers are
    numbers or valid Python identifiers, we just let
    getattr/getitem handle that, so the implementation
    is more flexible than the PEP would indicate.
*/
Py_LOCAL_INLINE(int)
end_identifier(STRINGLIB_CHAR c)
{
    switch (c) {
    case '.': case '[': case ']':
        return 1;
    default:
        return 0;
    }
}

/*
    get_integer consumes 0 or more decimal digit characters from an
    input string, updates *result with the corresponding positive
    integer, and returns the number of digits consumed.

    returns -1 on error.
*/
static int
get_integer(STRINGLIB_CHAR **ptr, STRINGLIB_CHAR *end,
                  Py_ssize_t *result)
{
    Py_ssize_t accumulator, digitval, oldaccumulator;
    int numdigits;
    accumulator = numdigits = 0;
    for (;;(*ptr)++, numdigits++) {
        if (*ptr >= end)
            break;
        digitval = STRINGLIB_TODECIMAL(**ptr);
        if (digitval < 0)
            break;
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
    *result = accumulator;
    return numdigits;
}

/*
    get_identifier is a bit of a misnomer.  It returns a value for use
    with getattr or getindex.  This value will a string/unicode
    object. The input cannot be zero length.  Continues until end of
    input, or end_identifier() returns true.
*/
static PyObject *
get_identifier(SubString *input)
{
    STRINGLIB_CHAR *start;

    for (start = input->ptr;
         input->ptr < input->end && !end_identifier(*input->ptr);
         input->ptr++)
        ;

    return  STRINGLIB_NEW(start, input->ptr - start);

    /*
        We might want to add code here to check for invalid Python
        identifiers.  All identifiers are eventually passed to getattr
        or getitem, so there is a check when used.  However, we might
        want to remove (or not) the ability to have strings like
        "a/b" or " ab" or "-1" (which is not parsed as a number).
        For now, this is left as an exercise for the first disgruntled
        user...

    if (XXX -- need check function) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_ValueError,
                      "Invalid embedded Python identifier");
        return NULL;
    }
    */
}

/************************************************************************/
/******** Functions to get field objects and specification strings ******/
/************************************************************************/

/* get_field_and_spec is the main function in this section.  It parses
   the format string well enough to return a field object to render along
   with a field specification string.
*/

/*
    look up key in our keyword arguments
*/
static PyObject *
key_lookup(PyObject *kwargs, PyObject *key)
{
    PyObject *result;

    if (kwargs && (result = PyDict_GetItem(kwargs, key)) != NULL) {
        Py_INCREF(result);
        return result;
    }
    return NULL;
}

/*
    get_field_object returns the object inside {}, before the
    format_spec.  It handles getindex and getattr lookups and consumes
    the entire input string.
*/
static PyObject *
get_field_object(SubString *input, PyObject *args, PyObject *kwargs)
{
    PyObject *myobj, *subobj, *newobj;
    STRINGLIB_CHAR c;
    Py_ssize_t index;
    int isindex, isnumeric, isargument;

    index = isnumeric = 0;  /* Just to shut up the compiler warnings */

    myobj = args;
    Py_INCREF(myobj);

    for (isindex=1, isargument=1;;) {
        if (!check_input(input))
            break;
        if (!isindex) {
            if ((subobj = get_identifier(input)) == NULL)
                break;
            newobj = PyObject_GetAttr(myobj, subobj);
            Py_DECREF(subobj);
        } else {
            isnumeric = (STRINGLIB_ISDECIMAL(*input->ptr));
            if (isnumeric)
                /* XXX: add error checking */
                get_integer(&input->ptr, input->end, &index);

            if (isnumeric && PySequence_Check(myobj))
                newobj = PySequence_GetItem(myobj, index);
            else {
                /* XXX -- do we need PyLong_FromLongLong?
                                   Using ssizet, not int... */
                subobj = isnumeric ?
                          PyInt_FromLong(index) :
                          get_identifier(input);
                if (subobj == NULL)
                    break;
                if (isargument) {
                    newobj = key_lookup(kwargs, subobj);
                } else {
                    newobj = PyObject_GetItem(myobj, subobj);
                }
                Py_DECREF(subobj);
            }
        }
        Py_DECREF(myobj);
        myobj = newobj;
        if (myobj == NULL)
            break;
        if (!isargument && isindex)
            if  ((!check_input(input)) || (*(input->ptr++) != ']')) {
                SetError("Expected ]");
                break;
            }

        /* if at the end of input, return with myobj */
        if (input->ptr >= input->end)
            return myobj;

        c = *input->ptr;
        input->ptr++;
        isargument = 0;
        isindex = (c == '[');
        if (!isindex && (c != '.')) {
           SetError("Expected ., [, :, !, or }");
           break;
        }
    }
    if ((myobj == NULL) && isargument) {
        /* XXX: include more useful error information, like which
         * keyword not found or which index missing */
       PyErr_Clear();
       return SetError(isnumeric
            ? "Not enough positional arguments"
            : "Keyword argument not found");
    }
    Py_XDECREF(myobj);
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

    } else {
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

    } else {
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
            SetError("Single } encountered");
            return 0;
        }
        if (at_end && c == '{') {
            SetError("Single { encountered");
            return 0;
        }
        if (!at_end) {
            if (c == *self->str.ptr) {
                /* escaped } or {, skip it in the input */
                self->str.ptr++;
                self->in_markup = 0;
            } else
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
    } else
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
        } else {
            if (!output_data(output, str.ptr, str.end-str.ptr))
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
