import calldll
import macfs
import sys
import MacOS
from Carbon import Res

fss, ok = macfs.PromptGetFile("Show me calldll.ppc.slb")

lib = calldll.getdiskfragment(fss, 'calldll.ppc.slb')

cdll_b_bbbbbbbb = calldll.newcall(lib.cdll_b_bbbbbbbb, 'Byte', 'InByte', 'InByte',
                                'InByte', 'InByte','InByte', 'InByte','InByte', 'InByte')
cdll_h_hhhhhhhh = calldll.newcall(lib.cdll_h_hhhhhhhh, 'Short', 'InShort', 'InShort',
                                'InShort', 'InShort','InShort', 'InShort','InShort', 'InShort')
cdll_l_llllllll = calldll.newcall(lib.cdll_l_llllllll, 'Long', 'InLong', 'InLong',
                                'InLong', 'InLong','InLong', 'InLong','InLong', 'InLong')

cdll_N_ssssssss = calldll.newcall(lib.cdll_N_ssssssss, 'None', 'InString', 'InString',
                                'InString', 'InString', 'InString', 'InString', 'InString', 'InString')

cdll_o_l = calldll.newcall(lib.cdll_o_l, 'OSErr', 'InLong')

cdll_N_pp = calldll.newcall(lib.cdll_N_pp, 'None', 'InPstring', 'OutPstring')

cdll_N_bb = calldll.newcall(lib.cdll_N_bb, 'None', 'InByte', 'OutByte')
cdll_N_hh = calldll.newcall(lib.cdll_N_hh, 'None', 'InShort', 'OutShort')
cdll_N_ll = calldll.newcall(lib.cdll_N_ll, 'None', 'InLong', 'OutLong')
cdll_N_sH = calldll.newcall(lib.cdll_N_sH, 'None', 'InString', 'InHandle')

print 'Test cdll_b_bbbbbbbb'
rv = cdll_b_bbbbbbbb(1, 2, 3, 4, 5, 6, 7, 8)
if rv == 36:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_b_bbbbbbbb negative'
rv = cdll_b_bbbbbbbb(-1, -2, -3, -4, -5, -6, -7, -8)
if rv == -36:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_h_hhhhhhhh'
rv = cdll_h_hhhhhhhh(1, 2, 3, 4, 5, 6, 7, 8)
if rv == 36:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_h_hhhhhhhh negative'
rv = cdll_h_hhhhhhhh(-1, -2, -3, -4, -5, -6, -7, -8)
if rv == -36:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_l_llllllll'
rv = cdll_l_llllllll(1, 2, 3, 4, 5, 6, 7, 8)
if rv == 36:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_l_llllllll negative'
rv = cdll_l_llllllll(-1, -2, -3, -4, -5, -6, -7, -8)
if rv == -36:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_N_ssssssss'
print 'Should print one two three four five six seven eight'
rv = cdll_N_ssssssss('one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight')
if rv == None:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_o_l(0)'
rv = cdll_o_l(0)
if rv == None:
    print 'ok.'
else:
    print 'Error, returned', rv

print 'Test cdll_o_l(-100)'
try:
    rv = cdll_o_l(-100)
    print 'Error, did not raise exception, returned', rv
except MacOS.Error, arg:
    if arg[0] == -100:
        print 'ok.'
    else:
        print 'Error, returned incorrect exception arg:', arg[0]

print 'Test cdll_N_pp'
rv = cdll_N_pp('pascal string')
if rv == 'Was: pascal string':
    print 'ok.'
else:
    print 'Failed, returned', repr(rv)

print 'Test cdll_N_bb'
rv = cdll_N_bb(-100)
if rv == -100:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_N_hh'
rv = cdll_N_hh(-100)
if rv == -100:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_N_ll'
rv = cdll_N_ll(-100)
if rv == -100:
    print 'ok.'
else:
    print 'Failed, returned', rv

print 'Test cdll_N_sH'
h = Res.Resource('xyz')
rv = cdll_N_sH('new data', h)
if rv == None and h.data == 'new data':
    print 'ok.'
else:
    print 'Failed, rv is', rv, 'and handle data is', repr(rv.data)
sys.exit(1)
