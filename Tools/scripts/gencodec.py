""" Unicode Mapping Parser and Codec Generator.

This script parses Unicode mapping files as available from the Unicode
site (ftp.unicode.org) and creates Python codec modules from them. The
codecs use the standard character mapping codec to actually apply the
mapping.

Synopsis: gencodec.py dir codec_prefix

All files in dir are scanned and those producing non-empty mappings
will be written to <codec_prefix><mapname>.py with <mapname> being the
first part of the map's filename ('a' in a.b.c.txt) converted to
lowercase with hyphens replaced by underscores.

The tool also writes marshalled versions of the mapping tables to the
same location (with .mapping extension).

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"

import string,re,os,time,marshal

# Create numeric tables or character based ones ?
numeric = 1

mapRE = re.compile('((?:0x[0-9a-fA-F]+\+?)+)'
                   '\s+'
                   '((?:(?:0x[0-9a-fA-Z]+|<[A-Za-z]+>)\+?)*)'
                   '\s*'
                   '(#.+)?')

def parsecodes(codes,

               split=string.split,atoi=string.atoi,len=len,
               filter=filter,range=range):

    """ Converts code combinations to either a single code integer
        or a tuple of integers.

        meta-codes (in angular brackets, e.g. <LR> and <RL>) are
        ignored.

        Empty codes or illegal ones are returned as None.

    """
    if not codes:
        return None
    l = split(codes,'+')
    if len(l) == 1:
        return atoi(l[0],16)
    for i in range(len(l)):
        try:
            l[i] = atoi(l[i],16)
        except ValueError:
            l[i] = None
    l = filter(lambda x: x is not None, l)
    if len(l) == 1:
        return l[0]
    else:
        return tuple(l)

def readmap(filename,

            strip=string.strip):

    f = open(filename,'r')
    lines = f.readlines()
    f.close()
    enc2uni = {}
    for line in lines:
        line = strip(line)
        if not line or line[0] == '#':
            continue
        m = mapRE.match(line)
        if not m:
            #print '* not matched: %s' % repr(line)
            continue
        enc,uni,comment = m.groups()
        enc = parsecodes(enc)
        uni = parsecodes(uni)
        if not comment:
            comment = ''
        else:
            comment = comment[1:]
        if enc != uni:
            enc2uni[enc] = (uni,comment)
    return enc2uni

def hexrepr(t,

            join=string.join):

    if t is None:
        return 'None'
    try:
        len(t)
    except:
        return '0x%04x' % t
    return '(' + join(map(lambda t: '0x%04x' % t, t),', ') + ')'

def unicoderepr(t,

                join=string.join):

    if t is None:
        return 'None'
    if numeric:
        return hexrepr(t)
    else:
        try:
            len(t)
        except:
            return repr(unichr(t))
        return repr(join(map(unichr, t),''))

def keyrepr(t,

            join=string.join):

    if t is None:
        return 'None'
    if numeric:
        return hexrepr(t)
    else:
        try:
            len(t)
        except:
            if t < 256:
                return repr(chr(t))
            else:
                return repr(unichr(t))
        return repr(join(map(chr, t),''))

def codegen(name,map,comments=1):

    """ Returns Python source for the given map.

        Comments are included in the source, if comments is true (default).

    """
    l = [
        '''\
""" Python Character Mapping Codec generated from '%s'.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"

import codecs

### Codec APIs

class Codec(codecs.Codec):

    def encode(self,input,errors='strict'):

        return codecs.charmap_encode(input,errors,encoding_map)
        
    def decode(self,input,errors='strict'):

        return codecs.charmap_decode(input,errors,decoding_map)

class StreamWriter(Codec,codecs.StreamWriter):
    pass
        
class StreamReader(Codec,codecs.StreamReader):
    pass

### encodings module API

def getregentry():

    return (Codec().encode,Codec().decode,StreamReader,StreamWriter)

### Decoding Map

decoding_map = {
''' % name,
        ]
    mappings = map.items()
    mappings.sort()
    append = l.append
    i = 0
    splits = 0
    for e,value in mappings:
        try:
            (u,c) = value
        except TypeError:
            u = value
            c = ''
        key = keyrepr(e)
        if c and comments:
            append('\t%s: %s,\t# %s' % (key,unicoderepr(u),c))
        else:
            append('\t%s: %s,' % (key,unicoderepr(u)))
        i = i + 1
        if i == 4096:
            # Split the definition into parts to that the Python
            # parser doesn't dump core
            if splits == 0:
                append('}')
            else:
                append('})')
            append('map.update({')
            i = 0
            splits = splits + 1
    if splits == 0:
        append('}')
    else:
        append('})')
    append('''
### Encoding Map

encoding_map = {}
for k,v in decoding_map.items():
    encoding_map[v] = k
''')
    return string.join(l,'\n')

def pymap(name,map,pyfile,comments=1):

    code = codegen(name,map,comments)
    f = open(pyfile,'w')
    f.write(code)
    f.close()

def marshalmap(name,map,marshalfile):

    d = {}
    for e,(u,c) in map.items():
        d[e] = (u,c)
    f = open(marshalfile,'wb')
    marshal.dump(d,f)
    f.close()

def convertdir(dir,prefix='',comments=1):

    mapnames = os.listdir(dir)
    for mapname in mapnames:
        name = os.path.split(mapname)[1]
        name = string.replace(name,'-','_')
        name = string.split(name, '.')[0]
        name = string.lower(name)
        codefile = name + '.py'
        marshalfile = name + '.mapping'
        print 'converting %s to %s and %s' % (mapname,
                                              prefix + codefile,
                                              prefix + marshalfile)
        try:
            map = readmap(os.path.join(dir,mapname))
            if not map:
                print '* map is empty; skipping'
            else:
                pymap(mapname, map, prefix + codefile,comments)
                marshalmap(mapname, map, prefix + marshalfile)
        except ValueError:
            print '* conversion failed'

def rewritepythondir(dir,prefix='',comments=1):
    
    mapnames = os.listdir(dir)
    for mapname in mapnames:
        if mapname[-len('.mapping'):] != '.mapping':
            continue
        codefile = mapname[:-len('.mapping')] + '.py'
        print 'converting %s to %s' % (mapname,
                                       prefix + codefile)
        try:
            map = marshal.load(open(os.path.join(dir,mapname),
                               'rb'))
            if not map:
                print '* map is empty; skipping'
            else:
                pymap(mapname, map, prefix + codefile,comments)
        except ValueError, why:
            print '* conversion failed: %s' % why

if __name__ == '__main__':

    import sys
    if 1:
        apply(convertdir,tuple(sys.argv[1:]))
    else:
        apply(rewritepythondir,tuple(sys.argv[1:]))
