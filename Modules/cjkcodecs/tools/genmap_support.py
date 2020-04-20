#
# genmap_support.py: Multibyte Codec Map Generator
#
# Original Author:  Hye-Shik Chang <perky@FreeBSD.org>
# Modified Author:  Dong-hee Na <donghee.na92@gmail.com>
#


class BufferedFiller:
    def __init__(self, column=78):
        self.column = column
        self.buffered = []
        self.cline = []
        self.clen = 0
        self.count = 0

    def write(self, *data):
        for s in data:
            if len(s) > self.column:
                raise ValueError("token is too long")
            if len(s) + self.clen > self.column:
                self.flush()
            self.clen += len(s)
            self.cline.append(s)
            self.count += 1

    def flush(self):
        if not self.cline:
            return
        self.buffered.append(''.join(self.cline))
        self.clen = 0
        del self.cline[:]

    def printout(self, fp):
        self.flush()
        for l in self.buffered:
            fp.write(f'{l}\n')
        del self.buffered[:]

    def __len__(self):
        return self.count

class UCMReader:
    def __init__(self, fp):
        self.file = fp

    def itertokens(self):
        isincharmap = False
        for line in self.file:
            body = line.split('#', 1)[0].strip()
            if body == 'CHARMAP':
                isincharmap = True
            elif body == 'END CHARMAP':
                isincharmap = False
            elif isincharmap:
                index, data = body.split(None, 1)
                index = int(index[2:-1], 16)
                data = self.parsedata(data)
                yield index, data

    def parsedata(self, data):
        return eval('"'+data.split()[0]+'"')


class EncodeMapWriter:
    filler_class = BufferedFiller
    elemtype = 'DBCHAR'
    indextype = 'struct unim_index'

    def __init__(self, fp, prefix, m):
        self.file = fp
        self.prefix = prefix
        self.m = m
        self.filler = self.filler_class()

    def generate(self):
        self.buildmap(self.m)
        self.printmap(self.m)

    def buildmap(self, emap):
        for c1 in range(0, 256):
            if c1 not in emap:
                continue
            c2map = emap[c1]
            rc2values = [k for k in c2map.keys()]
            rc2values.sort()
            if not rc2values:
                continue

            c2map[self.prefix] = True
            c2map['min'] = rc2values[0]
            c2map['max'] = rc2values[-1]
            c2map['midx'] = len(self.filler)

            for v in range(rc2values[0], rc2values[-1] + 1):
                if v not in c2map:
                    self.write_nochar()
                elif isinstance(c2map[v], int):
                    self.write_char(c2map[v])
                elif isinstance(c2map[v], tuple):
                    self.write_multic(c2map[v])
                else:
                    raise ValueError

    def write_nochar(self):
        self.filler.write('N,')

    def write_multic(self, point):
        self.filler.write('M,')

    def write_char(self, point):
        self.filler.write(str(point) + ',')

    def printmap(self, fmap):
        self.file.write(f"static const {self.elemtype} __{self.prefix}_encmap[{len(self.filler)}] = {{\n")
        self.filler.printout(self.file)
        self.file.write("};\n\n")
        self.file.write(f"static const {self.indextype} {self.prefix}_encmap[256] = {{\n")

        for i in range(256):
            if i in fmap and self.prefix in fmap[i]:
                self.filler.write("{", "__%s_encmap" % self.prefix, "+",
                                  "%d" % fmap[i]['midx'], ",",
                                  "%d," % fmap[i]['min'],
                                  "%d" % fmap[i]['max'], "},")
            else:
                self.filler.write("{", "0,", "0,", "0", "},")
                continue
        self.filler.printout(self.file)
        self.file.write("};\n\n")

def open_mapping_file(path, source):
    try:
        f = open(path)
    except IOError:
        raise SystemExit(f'{source} is needed')
    return f

def print_autogen(fo, source):
    fo.write(f'// AUTO-GENERATED FILE FROM {source}: DO NOT EDIT\n')

def genmap_decode(filler, prefix, c1range, c2range, dmap, onlymask=(),
                  wide=0):
    c2width  = c2range[1] - c2range[0] + 1
    c2values = range(c2range[0], c2range[1] + 1)

    for c1 in range(c1range[0], c1range[1] + 1):
        if c1 not in dmap or (onlymask and c1 not in onlymask):
            continue
        c2map = dmap[c1]
        rc2values = [n for n in c2values if n in c2map]
        if not rc2values:
            continue

        c2map[prefix] = True
        c2map['min'] = rc2values[0]
        c2map['max'] = rc2values[-1]
        c2map['midx'] = len(filler)

        for v in range(rc2values[0], rc2values[-1] + 1):
            if v in c2map:
                filler.write('%d,' % c2map[v])
            else:
                filler.write('U,')

def print_decmap(fo, filler, fmapprefix, fmap, f2map={}, f2mapprefix='',
                 wide=0):
    if not wide:
        fo.write(f"static const ucs2_t __{fmapprefix}_decmap[{len(filler)}] = {{\n")
        width = 8
    else:
        fo.write(f"static const Py_UCS4 __{fmapprefix}_decmap[{len(filler)}] = {{\n")
        width = 4
    filler.printout(fo)
    fo.write("};\n\n")

    if not wide:
        fo.write(f"static const struct dbcs_index {fmapprefix}_decmap[256] = {{\n")
    else:
        fo.write(f"static const struct widedbcs_index {fmapprefix}_decmap[256] = {{\n")

    for i in range(256):
        if i in fmap and fmapprefix in fmap[i]:
            m = fmap
            prefix = fmapprefix
        elif i in f2map and f2mapprefix in f2map[i]:
            m = f2map
            prefix = f2mapprefix
        else:
            filler.write("{", "0,", "0,", "0", "},")
            continue

        filler.write("{", "__%s_decmap" % prefix, "+", "%d" % m[i]['midx'],
                     ",", "%d," % m[i]['min'], "%d" % m[i]['max'], "},")
    filler.printout(fo)
    fo.write("};\n\n")

def loadmap(fo, natcol=0, unicol=1, sbcs=0):
    print("Loading from", fo)
    fo.seek(0, 0)
    decmap = {}
    for line in fo:
        line = line.split('#', 1)[0].strip()
        if not line or len(line.split()) < 2:
            continue

        row = [eval(e) for e in line.split()]
        loc, uni = row[natcol], row[unicol]
        if loc >= 0x100 or sbcs:
            decmap.setdefault((loc >> 8), {})
            decmap[(loc >> 8)][(loc & 0xff)] = uni

    return decmap
