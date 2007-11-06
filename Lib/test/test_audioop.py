# Test audioop.
import audioop
from test.test_support import verbose

def gendata1():
    return b'\0\1\2'

def gendata2():
    if verbose:
        print('getsample')
    if audioop.getsample(b'\0\1', 2, 0) == 1:
        return b'\0\0\0\1\0\2'
    else:
        return b'\0\0\1\0\2\0'

def gendata4():
    if verbose:
        print('getsample')
    if audioop.getsample(b'\0\0\0\1', 4, 0) == 1:
        return b'\0\0\0\0\0\0\0\1\0\0\0\2'
    else:
        return b'\0\0\0\0\1\0\0\0\2\0\0\0'

def testmax(data):
    if verbose:
        print('max')
    if audioop.max(data[0], 1) != 2 or \
              audioop.max(data[1], 2) != 2 or \
              audioop.max(data[2], 4) != 2:
        return 0
    return 1

def testminmax(data):
    if verbose:
        print('minmax')
    if audioop.minmax(data[0], 1) != (0, 2) or \
              audioop.minmax(data[1], 2) != (0, 2) or \
              audioop.minmax(data[2], 4) != (0, 2):
        return 0
    return 1

def testmaxpp(data):
    if verbose:
        print('maxpp')
    if audioop.maxpp(data[0], 1) != 0 or \
              audioop.maxpp(data[1], 2) != 0 or \
              audioop.maxpp(data[2], 4) != 0:
        return 0
    return 1

def testavg(data):
    if verbose:
        print('avg')
    if audioop.avg(data[0], 1) != 1 or \
              audioop.avg(data[1], 2) != 1 or \
              audioop.avg(data[2], 4) != 1:
        return 0
    return 1

def testavgpp(data):
    if verbose:
        print('avgpp')
    if audioop.avgpp(data[0], 1) != 0 or \
              audioop.avgpp(data[1], 2) != 0 or \
              audioop.avgpp(data[2], 4) != 0:
        return 0
    return 1

def testrms(data):
    if audioop.rms(data[0], 1) != 1 or \
              audioop.rms(data[1], 2) != 1 or \
              audioop.rms(data[2], 4) != 1:
        return 0
    return 1

def testcross(data):
    if verbose:
        print('cross')
    if audioop.cross(data[0], 1) != 0 or \
              audioop.cross(data[1], 2) != 0 or \
              audioop.cross(data[2], 4) != 0:
        return 0
    return 1

def testadd(data):
    if verbose:
        print('add')
    data2 = []
    for d in data:
        str = buffer(len(d))
        for i,b in enumerate(d):
            str[i] = 2*b
        data2.append(str)
    if audioop.add(data[0], data[0], 1) != data2[0] or \
              audioop.add(data[1], data[1], 2) != data2[1] or \
              audioop.add(data[2], data[2], 4) != data2[2]:
        return 0
    return 1

def testbias(data):
    if verbose:
        print('bias')
    # Note: this test assumes that avg() works
    d1 = audioop.bias(data[0], 1, 100)
    d2 = audioop.bias(data[1], 2, 100)
    d4 = audioop.bias(data[2], 4, 100)
    if audioop.avg(d1, 1) != 101 or \
              audioop.avg(d2, 2) != 101 or \
              audioop.avg(d4, 4) != 101:
        return 0
    return 1

def testlin2lin(data):
    if verbose:
        print('lin2lin')
    # too simple: we test only the size
    for d1 in data:
        for d2 in data:
            got = len(d1)//3
            wtd = len(d2)//3
            if len(audioop.lin2lin(d1, got, wtd)) != len(d2):
                return 0
    return 1

def testadpcm2lin(data):
    # Very cursory test
    if audioop.adpcm2lin(b'\0\0', 1, None) != (b'\0\0\0\0', (0,0)):
        return 0
    return 1

def testlin2adpcm(data):
    if verbose:
        print('lin2adpcm')
    # Very cursory test
    if audioop.lin2adpcm(b'\0\0\0\0', 1, None) != (b'\0\0', (0,0)):
        return 0
    return 1

def testlin2alaw(data):
    if verbose:
        print('lin2alaw')
    if audioop.lin2alaw(data[0], 1) != b'\xd5\xc5\xf5' or \
              audioop.lin2alaw(data[1], 2) != b'\xd5\xd5\xd5' or \
              audioop.lin2alaw(data[2], 4) != b'\xd5\xd5\xd5':
        return 0
    return 1

def testalaw2lin(data):
    if verbose:
        print('alaw2lin')
    # Cursory
    d = audioop.lin2alaw(data[0], 1)
    if audioop.alaw2lin(d, 1) != data[0]:
        return 0
    return 1

def testlin2ulaw(data):
    if verbose:
        print('lin2ulaw')
    if audioop.lin2ulaw(data[0], 1) != b'\xff\xe7\xdb' or \
              audioop.lin2ulaw(data[1], 2) != b'\xff\xff\xff' or \
              audioop.lin2ulaw(data[2], 4) != b'\xff\xff\xff':
        return 0
    return 1

def testulaw2lin(data):
    if verbose:
        print('ulaw2lin')
    # Cursory
    d = audioop.lin2ulaw(data[0], 1)
    if audioop.ulaw2lin(d, 1) != data[0]:
        return 0
    return 1

def testmul(data):
    if verbose:
        print('mul')
    data2 = []
    for d in data:
        str = buffer(len(d))
        for i,b in enumerate(d):
            str[i] = 2*b
        data2.append(str)
    if audioop.mul(data[0], 1, 2) != data2[0] or \
              audioop.mul(data[1],2, 2) != data2[1] or \
              audioop.mul(data[2], 4, 2) != data2[2]:
        return 0
    return 1

def testratecv(data):
    if verbose:
        print('ratecv')
    state = None
    d1, state = audioop.ratecv(data[0], 1, 1, 8000, 16000, state)
    d2, state = audioop.ratecv(data[0], 1, 1, 8000, 16000, state)
    if d1 + d2 != b'\000\000\001\001\002\001\000\000\001\001\002':
        return 0
    return 1

def testreverse(data):
    if verbose:
        print('reverse')
    if audioop.reverse(data[0], 1) != b'\2\1\0':
        return 0
    return 1

def testtomono(data):
    if verbose:
        print('tomono')
    data2 = buffer()
    for d in data[0]:
        data2.append(d)
        data2.append(d)
    if audioop.tomono(data2, 1, 0.5, 0.5) != data[0]:
        return 0
    return 1

def testtostereo(data):
    if verbose:
        print('tostereo')
    data2 = buffer()
    for d in data[0]:
        data2.append(d)
        data2.append(d)
    if audioop.tostereo(data[0], 1, 1, 1) != data2:
        return 0
    return 1

def testfindfactor(data):
    if verbose:
        print('findfactor')
    if audioop.findfactor(data[1], data[1]) != 1.0:
        return 0
    return 1

def testfindfit(data):
    if verbose:
        print('findfit')
    if audioop.findfit(data[1], data[1]) != (0, 1.0):
        return 0
    return 1

def testfindmax(data):
    if verbose:
        print('findmax')
    if audioop.findmax(data[1], 1) != 2:
        return 0
    return 1

def testgetsample(data):
    if verbose:
        print('getsample')
    for i in range(3):
        if audioop.getsample(data[0], 1, i) != i or \
                  audioop.getsample(data[1], 2, i) != i or \
                  audioop.getsample(data[2], 4, i) != i:
            return 0
    return 1

def testone(name, data):
    try:
        func = eval('test'+name)
    except NameError:
        print('No test found for audioop.'+name+'()')
        return
    try:
        rv = func(data)
    except Exception as e:
        print('Test FAILED for audioop.'+name+'() (with %s)' % repr(e))
        return
    if not rv:
        print('Test FAILED for audioop.'+name+'()')

def testall():
    data = [gendata1(), gendata2(), gendata4()]
    names = dir(audioop)
    # We know there is a routine 'add'
    routines = []
    for n in names:
        if type(eval('audioop.'+n)) == type(audioop.add):
            routines.append(n)
    for n in routines:
        testone(n, data)
testall()
