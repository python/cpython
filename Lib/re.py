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
    return _cachecompile(pattern, flags).match(string)
  
def search(pattern, string, flags=0):
    return _cachecompile(pattern, flags).search(string)
  
def sub(pattern, repl, string, count=0):
    if type(pattern) == type(''):
        pattern = _cachecompile(pattern)
    return pattern.sub(repl, string, count)

def subn(pattern, repl, string, count=0):
    if type(pattern) == type(''):
        pattern = _cachecompile(pattern)
    return pattern.subn(repl, string, count)
  
def split(pattern, string, maxsplit=0):
    if type(pattern) == type(''):
        pattern = _cachecompile(pattern)
    return pattern.split(string, maxsplit)

def findall(pattern, string):
    if type(pattern) == type(''):
        pattern = _cachecompile(pattern)
    return pattern.findall(string)

def escape(pattern):
    "Escape all non-alphanumeric characters in pattern."
    result = []
    alphanum=string.letters+'_'+string.digits
    for char in pattern:
        if char not in alphanum:
            if char=='\000': result.append('\\000')
            else: result.append('\\'+char)
        else: result.append(char)
    return string.join(result, '')

def compile(pattern, flags=0):
    "Compile a regular expression pattern, returning a RegexObject."
    groupindex={}
    code=pcre_compile(pattern, flags, groupindex)
    return RegexObject(pattern, flags, code, groupindex)
    

#
#   Class definitions
#

class RegexObject:

    def __init__(self, pattern, flags, code, groupindex):
        self.code = code 
        self.flags = flags
        self.pattern = pattern
        self.groupindex = groupindex

    def search(self, string, pos=0, endpos=None):
        """Scan through string looking for a match to the pattern, returning
        a MatchObject instance, or None if no match was found."""

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
        """Try to apply the pattern at the start of the string, returning
        a MatchObject instance, or None if no match was found."""

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
        """Return the string obtained by replacing the leftmost
        non-overlapping occurrences of the pattern in string by the
        replacement repl""" 

        return self.subn(repl, string, count)[0]
    
    def subn(self, repl, source, count=0): 
        """Return a 2-tuple containing (new_string, number).
        new_string is the string obtained by replacing the leftmost
        non-overlapping occurrences of the pattern in the source
        string by the replacement repl.  number is the number of
        substitutions that were made."""

        if count < 0:
            raise error, "negative substitution count"
        if count == 0:
            import sys
            count = sys.maxint
        if type(repl) == type(''):
            if '\\' in repl:
                repl = lambda m, r=repl: pcre_expand(m, r)
            else:
                repl = lambda m, r=repl: r
        n = 0           # Number of matches
        pos = 0         # Where to start searching
        lastmatch = -1  # End of last match
        results = []    # Substrings making up the result
        end = len(source)
        while n < count and pos <= end:
            m = self.search(source, pos)
            if not m:
                break
            i, j = m.span(0)
            if i == j == lastmatch:
                # Empty match adjacent to previous match
                pos = pos + 1
                results.append(source[lastmatch:pos])
                continue
            if pos < i:
                results.append(source[pos:i])
            results.append(repl(m))
            pos = lastmatch = j
            if i == j:
                # Last match was empty; don't try here again
                pos = pos + 1
                results.append(source[lastmatch:pos])
            n = n + 1
        results.append(source[pos:])
        return (string.join(results, ''), n)
                                                                            
    def split(self, source, maxsplit=0):
        """Split the source string by the occurrences of the pattern,
        returning a list containing the resulting substrings."""

        if maxsplit < 0:
            raise error, "negative split count"
        if maxsplit == 0:
            import sys
            maxsplit = sys.maxint
        n = 0
        pos = 0
        lastmatch = 0
        results = []
        end = len(source)
        while n < maxsplit:
            m = self.search(source, pos)
            if not m:
                break
            i, j = m.span(0)
            if i == j:
                # Empty match
                if pos >= end:
                    break
                pos = pos+1
                continue
            results.append(source[lastmatch:i])
            g = m.groups()
            if g:
                results[len(results):] = list(g)
            pos = lastmatch = j
            n = n + 1
        results.append(source[lastmatch:])
        return results

    def findall(self, string):
        """Return a list of all non-overlapping matches in the string.

        If one or more groups are present in the pattern, return a
        list of groups; this will be a list of tuples if the pattern
        has more than one group.

        Empty matches are included in the result.

        """
        pos = 0
        n = len(string)
        result = []
        while pos <= n:
            m = self.search(string, pos)
            if not m:
                break
            gr = m.groups()
            if not gr:
                gr = m.group()
            elif len(gr) == 1:
                gr = gr[0]
            result.append(gr)
            pos = max(m.end(), pos+1)
        return result

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

class MatchObject:

    def __init__(self, re, string, pos, endpos, regs):
        self.re = re
        self.string = string
        self.pos = pos 
        self.endpos = endpos
        self.regs = regs
        
    def start(self, g = 0):
        "Return the start of the substring matched by group g"
        if type(g) == type(''):
            try:
                g = self.re.groupindex[g]
            except (KeyError, TypeError):
                raise IndexError, 'group %s is undefined' % `g`
        return self.regs[g][0]
    
    def end(self, g = 0):
        "Return the end of the substring matched by group g"
        if type(g) == type(''):
            try:
                g = self.re.groupindex[g]
            except (KeyError, TypeError):
                raise IndexError, 'group %s is undefined' % `g`
        return self.regs[g][1]
    
    def span(self, g = 0):
        "Return (start, end) of the substring matched by group g"
        if type(g) == type(''):
            try:
                g = self.re.groupindex[g]
            except (KeyError, TypeError):
                raise IndexError, 'group %s is undefined' % `g`
        return self.regs[g]
    
    def groups(self, default=None):
        "Return a tuple containing all subgroups of the match object"
        result = []
        for g in range(1, self.re._num_regs):
            a, b = self.regs[g]
            if a == -1 or b == -1:
                result.append(default)
            else:
                result.append(self.string[a:b])
        return tuple(result)

    def group(self, *groups):
        "Return one or more groups of the match"
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
        "Return a dictionary containing all named subgroups of the match"
        dict = {}
        for name, index in self.re.groupindex.items():
            a, b = self.regs[index]
            if a == -1 or b == -1:
                dict[name] = default
            else:
                dict[name] = self.string[a:b]
        return dict
