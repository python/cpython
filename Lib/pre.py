# module 're' -- A collection of regular expression operations

"""Support for regular expressions (RE).

This module provides regular expression matching operations similar to
those found in Perl. It's 8-bit clean: the strings being processed may
contain both null bytes and characters whose high bit is set. Regular
expression pattern strings may not contain null bytes, but can specify
the null byte using the \\number notation. Characters with the high
bit set may be included.

Regular expressions can contain both special and ordinary
characters. Most ordinary characters, like "A", "a", or "0", are the
simplest regular expressions; they simply match themselves. You can
concatenate ordinary characters, so last matches the string 'last'.

The special characters are:
    "."      Matches any character except a newline.
    "^"      Matches the start of the string.
    "$"      Matches the end of the string.
    "*"      Matches 0 or more (greedy) repetitions of the preceding RE.
             Greedy means that it will match as many repetitions as possible.
    "+"      Matches 1 or more (greedy) repetitions of the preceding RE.
    "?"      Matches 0 or 1 (greedy) of the preceding RE.
    *?,+?,?? Non-greedy versions of the previous three special characters.
    {m,n}    Matches from m to n repetitions of the preceding RE.
    {m,n}?   Non-greedy version of the above.
    "\\"      Either escapes special characters or signals a special sequence.
    []       Indicates a set of characters.
             A "^" as the first character indicates a complementing set.
    "|"      A|B, creates an RE that will match either A or B.
    (...)    Matches the RE inside the parentheses.
             The contents can be retrieved or matched later in the string.
    (?iLmsx) Set the I, L, M, S, or X flag for the RE.
    (?:...)  Non-grouping version of regular parentheses.
    (?P<name>...) The substring matched by the group is accessible by name.
    (?P=name)     Matches the text matched earlier by the group named name.
    (?#...)  A comment; ignored.
    (?=...)  Matches if ... matches next, but doesn't consume the string.
    (?!...)  Matches if ... doesn't match next.

The special sequences consist of "\\" and a character from the list
below. If the ordinary character is not on the list, then the
resulting RE will match the second character.
    \\number  Matches the contents of the group of the same number.
    \\A       Matches only at the start of the string.
    \\Z       Matches only at the end of the string. 
    \\b       Matches the empty string, but only at the start or end of a word.
    \\B       Matches the empty string, but not at the start or end of a word.
    \\d       Matches any decimal digit; equivalent to the set [0-9].
    \\D       Matches any non-digit character; equivalent to the set [^0-9].
    \\s       Matches any whitespace character; equivalent to [ \\t\\n\\r\\f\\v].
    \\S       Matches any non-whitespace character; equiv. to [^ \\t\\n\\r\\f\\v].
    \\w       Matches any alphanumeric character; equivalent to [a-zA-Z0-9_].
             With LOCALE, it will match the set [0-9_] plus characters defined
             as letters for the current locale.
    \\W       Matches the complement of \\w.
    \\\\       Matches a literal backslash. 

This module exports the following functions:
    match    Match a regular expression pattern to the beginning of a string.
    search   Search a string for the presence of a pattern.
    sub      Substitute occurrences of a pattern found in a string.
    subn     Same as sub, but also return the number of substitutions made.
    split    Split a string by the occurrences of a pattern.
    findall  Find all occurrences of a pattern in a string.
    compile  Compile a pattern into a RegexObject.
    escape   Backslash all non-alphanumerics in a string.

This module exports the following classes:
    RegexObject    Holds a compiled regular expression pattern.
    MatchObject    Contains information about pattern matches.

Some of the functions in this module takes flags as optional parameters:
    I  IGNORECASE  Perform case-insensitive matching.
    L  LOCALE      Make \w, \W, \b, \B, dependent on the current locale.
    M  MULTILINE   "^" matches the beginning of lines as well as the string.
                   "$" matches the end of lines as well as the string.
    S  DOTALL      "." matches any character at all, including the newline.
    X  VERBOSE     Ignore whitespace and comments for nicer looking RE's.

This module also defines an exception 'error'.

"""


import sys
import string
from pcre import *

#
# First, the public part of the interface:
#

# pcre.error and re.error should be the same, since exceptions can be
# raised from either module.

# compilation flags

I = IGNORECASE
L = LOCALE
M = MULTILINE
S = DOTALL 
X = VERBOSE 


#
#
#

_cache = {}
_MAXCACHE = 20

def _cachecompile(pattern, flags=0):
    key = (pattern, flags)
    try:
        return _cache[key]
    except KeyError:
        pass
    value = compile(pattern, flags)
    if len(_cache) >= _MAXCACHE:
        _cache.clear()
    _cache[key] = value
    return value

def match(pattern, string, flags=0):
    """match (pattern, string[, flags]) -> MatchObject or None
    
    If zero or more characters at the beginning of string match the
    regular expression pattern, return a corresponding MatchObject
    instance. Return None if the string does not match the pattern;
    note that this is different from a zero-length match.

    Note: If you want to locate a match anywhere in string, use
    search() instead.

    """
    
    return _cachecompile(pattern, flags).match(string)
  
def search(pattern, string, flags=0):
    """search (pattern, string[, flags]) -> MatchObject or None
    
    Scan through string looking for a location where the regular
    expression pattern produces a match, and return a corresponding
    MatchObject instance. Return None if no position in the string
    matches the pattern; note that this is different from finding a
    zero-length match at some point in the string.

    """
    return _cachecompile(pattern, flags).search(string)
  
def sub(pattern, repl, string, count=0):
    """sub(pattern, repl, string[, count=0]) -> string
    
    Return the string obtained by replacing the leftmost
    non-overlapping occurrences of pattern in string by the
    replacement repl. If the pattern isn't found, string is returned
    unchanged. repl can be a string or a function; if a function, it
    is called for every non-overlapping occurrence of pattern. The
    function takes a single match object argument, and returns the
    replacement string.

    The pattern may be a string or a regex object; if you need to
    specify regular expression flags, you must use a regex object, or
    use embedded modifiers in a pattern; e.g.
    sub("(?i)b+", "x", "bbbb BBBB") returns 'x x'.

    The optional argument count is the maximum number of pattern
    occurrences to be replaced; count must be a non-negative integer,
    and the default value of 0 means to replace all occurrences.

    """
    if type(pattern) == type(''):
        pattern = _cachecompile(pattern)
    return pattern.sub(repl, string, count)

def subn(pattern, repl, string, count=0):
    """subn(pattern, repl, string[, count=0]) -> (string, num substitutions)
    
    Perform the same operation as sub(), but return a tuple
    (new_string, number_of_subs_made).

    """
    if type(pattern) == type(''):
        pattern = _cachecompile(pattern)
    return pattern.subn(repl, string, count)
  
def split(pattern, string, maxsplit=0):
    """split(pattern, string[, maxsplit=0]) -> list of strings
    
    Split string by the occurrences of pattern. If capturing
    parentheses are used in pattern, then the text of all groups in
    the pattern are also returned as part of the resulting list. If
    maxsplit is nonzero, at most maxsplit splits occur, and the
    remainder of the string is returned as the final element of the
    list.

    """
    if type(pattern) == type(''):
        pattern = _cachecompile(pattern)
    return pattern.split(string, maxsplit)

def findall(pattern, string):
    """findall(pattern, string) -> list
    
    Return a list of all non-overlapping matches of pattern in
    string. If one or more groups are present in the pattern, return a
    list of groups; this will be a list of tuples if the pattern has
    more than one group. Empty matches are included in the result.

    """
    if type(pattern) == type(''):
        pattern = _cachecompile(pattern)
    return pattern.findall(string)

def escape(pattern):
    """escape(string) -> string
    
    Return string with all non-alphanumerics backslashed; this is
    useful if you want to match an arbitrary literal string that may
    have regular expression metacharacters in it.

    """
    result = list(pattern)
    alphanum=string.letters+'_'+string.digits
    for i in range(len(pattern)):
        char = pattern[i]
        if char not in alphanum:
            if char=='\000': result[i] = '\\000'
            else: result[i] = '\\'+char
    return string.join(result, '')

def compile(pattern, flags=0):
    """compile(pattern[, flags]) -> RegexObject

    Compile a regular expression pattern into a regular expression
    object, which can be used for matching using its match() and
    search() methods.

    """
    groupindex={}
    code=pcre_compile(pattern, flags, groupindex)
    return RegexObject(pattern, flags, code, groupindex)
    

#
#   Class definitions
#

class RegexObject:
    """Holds a compiled regular expression pattern.

    Methods:
    match    Match the pattern to the beginning of a string.
    search   Search a string for the presence of the pattern.
    sub      Substitute occurrences of the pattern found in a string.
    subn     Same as sub, but also return the number of substitutions made.
    split    Split a string by the occurrences of the pattern.
    findall  Find all occurrences of the pattern in a string.
    
    """

    def __init__(self, pattern, flags, code, groupindex):
        self.code = code 
        self.flags = flags
        self.pattern = pattern
        self.groupindex = groupindex

    def search(self, string, pos=0, endpos=None):
        """search(string[, pos][, endpos]) -> MatchObject or None
        
        Scan through string looking for a location where this regular
        expression produces a match, and return a corresponding
        MatchObject instance. Return None if no position in the string
        matches the pattern; note that this is different from finding
        a zero-length match at some point in the string. The optional
        pos and endpos parameters have the same meaning as for the
        match() method.
    
        """
        if endpos is None or endpos>len(string): 
            endpos=len(string)
        if endpos<pos: endpos=pos
        regs = self.code.match(string, pos, endpos, 0)
        if regs is None:
            return None
        self._num_regs=len(regs)
        
        return MatchObject(self,
                           string,
                           pos, endpos,
                           regs)
    
    def match(self, string, pos=0, endpos=None):
        """match(string[, pos][, endpos]) -> MatchObject or None
        
        If zero or more characters at the beginning of string match
        this regular expression, return a corresponding MatchObject
        instance. Return None if the string does not match the
        pattern; note that this is different from a zero-length match.

        Note: If you want to locate a match anywhere in string, use
        search() instead.

        The optional second parameter pos gives an index in the string
        where the search is to start; it defaults to 0.  This is not
        completely equivalent to slicing the string; the '' pattern
        character matches at the real beginning of the string and at
        positions just after a newline, but not necessarily at the
        index where the search is to start.

        The optional parameter endpos limits how far the string will
        be searched; it will be as if the string is endpos characters
        long, so only the characters from pos to endpos will be
        searched for a match.

        """
        if endpos is None or endpos>len(string): 
            endpos=len(string)
        if endpos<pos: endpos=pos
        regs = self.code.match(string, pos, endpos, ANCHORED)
        if regs is None:
            return None
        self._num_regs=len(regs)
        return MatchObject(self,
                           string,
                           pos, endpos,
                           regs)
    
    def sub(self, repl, string, count=0):
        """sub(repl, string[, count=0]) -> string
        
        Return the string obtained by replacing the leftmost
        non-overlapping occurrences of the compiled pattern in string
        by the replacement repl. If the pattern isn't found, string is
        returned unchanged.

        Identical to the sub() function, using the compiled pattern.
        
        """
        return self.subn(repl, string, count)[0]
    
    def subn(self, repl, source, count=0): 
        """subn(repl, string[, count=0]) -> tuple
        
        Perform the same operation as sub(), but return a tuple
        (new_string, number_of_subs_made).

        """
        if count < 0:
            raise error, "negative substitution count"
        if count == 0:
            count = sys.maxint
        n = 0           # Number of matches
        pos = 0         # Where to start searching
        lastmatch = -1  # End of last match
        results = []    # Substrings making up the result
        end = len(source)

        if type(repl) is type(''):
            # See if repl contains group references
            try:
                repl = pcre_expand(_Dummy, repl)
            except:
                m = MatchObject(self, source, 0, end, [])
                repl = lambda m, repl=repl, expand=pcre_expand: expand(m, repl)
            else:
                m = None
        else:
            m = MatchObject(self, source, 0, end, [])

        match = self.code.match
        append = results.append
        while n < count and pos <= end:
            regs = match(source, pos, end, 0)
            if not regs:
                break
            self._num_regs = len(regs)
            i, j = regs[0]
            if i == j == lastmatch:
                # Empty match adjacent to previous match
                pos = pos + 1
                append(source[lastmatch:pos])
                continue
            if pos < i:
                append(source[pos:i])
            if m:
                m.pos = pos
                m.regs = regs
                append(repl(m))
            else:
                append(repl)
            pos = lastmatch = j
            if i == j:
                # Last match was empty; don't try here again
                pos = pos + 1
                append(source[lastmatch:pos])
            n = n + 1
        append(source[pos:])
        return (string.join(results, ''), n)
                                                                            
    def split(self, source, maxsplit=0):
        """split(source[, maxsplit=0]) -> list of strings
    
        Split string by the occurrences of the compiled pattern. If
        capturing parentheses are used in the pattern, then the text
        of all groups in the pattern are also returned as part of the
        resulting list. If maxsplit is nonzero, at most maxsplit
        splits occur, and the remainder of the string is returned as
        the final element of the list.
        
        """
        if maxsplit < 0:
            raise error, "negative split count"
        if maxsplit == 0:
            maxsplit = sys.maxint
        n = 0
        pos = 0
        lastmatch = 0
        results = []
        end = len(source)
        match = self.code.match
        append = results.append
        while n < maxsplit:
            regs = match(source, pos, end, 0)
            if not regs:
                break
            i, j = regs[0]
            if i == j:
                # Empty match
                if pos >= end:
                    break
                pos = pos+1
                continue
            append(source[lastmatch:i])
            rest = regs[1:]
            if rest:
                for a, b in rest:
                    if a == -1 or b == -1:
                        group = None
                    else:
                        group = source[a:b]
                    append(group)
            pos = lastmatch = j
            n = n + 1
        append(source[lastmatch:])
        return results

    def findall(self, source):
        """findall(source) -> list
    
        Return a list of all non-overlapping matches of the compiled
        pattern in string. If one or more groups are present in the
        pattern, return a list of groups; this will be a list of
        tuples if the pattern has more than one group. Empty matches
        are included in the result.

        """
        pos = 0
        end = len(source)
        results = []
        match = self.code.match
        append = results.append
        while pos <= end:
            regs = match(source, pos, end, 0)
            if not regs:
                break
            i, j = regs[0]
            rest = regs[1:]
            if not rest:
                gr = source[i:j]
            elif len(rest) == 1:
                a, b = rest[0]
                gr = source[a:b]
            else:
                gr = []
                for (a, b) in rest:
                    gr.append(source[a:b])
                gr = tuple(gr)
            append(gr)
            pos = max(j, pos+1)
        return results

    # The following 3 functions were contributed by Mike Fletcher, and
    # allow pickling and unpickling of RegexObject instances.
    def __getinitargs__(self):
        return (None,None,None,None) # any 4 elements, to work around
                                     # problems with the
                                     # pickle/cPickle modules not yet 
                                     # ignoring the __init__ function
    def __getstate__(self):
        return self.pattern, self.flags, self.groupindex
    def __setstate__(self, statetuple):
        self.pattern = statetuple[0]
        self.flags = statetuple[1]
        self.groupindex = statetuple[2]
        self.code = apply(pcre_compile, statetuple)

class _Dummy:
    # Dummy class used by _subn_string().  Has 'group' to avoid core dump.
    group = None

class MatchObject:
    """Holds a compiled regular expression pattern.

    Methods:
    start      Return the index of the start of a matched substring.
    end        Return the index of the end of a matched substring.
    span       Return a tuple of (start, end) of a matched substring.
    groups     Return a tuple of all the subgroups of the match.
    group      Return one or more subgroups of the match.
    groupdict  Return a dictionary of all the named subgroups of the match.

    """

    def __init__(self, re, string, pos, endpos, regs):
        self.re = re
        self.string = string
        self.pos = pos 
        self.endpos = endpos
        self.regs = regs
        
    def start(self, g = 0):
        """start([group=0]) -> int or None
        
        Return the index of the start of the substring matched by
        group; group defaults to zero (meaning the whole matched
        substring). Return -1 if group exists but did not contribute
        to the match.

        """
        if type(g) == type(''):
            try:
                g = self.re.groupindex[g]
            except (KeyError, TypeError):
                raise IndexError, 'group %s is undefined' % `g`
        return self.regs[g][0]
    
    def end(self, g = 0):
        """end([group=0]) -> int or None
        
        Return the indices of the end of the substring matched by
        group; group defaults to zero (meaning the whole matched
        substring). Return -1 if group exists but did not contribute
        to the match.

        """
        if type(g) == type(''):
            try:
                g = self.re.groupindex[g]
            except (KeyError, TypeError):
                raise IndexError, 'group %s is undefined' % `g`
        return self.regs[g][1]
    
    def span(self, g = 0):
        """span([group=0]) -> tuple
        
        Return the 2-tuple (m.start(group), m.end(group)). Note that
        if group did not contribute to the match, this is (-1,
        -1). Group defaults to zero (meaning the whole matched
        substring).

        """
        if type(g) == type(''):
            try:
                g = self.re.groupindex[g]
            except (KeyError, TypeError):
                raise IndexError, 'group %s is undefined' % `g`
        return self.regs[g]
    
    def groups(self, default=None):
        """groups([default=None]) -> tuple
        
        Return a tuple containing all the subgroups of the match, from
        1 up to however many groups are in the pattern. The default
        argument is used for groups that did not participate in the
        match.

        """
        result = []
        for g in range(1, self.re._num_regs):
            a, b = self.regs[g]
            if a == -1 or b == -1:
                result.append(default)
            else:
                result.append(self.string[a:b])
        return tuple(result)

    def group(self, *groups):
        """group([group1, group2, ...]) -> string or tuple
        
        Return one or more subgroups of the match. If there is a
        single argument, the result is a single string; if there are
        multiple arguments, the result is a tuple with one item per
        argument. Without arguments, group1 defaults to zero (i.e. the
        whole match is returned). If a groupN argument is zero, the
        corresponding return value is the entire matching string; if
        it is in the inclusive range [1..99], it is the string
        matching the the corresponding parenthesized group. If a group
        number is negative or larger than the number of groups defined
        in the pattern, an IndexError exception is raised. If a group
        is contained in a part of the pattern that did not match, the
        corresponding result is None. If a group is contained in a
        part of the pattern that matched multiple times, the last
        match is returned.

        If the regular expression uses the (?P<name>...) syntax, the
        groupN arguments may also be strings identifying groups by
        their group name. If a string argument is not used as a group
        name in the pattern, an IndexError exception is raised.

        """
        if len(groups) == 0:
            groups = (0,)
        result = []
        for g in groups:
            if type(g) == type(''):
                try:
                    g = self.re.groupindex[g]
                except (KeyError, TypeError):
                    raise IndexError, 'group %s is undefined' % `g`
            if g >= len(self.regs):
                raise IndexError, 'group %s is undefined' % `g`
            a, b = self.regs[g]
            if a == -1 or b == -1:
                result.append(None)
            else:
                result.append(self.string[a:b])
        if len(result) > 1:
            return tuple(result)
        elif len(result) == 1:
            return result[0]
        else:
            return ()

    def groupdict(self, default=None):
        """groupdict([default=None]) -> dictionary
        
        Return a dictionary containing all the named subgroups of the
        match, keyed by the subgroup name. The default argument is
        used for groups that did not participate in the match.

        """
        dict = {}
        for name, index in self.re.groupindex.items():
            a, b = self.regs[index]
            if a == -1 or b == -1:
                dict[name] = default
            else:
                dict[name] = self.string[a:b]
        return dict
