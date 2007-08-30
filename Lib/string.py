"""A collection of string constants.

Public module variables:

whitespace -- a string containing all characters considered whitespace
lowercase -- a string containing all characters considered lowercase letters
uppercase -- a string containing all characters considered uppercase letters
letters -- a string containing all characters considered letters
digits -- a string containing all characters considered decimal digits
hexdigits -- a string containing all characters considered hexadecimal digits
octdigits -- a string containing all characters considered octal digits
punctuation -- a string containing all characters considered punctuation
printable -- a string containing all characters considered printable

"""

# Some strings for ctype-style character classification
whitespace = ' \t\n\r\v\f'
ascii_lowercase = 'abcdefghijklmnopqrstuvwxyz'
ascii_uppercase = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
ascii_letters = ascii_lowercase + ascii_uppercase
digits = '0123456789'
hexdigits = digits + 'abcdef' + 'ABCDEF'
octdigits = '01234567'
punctuation = """!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~"""
printable = digits + ascii_letters + punctuation + whitespace

# Case conversion helpers
# Use str to convert Unicode literal in case of -U
_idmap = str('').join(chr(c) for c in range(256))

# Functions which aren't available as string methods.

# Capitalize the words in a string, e.g. " aBc  dEf " -> "Abc Def".
def capwords(s, sep=None):
    """capwords(s, [sep]) -> string

    Split the argument into words using split, capitalize each
    word using capitalize, and join the capitalized words using
    join. Note that this replaces runs of whitespace characters by
    a single space.

    """
    return (sep or ' ').join([x.capitalize() for x in s.split(sep)])


# Construct a translation string
_idmapL = None
def maketrans(fromstr, tostr):
    """maketrans(frm, to) -> string

    Return a translation table (a string of 256 bytes long)
    suitable for use in string.translate.  The strings frm and to
    must be of the same length.

    """
    if len(fromstr) != len(tostr):
        raise ValueError("maketrans arguments must have same length")
    global _idmapL
    if not _idmapL:
        _idmapL = list(_idmap)
    L = _idmapL[:]
    for i, c in enumerate(fromstr):
        L[ord(c)] = tostr[i]
    return ''.join(L)



####################################################################
import re as _re

class _multimap:
    """Helper class for combining multiple mappings.

    Used by .{safe_,}substitute() to combine the mapping and keyword
    arguments.
    """
    def __init__(self, primary, secondary):
        self._primary = primary
        self._secondary = secondary

    def __getitem__(self, key):
        try:
            return self._primary[key]
        except KeyError:
            return self._secondary[key]


class _TemplateMetaclass(type):
    pattern = r"""
    %(delim)s(?:
      (?P<escaped>%(delim)s) |   # Escape sequence of two delimiters
      (?P<named>%(id)s)      |   # delimiter and a Python identifier
      {(?P<braced>%(id)s)}   |   # delimiter and a braced identifier
      (?P<invalid>)              # Other ill-formed delimiter exprs
    )
    """

    def __init__(cls, name, bases, dct):
        super(_TemplateMetaclass, cls).__init__(name, bases, dct)
        if 'pattern' in dct:
            pattern = cls.pattern
        else:
            pattern = _TemplateMetaclass.pattern % {
                'delim' : _re.escape(cls.delimiter),
                'id'    : cls.idpattern,
                }
        cls.pattern = _re.compile(pattern, _re.IGNORECASE | _re.VERBOSE)


class Template(metaclass=_TemplateMetaclass):
    """A string class for supporting $-substitutions."""

    delimiter = '$'
    idpattern = r'[_a-z][_a-z0-9]*'

    def __init__(self, template):
        self.template = template

    # Search for $$, $identifier, ${identifier}, and any bare $'s

    def _invalid(self, mo):
        i = mo.start('invalid')
        lines = self.template[:i].splitlines(True)
        if not lines:
            colno = 1
            lineno = 1
        else:
            colno = i - len(''.join(lines[:-1]))
            lineno = len(lines)
        raise ValueError('Invalid placeholder in string: line %d, col %d' %
                         (lineno, colno))

    def substitute(self, *args, **kws):
        if len(args) > 1:
            raise TypeError('Too many positional arguments')
        if not args:
            mapping = kws
        elif kws:
            mapping = _multimap(kws, args[0])
        else:
            mapping = args[0]
        # Helper function for .sub()
        def convert(mo):
            # Check the most common path first.
            named = mo.group('named') or mo.group('braced')
            if named is not None:
                val = mapping[named]
                # We use this idiom instead of str() because the latter will
                # fail if val is a Unicode containing non-ASCII characters.
                return '%s' % (val,)
            if mo.group('escaped') is not None:
                return self.delimiter
            if mo.group('invalid') is not None:
                self._invalid(mo)
            raise ValueError('Unrecognized named group in pattern',
                             self.pattern)
        return self.pattern.sub(convert, self.template)

    def safe_substitute(self, *args, **kws):
        if len(args) > 1:
            raise TypeError('Too many positional arguments')
        if not args:
            mapping = kws
        elif kws:
            mapping = _multimap(kws, args[0])
        else:
            mapping = args[0]
        # Helper function for .sub()
        def convert(mo):
            named = mo.group('named')
            if named is not None:
                try:
                    # We use this idiom instead of str() because the latter
                    # will fail if val is a Unicode containing non-ASCII
                    return '%s' % (mapping[named],)
                except KeyError:
                    return self.delimiter + named
            braced = mo.group('braced')
            if braced is not None:
                try:
                    return '%s' % (mapping[braced],)
                except KeyError:
                    return self.delimiter + '{' + braced + '}'
            if mo.group('escaped') is not None:
                return self.delimiter
            if mo.group('invalid') is not None:
                return self.delimiter
            raise ValueError('Unrecognized named group in pattern',
                             self.pattern)
        return self.pattern.sub(convert, self.template)



########################################################################
# the Formatter class
# see PEP 3101 for details and purpose of this class

# The hard parts are reused from the C implementation.  They're
# exposed here via the sys module.  sys was chosen because it's always
# available and doesn't have to be dynamically loaded.

# The overall parser is implemented in str._formatter_parser.
# The field name parser is implemented in str._formatter_field_name_split

class Formatter:
    def format(self, format_string, *args, **kwargs):
        return self.vformat(format_string, args, kwargs)

    def vformat(self, format_string, args, kwargs):
        used_args = self.get_empty_used_args()
        result = []
        for literal_text, field_name, format_spec, conversion in \
                self.parse(format_string):

            # output the literal text
            if literal_text:
                result.append(literal_text)

            # if there's a field, output it
            if field_name is not None:
                # this is some markup, find the object and do
                #  the formatting

                # given the field_name, find the object it references
                obj = self.get_field(field_name, args, kwargs, used_args)

                # do any conversion on the resulting object
                obj = self.convert_field(obj, conversion)

                # format the object and append to the result
                result.append(self.format_field(obj, format_spec))

        self.check_unused_args(used_args, args, kwargs)
        return ''.join(result)


    def get_empty_used_args(self):
        return set()


    def get_value(self, key, args, kwargs):
        if isinstance(key, int):
            return args[key]
        else:
            return kwargs[key]


    def check_unused_args(self, used_args, args, kwargs):
        pass


    def format_field(self, value, format_spec):
        return format(value, format_spec)


    def convert_field(self, value, conversion):
        # do any conversion on the resulting object
        if conversion == 'r':
            return repr(value)
        elif conversion == 's':
            return str(value)
        else:
            assert conversion is None
            return value


    # returns an iterable that contains tuples of the form:
    # (literal_text, field_name, format_spec, conversion)
    # literal_text can be zero length
    # field_name can be None, in which case there's no
    #  object to format and output
    # if field_name is not None, it is looked up, formatted
    #  with format_spec and conversion and then used
    def parse(self, format_string):
        return format_string._formatter_parser()


    # given a field_name, find the object it references.
    #  field_name:   the field being looked up, e.g. "0.name"
    #                 or "lookup[3]"
    #  used_args:    a set of which args have been used
    #  args, kwargs: as passed in to vformat
    # also, mark it as used in 'used_args'
    def get_field(self, field_name, args, kwargs, used_args):
        first, rest = field_name._formatter_field_name_split()

        used_args.add(first)
        obj = self.get_value(first, args, kwargs)

        # loop through the rest of the field_name, doing
        #  getattr or getitem as needed
        for is_attr, i in rest:
            if is_attr:
                obj = getattr(obj, i)
            else:
                obj = obj[i]

        return obj
