from test_support import verbose
import regex
from regex_syntax import *

re = 'a+b+c+'
print 'no match:', regex.match(re, 'hello aaaabcccc world')
print 'successful search:', regex.search(re, 'hello aaaabcccc world')
try:
    cre = regex.compile('\(' + re)
except regex.error:
    print 'caught expected exception'
else:
    print 'expected regex.error not raised'

print 'failed awk syntax:', regex.search('(a+)|(b+)', 'cdb')
prev = regex.set_syntax(RE_SYNTAX_AWK)
print 'successful awk syntax:', regex.search('(a+)|(b+)', 'cdb')
regex.set_syntax(prev)
print 'failed awk syntax:', regex.search('(a+)|(b+)', 'cdb')

re = '\(<one>[0-9]+\) *\(<two>[0-9]+\)'
print 'matching with group names and compile()'
cre = regex.compile(re)
print cre.match('801 999')
try:
    print cre.group('one')
except regex.error:
    print 'caught expected exception'
else:
    print 'expected regex.error not raised'

print 'matching with group names and symcomp()'
cre = regex.symcomp(re)
print cre.match('801 999')
print cre.group(0)
print cre.group('one')
print cre.group(1, 2)
print cre.group('one', 'two')
print 'realpat:', cre.realpat
print 'groupindex:', cre.groupindex

re = 'world'
cre = regex.compile(re)
print 'not case folded search:', cre.search('HELLO WORLD')
cre = regex.compile(re, regex.casefold)
print 'case folded search:', cre.search('HELLO WORLD')

print '__members__:', cre.__members__
print 'regs:', cre.regs
print 'last:', cre.last
print 'translate:', `cre.translate`
print 'givenpat:', cre.givenpat

print 'match with pos:', cre.match('hello world', 7)
print 'search with pos:', cre.search('hello world there world', 7)
print 'bogus group:', cre.group(0, 1, 3)
try:
    print 'no name:', cre.group('one')
except regex.error:
    print 'caught expected exception'
else:
    print 'expected regex.error not raised'
