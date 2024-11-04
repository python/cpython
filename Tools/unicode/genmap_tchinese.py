#
# genmap_tchinese.py: Traditional Chinese Codecs Map Generator
#
# Original Author:  Hye-Shik Chang <perky@FreeBSD.org>
#
import os

from genmap_support import *


# ranges for (lead byte, follower byte)
BIG5_C1 = (0xa1, 0xfe)
BIG5_C2 = (0x40, 0xfe)
BIG5HKSCS_C1 = (0x87, 0xfe)
BIG5HKSCS_C2 = (0x40, 0xfe)

MAPPINGS_BIG5 = 'https://unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/OTHER/BIG5.TXT'
MAPPINGS_CP950 = 'https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP950.TXT'

HKSCS_VERSION = '2004'
# The files for HKSCS mappings are available under a restrictive license.
# Users of the script need to download the files from the HKSARG CCLI website:
MAPPINGS_HKSCS = f'https://www.ccli.gov.hk/en/archive/terms_hkscs-{HKSCS_VERSION}-big5-iso.html'


def bh2s(code):
    return ((code >> 8) - 0x87) * (0xfe - 0x40 + 1) + ((code & 0xff) - 0x40)


def split_bytes(code):
    """Split 0xABCD into 0xAB, 0xCD"""
    return code >> 8, code & 0xff


def parse_hkscs_map(fo):
    fo.seek(0, 0)
    table = []
    for line in fo:
        line = line.split('#', 1)[0].strip()
        # We expect 4 columns in supported HKSCS files:
        # [1999]: unsupported
        # [2001]: unsupported
        # [2004]: Big-5; iso10646-1:1993; iso10646-1:2000; iso10646:2003+amd1
        # [2008]: Big-5; iso10646-1:1993; iso10646-1:2000; iso10646:2003+amd6
        # [2016]: not supported here--uses a json file instead
        #
        # In both supported cases, we only need the first and last column:
        #  * Big-5 is a hex string (always 4 digits)
        #  * iso10646:2003 is either a hex string (4 or 5 digits) or a sequence
        #    of hex strings like: `<code_point1,code_point2>`
        try:
            hkscs_col, _, _, uni_col = line.split()
            hkscs = int(hkscs_col, 16)
            seq = tuple(int(cp, 16) for cp in uni_col.strip('<>').split(','))
        except ValueError:
            continue
        table.append((hkscs, seq))
    return table


def make_hkscs_map(table):
    decode_map = {}
    encode_map_bmp, encode_map_notbmp = {}, {}
    is_bmp_map = {}
    sequences = []
    beginnings = {}
    single_cp_table = []
    # Determine multi-codepoint sequences, and sequence beginnings that encode
    # multiple multibyte (i.e. Big-5) codes.
    for mbcode, cp_seq in table:
        cp, *_ = cp_seq
        if len(cp_seq) == 1:
            single_cp_table.append((mbcode, cp))
        else:
            sequences.append((mbcode, cp_seq))
        beginnings.setdefault(cp, []).append(mbcode)
    # Decode table only cares about single code points (no sequences) currently
    for mbcode, cp in single_cp_table:
        b1, b2 = split_bytes(mbcode)
        decode_map.setdefault(b1, {})
        decode_map[b1][b2] = cp & 0xffff
    # Encode table needs to mark code points beginning a sequence as tuples.
    for cp, mbcodes in beginnings.items():
        plane = cp >> 16
        if plane == 0:
            encode_map = encode_map_bmp
        elif plane == 2:
            encode_map = encode_map_notbmp
            is_bmp_map[bh2s(mbcodes[0])] = 1
        else:
            assert False, 'only plane 0 (BMP) and plane 2 (SIP) allowed'
        if len(mbcodes) == 1:
            encode_value = mbcodes[0]
        else:
            encode_value = tuple(mbcodes)
        uni_b1, uni_b2 = split_bytes(cp & 0xffff)
        encode_map.setdefault(uni_b1, {})
        encode_map[uni_b1][uni_b2] = encode_value

    return decode_map, encode_map_bmp, encode_map_notbmp, is_bmp_map


def load_big5_map():
    mapfile = open_mapping_file('python-mappings/BIG5.txt', MAPPINGS_BIG5)
    with mapfile:
        big5decmap = loadmap(mapfile)
    # big5 mapping fix: use the cp950 mapping for these characters as the file
    # provided by unicode.org doesn't define a mapping. See notes in BIG5.txt.
    # Since U+5341, U+5345, U+FF0F, U+FF3C already have a big5 mapping, no
    # roundtrip compatibility is guaranteed for those.
    for m in """\
    0xA15A      0x2574
    0xA1C3      0xFFE3
    0xA1C5      0x02CD
    0xA1FE      0xFF0F
    0xA240      0xFF3C
    0xA2CC      0x5341
    0xA2CE      0x5345""".splitlines():
        bcode, ucode = list(map(eval, m.split()))
        big5decmap[bcode >> 8][bcode & 0xff] = ucode
    # encoding map
    big5encmap = {}
    for c1, m in list(big5decmap.items()):
        for c2, code in list(m.items()):
            big5encmap.setdefault(code >> 8, {})
            if code & 0xff not in big5encmap[code >> 8]:
                big5encmap[code >> 8][code & 0xff] = c1 << 8 | c2
    # fix unicode->big5 priority for the above-mentioned duplicate characters
    big5encmap[0xFF][0x0F] = 0xA241
    big5encmap[0xFF][0x3C] = 0xA242
    big5encmap[0x53][0x41] = 0xA451
    big5encmap[0x53][0x45] = 0xA4CA

    return big5decmap, big5encmap


def load_cp950_map():
    mapfile = open_mapping_file('python-mappings/CP950.TXT', MAPPINGS_CP950)
    with mapfile:
        cp950decmap = loadmap(mapfile)
    cp950encmap = {}
    for c1, m in list(cp950decmap.items()):
        for c2, code in list(m.items()):
            cp950encmap.setdefault(code >> 8, {})
            if code & 0xff not in cp950encmap[code >> 8]:
                cp950encmap[code >> 8][code & 0xff] = c1 << 8 | c2
    # fix unicode->big5 duplicated mapping priority
    cp950encmap[0x53][0x41] = 0xA451
    cp950encmap[0x53][0x45] = 0xA4CA
    return cp950decmap, cp950encmap


def main_tw():
    big5decmap, big5encmap = load_big5_map()
    cp950decmap, cp950encmap = load_cp950_map()

    # CP950 extends Big5, and the codec can use the Big5 lookup tables
    # for most entries. So the CP950 tables should only include entries
    # that are not in Big5:
    for c1, m in list(cp950encmap.items()):
        for c2, code in list(m.items()):
            if (c1 in big5encmap and c2 in big5encmap[c1]
                    and big5encmap[c1][c2] == code):
                del cp950encmap[c1][c2]
    for c1, m in list(cp950decmap.items()):
        for c2, code in list(m.items()):
            if (c1 in big5decmap and c2 in big5decmap[c1]
                    and big5decmap[c1][c2] == code):
                del cp950decmap[c1][c2]

    with open('mappings_tw.h', 'w') as fp:
        print_autogen(fp, os.path.basename(__file__))
        write_big5_maps(fp, 'BIG5', 'big5', big5decmap, big5encmap)
        write_big5_maps(fp, 'CP950', 'cp950ext', cp950decmap, cp950encmap)


def write_big5_maps(fp, display_name, table_name, decode_map, encode_map):
    print(f'Generating {display_name} decode map...')
    writer = DecodeMapWriter(fp, table_name, decode_map)
    writer.update_decode_map(BIG5_C1, BIG5_C2)
    writer.generate()
    print(f'Generating {display_name} encode map...')
    writer = EncodeMapWriter(fp, table_name, encode_map)
    writer.generate()


class HintsWriter:
    filler_class = BufferedFiller

    def __init__(self, fp, prefix, isbmpmap):
        self.fp = fp
        self.prefix = prefix
        self.isbmpmap = isbmpmap
        self.filler = self.filler_class()

    def fillhints(self, hintfrom, hintto):
        name = f'{self.prefix}_phint_{hintfrom}'
        self.fp.write(f'static const unsigned char {name}[] = {{\n')
        for msbcode in range(hintfrom, hintto+1, 8):
            v = 0
            for c in range(msbcode, msbcode+8):
                v |= self.isbmpmap.get(c, 0) << (c - msbcode)
            self.filler.write('%d,' % v)
        self.filler.printout(self.fp)
        self.fp.write('};\n\n')


def main_hkscs():
    filename = f'python-mappings/hkscs-{HKSCS_VERSION}-big5-iso.txt'
    with open_mapping_file(filename, MAPPINGS_HKSCS) as f:
        table = parse_hkscs_map(f)
    hkscsdecmap, hkscsencmap_bmp, hkscsencmap_nonbmp, isbmpmap = (
        make_hkscs_map(table)
    )
    with open('mappings_hk.h', 'w') as fp:
        print('Generating BIG5HKSCS decode map...')
        print_autogen(fp, os.path.basename(__file__))
        writer = DecodeMapWriter(fp, 'big5hkscs', hkscsdecmap)
        writer.update_decode_map(BIG5HKSCS_C1, BIG5HKSCS_C2)
        writer.generate()

        print('Generating BIG5HKSCS decode map Unicode plane hints...')
        writer = HintsWriter(fp, 'big5hkscs', isbmpmap)
        writer.fillhints(bh2s(0x8740), bh2s(0xa0fe))
        writer.fillhints(bh2s(0xc6a1), bh2s(0xc8fe))
        writer.fillhints(bh2s(0xf9d6), bh2s(0xfefe))

        print('Generating BIG5HKSCS encode map (BMP)...')
        writer = EncodeMapWriter(fp, 'big5hkscs_bmp', hkscsencmap_bmp)
        writer.generate()

        print('Generating BIG5HKSCS encode map (non-BMP)...')
        writer = EncodeMapWriter(fp, 'big5hkscs_nonbmp', hkscsencmap_nonbmp)
        writer.generate()


if __name__ == '__main__':
    main_tw()
    main_hkscs()
