"""Configuration file parser.

A setup file consists of sections, lead by a "[section]" header,
and followed by "name: value" entries, with continuations and such in
the style of RFC 822.

The option values can contain format strings which refer to other values in
the same section, or values in a special [DEFAULT] section.

For example:

    something: %(dir)s/whatever

would resolve the "%(dir)s" to the value of dir.  All reference
expansions are done late, on demand.

Intrinsic defaults can be specified by passing them into the
ConfigParser constructor as a dictionary.

class:

ConfigParser -- responsible for for parsing a list of
                configuration files, and managing the parsed database.

    methods:

    __init__(defaults=None)
        create the parser and specify a dictionary of intrinsic defaults.  The
        keys must be strings, the values must be appropriate for %()s string
        interpolation.  Note that `__name__' is always an intrinsic default;
        it's value is the section's name.

    sections()
        return all the configuration section names, sans DEFAULT

    options(section)
        return list of configuration options for the named section

    read(*filenames)
        read and parse the list of named configuration files

    get(section, option, raw=0, vars=None)
        return a string value for the named option.  All % interpolations are
        expanded in the return values, based on the defaults passed into the
        constructor and the DEFAULT section.  Additional substitutions may be
        provided using the `vars' argument, which must be a dictionary whose
        contents override any pre-existing defaults.

    getint(section, options)
        like get(), but convert value to an integer

    getfloat(section, options)
        like get(), but convert value to a float

    getboolean(section, options)
        like get(), but convert value to a boolean (currently defined as 0 or
        1, only)
"""

import sys
import string
import re

DEFAULTSECT = "DEFAULT"



# exception classes
class Error:
    def __init__(self, msg=''):
        self._msg = msg
    def __repr__(self):
        return self._msg

class NoSectionError(Error):
    def __init__(self, section):
        Error.__init__(self, 'No section: %s' % section)
        self.section = section

class DuplicateSectionError(Error):
    def __init__(self, section):
        Error.__init__(self, "Section %s already exists" % section)
        self.section = section

class NoOptionError(Error):
    def __init__(self, option, section):
        Error.__init__(self, "No option `%s' in section: %s" %
                       (option, section))
        self.option = option
        self.section = section

class InterpolationError(Error):
    def __init__(self, reference, option, section, rawval):
        Error.__init__(self,
                       "Bad value substitution:\n"
                       "\tsection: [%s]\n"
                       "\toption : %s\n"
                       "\tkey    : %s\n"
                       "\trawval : %s\n"
                       % (section, option, reference, rawval))
        self.reference = reference
        self.option = option
        self.section = section

class MissingSectionHeaderError(Error):
    def __init__(self, filename, lineno, line):
        Error.__init__(
            self,
            'File contains no section headers.\nfile: %s, line: %d\n%s' %
            (filename, lineno, line))
        self.filename = filename
        self.lineno = lineno
        self.line = line

class ParsingError(Error):
    def __init__(self, filename):
        Error.__init__(self, 'File contains parsing errors: %s' % filename)
        self.filename = filename
        self.errors = []

    def append(self, lineno, line):
        self.errors.append((lineno, line))
        self._msg = self._msg + '\n\t[line %2d]: %s' % (lineno, line)



class ConfigParser:
    def __init__(self, defaults=None):
        self.__sections = {}
        if defaults is None:
            self.__defaults = {}
        else:
            self.__defaults = defaults

    def defaults(self):
        return self.__defaults

    def sections(self):
        """Return a list of section names, excluding [DEFAULT]"""
        # self.__sections will never have [DEFAULT] in it
        return self.__sections.keys()

    def add_section(self, section):
        """Create a new section in the configuration.

        Raise DuplicateSectionError if a section by the specified name
        already exists.
        """
        if self.__sections.has_key(section):
            raise DuplicateSectionError(section)
        self.__sections[section] = {}

    def has_section(self, section):
        """Indicate whether the named section is present in the configuration.

        The DEFAULT section is not acknowledged.
        """
        return self.__sections.has_key(section)

    def options(self, section):
        try:
            opts = self.__sections[section].copy()
        except KeyError:
            raise NoSectionError(section)
        opts.update(self.__defaults)
        return opts.keys()

    def read(self, filenames):
        """Read and parse a list of filenames."""
        if type(filenames) is type(''):
            filenames = [filenames]
        for file in filenames:
            try:
                fp = open(file, 'r')
                self.__read(fp)
            except IOError:
                pass

    def get(self, section, option, raw=0, vars=None):
        """Get an option value for a given section.

        All % interpolations are expanded in the return values, based on the
        defaults passed into the constructor, unless the optional argument
        `raw' is true.  Additional substitutions may be provided using the
        `vars' argument, which must be a dictionary whose contents overrides
        any pre-existing defaults.

        The section DEFAULT is special.
        """
        try:
            sectdict = self.__sections[section].copy()
        except KeyError:
            if section == DEFAULTSECT:
                sectdict = {}
            else:
                raise NoSectionError(section)
        d = self.__defaults.copy()
        d.update(sectdict)
        # Update with the entry specific variables
        if vars:
            d.update(vars)
        option = string.lower(option)
        try:
            rawval = d[option]
        except KeyError:
            raise NoOptionError(option, section)
        # do the string interpolation
        if raw:
            return rawval

        value = rawval                  # Make it a pretty variable name
        while 1:                        # Loop through this until it's done
            if not string.find(value, "%("):
                try:
                    value = value % d
                except KeyError, key:
                    raise InterpolationError(key, option, section, rawval)
            else:
                return value
    
    def __get(self, section, conv, option):
        return conv(self.get(section, option))

    def getint(self, section, option):
        return self.__get(section, string.atoi, option)

    def getfloat(self, section, option):
        return self.__get(section, string.atof, option)

    def getboolean(self, section, option):
        v = self.get(section, option)
        val = string.atoi(v)
        if val not in (0, 1):
            raise ValueError, 'Not a boolean: %s' % v
        return val

    #
    # Regular expressions for parsing section headers and options.  Note a
    # slight semantic change from the previous version, because of the use
    # of \w, _ is allowed in section header names.
    __SECTCRE = re.compile(
        r'\['                                 # [
        r'(?P<header>[-\w]+)'                 # `-', `_' or any alphanum
        r'\]'                                 # ]
        )
    __OPTCRE = re.compile(
        r'(?P<option>[-.\w]+)'                # - . _ alphanum
        r'[ \t]*[:=][ \t]*'                   # any number of space/tab,
                                              # followed by separator
                                              # (either : or =), followed
                                              # by any # space/tab
        r'(?P<value>.*)$'                     # everything up to eol
        )

    def __read(self, fp):
        """Parse a sectioned setup file.

        The sections in setup file contains a title line at the top,
        indicated by a name in square brackets (`[]'), plus key/value
        options lines, indicated by `name: value' format lines.
        Continuation are represented by an embedded newline then
        leading whitespace.  Blank lines, lines beginning with a '#',
        and just about everything else is ignored.
        """
        cursect = None                            # None, or a dictionary
        optname = None
        lineno = 0
        e = None                                  # None, or an exception
        while 1:
            line = fp.readline()
            if not line:
                break
            lineno = lineno + 1
            # comment or blank line?
            if string.strip(line) == '' or line[0] in '#;':
                continue
            if string.lower(string.split(line)[0]) == 'rem' \
               and line[0] == "r":      # no leading whitespace
                continue
            # continuation line?
            if line[0] in ' \t' and cursect is not None and optname:
                value = string.strip(line)
                if value:
                    cursect[optname] = cursect[optname] + '\n ' + value
            # a section header or option header?
            else:
                # is it a section header?
                mo = self.__SECTCRE.match(line)
                if mo:
                    sectname = mo.group('header')
                    if self.__sections.has_key(sectname):
                        cursect = self.__sections[sectname]
                    elif sectname == DEFAULTSECT:
                        cursect = self.__defaults
                    else:
                        cursect = {'__name__': sectname}
                        self.__sections[sectname] = cursect
                    # So sections can't start with a continuation line
                    optname = None
                # no section header in the file?
                elif cursect is None:
                    raise MissingSectionHeaderError(fp.name, lineno, `line`)
                # an option line?
                else:
                    mo = self.__OPTCRE.match(line)
                    if mo:
                        optname, optval = mo.group('option', 'value')
                        optname = string.lower(optname)
                        optval = string.strip(optval)
                        # allow empty values
                        if optval == '""':
                            optval = ''
                        cursect[optname] = optval
                    else:
                        # a non-fatal parsing error occurred.  set up the
                        # exception but keep going. the exception will be
                        # raised at the end of the file and will contain a
                        # list of all bogus lines
                        if not e:
                            e = ParsingError(fp.name)
                        e.append(lineno, `line`)
        # if any parsing errors occurred, raise an exception
        if e:
            raise e
