#
# genmap_support.py: Multibyte Codec Map Generator
#
# Original Author:  Hye-Shik Chang <perky@FreeBSD.org>
# Modified Author:  Donghee Na <donghee.na92@gmail.com>
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


class DecodeMapWriter:
    filler_class = BufferedFiller

    def __init__(self, fp, prefix, decode_map):
        self.fp = fp
        self.prefix = prefix
        self.decode_map = decode_map
        self.filler = self.filler_class()

    def update_decode_map(self, c1range, c2range, onlymask=(), wide=0):
        c2values = range(c2range[0], c2range[1] + 1)

        for c1 in range(c1range[0], c1range[1] + 1):
            if c1 not in self.decode_map or (onlymask and c1 not in onlymask):
                continue
            c2map = self.decode_map[c1]
            rc2values = [n for n in c2values if n in c2map]
            if not rc2values:
                continue

            c2map[self.prefix] = True
            c2map['min'] = rc2values[0]
            c2map['max'] = rc2values[-1]
            c2map['midx'] = len(self.filler)

            for v in range(rc2values[0], rc2values[-1] + 1):
                if v in c2map:
                    self.filler.write('%d,' % c2map[v])
                else:
                    self.filler.write('U,')

    def generate(self, wide=False):
        if not wide:
            self.fp.write(f"static const ucs2_t __{self.prefix}_decmap[{len(self.filler)}] = {{\n")
        else:
            self.fp.write(f"static const Py_UCS4 __{self.prefix}_decmap[{len(self.filler)}] = {{\n")

        self.filler.printout(self.fp)
        self.fp.write("};\n\n")

        if not wide:
            self.fp.write(f"static const struct dbcs_index {self.prefix}_decmap[256] = {{\n")
        else:
            self.fp.write(f"static const struct widedbcs_index {self.prefix}_decmap[256] = {{\n")

        for i in range(256):
            if i in self.decode_map and self.prefix in self.decode_map[i]:
                m = self.decode_map
                prefix = self.prefix
            else:
                self.filler.write("{", "0,", "0,", "0", "},")
                continue

            self.filler.write("{", "__%s_decmap" % prefix, "+", "%d" % m[i]['midx'],
                              ",", "%d," % m[i]['min'], "%d" % m[i]['max'], "},")
        self.filler.printout(self.fp)
        self.fp.write("};\n\n")


class EncodeMapWriter:
    filler_class = BufferedFiller
    elemtype = 'DBCHAR'
    indextype = 'struct unim_index'

    def __init__(self, fp, prefix, encode_map):
        self.fp = fp
        self.prefix = prefix
        self.encode_map = encode_map
        self.filler = self.filler_class()

    def generate(self):
        self.buildmap()
        self.printmap()

    def buildmap(self):
        for c1 in range(0, 256):
            if c1 not in self.encode_map:
                continue
            c2map = self.encode_map[c1]
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

    def printmap(self):
        self.fp.write(f"static const {self.elemtype} __{self.prefix}_encmap[{len(self.filler)}] = {{\n")
        self.filler.printout(self.fp)
        self.fp.write("};\n\n")
        self.fp.write(f"static const {self.indextype} {self.prefix}_encmap[256] = {{\n")

        for i in range(256):
            if i in self.encode_map and self.prefix in self.encode_map[i]:
                self.filler.write("{", "__%s_encmap" % self.prefix, "+",
                                  "%d" % self.encode_map[i]['midx'], ",",
                                  "%d," % self.encode_map[i]['min'],
                                  "%d" % self.encode_map[i]['max'], "},")
            else:
                self.filler.write("{", "0,", "0,", "0", "},")
                continue
        self.filler.printout(self.fp)
        self.fp.write("};\n\n")


def open_mapping_file(path, source):
    try:
        f = open(path)
    except IOError:
        raise SystemExit(f'{source} is needed')
    return f


def print_autogen(fo, source):
    fo.write(f'// AUTO-GENERATED FILE FROM {source}: DO NOT EDIT\n')


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
