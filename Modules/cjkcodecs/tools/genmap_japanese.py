#
# genmap_ja_codecs.py: Japanese Codecs Map Generator
#
# Original Author:  Hye-Shik Chang <perky@FreeBSD.org>
# Modified Author:  Dong-hee Na <donghee.na92@gmail.com>
#
import os

from genmap_support import *


JISX0208_C1 = (0x21, 0x74)
JISX0208_C2 = (0x21, 0x7e)
JISX0212_C1 = (0x22, 0x6d)
JISX0212_C2 = (0x21, 0x7e)
JISX0213_C1 = (0x21, 0x7e)
JISX0213_C2 = (0x21, 0x7e)
CP932P0_C1  = (0x81, 0x81) # patches between shift-jis and cp932
CP932P0_C2  = (0x5f, 0xca)
CP932P1_C1  = (0x87, 0x87) # CP932 P1
CP932P1_C2  = (0x40, 0x9c)
CP932P2_C1  = (0xed, 0xfc) # CP932 P2
CP932P2_C2  = (0x40, 0xfc)

def loadmap_jisx0213(fo):
    decmap3, decmap4 = {}, {} # maps to BMP for level 3 and 4
    decmap3_2, decmap4_2 = {}, {} # maps to U+2xxxx for level 3 and 4
    decmap3_pair = {} # maps to BMP-pair for level 3
    for line in fo:
        line = line.split('#', 1)[0].strip()
        if not line or len(line.split()) < 2:
            continue

        row = line.split()
        loc = eval('0x' + row[0][2:])
        level = eval(row[0][0])
        m = None
        if len(row[1].split('+')) == 2: # single unicode
            uni = eval('0x' + row[1][2:])
            if level == 3:
                if uni < 0x10000:
                    m = decmap3
                elif 0x20000 <= uni < 0x30000:
                    uni -= 0x20000
                    m = decmap3_2
            elif level == 4:
                if uni < 0x10000:
                    m = decmap4
                elif 0x20000 <= uni < 0x30000:
                    uni -= 0x20000
                    m = decmap4_2
            m.setdefault((loc >> 8), {})
            m[(loc >> 8)][(loc & 0xff)] = uni
        else: # pair
            uniprefix = eval('0x' + row[1][2:6]) # body
            uni = eval('0x' + row[1][7:11]) # modifier
            if level != 3:
                raise ValueError("invalid map")
            decmap3_pair.setdefault(uniprefix, {})
            m = decmap3_pair[uniprefix]

        if m is None:
            raise ValueError("invalid map")
        m.setdefault((loc >> 8), {})
        m[(loc >> 8)][(loc & 0xff)] = uni

    return decmap3, decmap4, decmap3_2, decmap4_2, decmap3_pair

jisx0208file = open_mapping_file('data/JIS0208.TXT', 'http://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/JIS0208.TXT')
jisx0212file = open_mapping_file('data/JIS0212.TXT', 'http://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/JIS0212.TXT')
cp932file = open_mapping_file('data/CP932.TXT', 'http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP932.TXT')
jisx0213file = open_mapping_file('data/jisx0213-2004-std.txt', 'http://wakaba-web.hp.infoseek.co.jp/table/jisx0213-2004-std.txt')

print("Loading Mapping File...")

sjisdecmap = loadmap(jisx0208file, natcol=0, unicol=2)
jisx0208decmap = loadmap(jisx0208file, natcol=1, unicol=2)
jisx0212decmap = loadmap(jisx0212file)
cp932decmap = loadmap(cp932file)
jis3decmap, jis4decmap, jis3_2_decmap, jis4_2_decmap, jis3_pairdecmap = loadmap_jisx0213(jisx0213file)

if jis3decmap[0x21][0x24] != 0xff0c:
    raise SystemExit('Please adjust your JIS X 0213 map using jisx0213-2000-std.txt.diff')

sjisencmap, cp932encmap = {}, {}
jisx0208_0212encmap = {}
for c1, m in sjisdecmap.items():
    for c2, code in m.items():
        sjisencmap.setdefault(code >> 8, {})
        sjisencmap[code >> 8][code & 0xff] = c1 << 8 | c2
for c1, m in cp932decmap.items():
    for c2, code in m.items():
        cp932encmap.setdefault(code >> 8, {})
        if (code & 0xff) not in cp932encmap[code >> 8]:
            cp932encmap[code >> 8][code & 0xff] = c1 << 8 | c2
for c1, m in cp932encmap.copy().items():
    for c2, code in m.copy().items():
        if c1 in sjisencmap and c2 in sjisencmap[c1] and sjisencmap[c1][c2] == code:
            del cp932encmap[c1][c2]
            if not cp932encmap[c1]:
                del cp932encmap[c1]

jisx0213pairdecmap = {}
jisx0213pairencmap = []
for unibody, m1 in jis3_pairdecmap.items():
    for c1, m2 in m1.items():
        for c2, modifier in m2.items():
            jisx0213pairencmap.append((unibody, modifier, c1 << 8 | c2))
            jisx0213pairdecmap.setdefault(c1, {})
            jisx0213pairdecmap[c1][c2] = unibody << 16 | modifier

# Twinmap for both of JIS X 0208 (MSB unset) and JIS X 0212 (MSB set)
for c1, m in jisx0208decmap.items():
    for c2, code in m.items():
        jisx0208_0212encmap.setdefault(code >> 8, {})
        jisx0208_0212encmap[code >> 8][code & 0xff] = c1 << 8 | c2

for c1, m in jisx0212decmap.items():
    for c2, code in m.items():
        jisx0208_0212encmap.setdefault(code >> 8, {})
        if (code & 0xff) in jisx0208_0212encmap[code >> 8]:
            print("OOPS!!!", (code))
        jisx0208_0212encmap[code >> 8][code & 0xff] = 0x8000 | c1 << 8 | c2

jisx0213bmpencmap = {}
for c1, m in jis3decmap.copy().items():
    for c2, code in m.copy().items():
        if c1 in jisx0208decmap and c2 in jisx0208decmap[c1]:
            if code in jis3_pairdecmap:
                jisx0213bmpencmap[code >> 8][code & 0xff] = (0,) # pair
                jisx0213pairencmap.append((code, 0, c1 << 8 | c2))
            elif jisx0208decmap[c1][c2] == code:
                del jis3decmap[c1][c2]
                if not jis3decmap[c1]:
                    del jis3decmap[c1]
            else:
                raise ValueError("Difference between JIS X 0208 and JIS X 0213 Plane 1 is found.")
        else:
            jisx0213bmpencmap.setdefault(code >> 8, {})
            if code not in jis3_pairdecmap:
                jisx0213bmpencmap[code >> 8][code & 0xff] = c1 << 8 | c2
            else:
                jisx0213bmpencmap[code >> 8][code & 0xff] = (0,) # pair
                jisx0213pairencmap.append((code, 0, c1 << 8 | c2))

for c1, m in jis4decmap.items():
    for c2, code in m.items():
        jisx0213bmpencmap.setdefault(code >> 8, {})
        jisx0213bmpencmap[code >> 8][code & 0xff] = 0x8000 | c1 << 8 | c2

jisx0213empencmap = {}
for c1, m in jis3_2_decmap.items():
    for c2, code in m.items():
        jisx0213empencmap.setdefault(code >> 8, {})
        jisx0213empencmap[code >> 8][code & 0xff] = c1 << 8 | c2
for c1, m in jis4_2_decmap.items():
    for c2, code in m.items():
        jisx0213empencmap.setdefault(code >> 8, {})
        jisx0213empencmap[code >> 8][code & 0xff] = 0x8000 | c1 << 8 | c2

with open("mappings_jp.h", "w") as omap:
    print_autogen(omap, os.path.basename(__file__))
    print("Generating JIS X 0208 decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "jisx0208", JISX0208_C1, JISX0208_C2, jisx0208decmap)
    print_decmap(omap, filler, "jisx0208", jisx0208decmap)

    print("Generating JIS X 0212 decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "jisx0212", JISX0212_C1, JISX0212_C2, jisx0212decmap)
    print_decmap(omap, filler, "jisx0212", jisx0212decmap)

    print("Generating JIS X 0208 && JIS X 0212 encode map...")
    filler = BufferedFiller()
    genmap_encode(filler, "jisxcommon", jisx0208_0212encmap)
    print_encmap(omap, filler, "jisxcommon", jisx0208_0212encmap)

    print("Generating CP932 Extension decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "cp932ext", CP932P0_C1, CP932P0_C2, cp932decmap)
    genmap_decode(filler, "cp932ext", CP932P1_C1, CP932P1_C2, cp932decmap)
    genmap_decode(filler, "cp932ext", CP932P2_C1, CP932P2_C2, cp932decmap)
    print_decmap(omap, filler, "cp932ext", cp932decmap)

    print("Generating CP932 Extension encode map...")
    filler = BufferedFiller()
    genmap_encode(filler, "cp932ext", cp932encmap)
    print_encmap(omap, filler, "cp932ext", cp932encmap)

    print("Generating JIS X 0213 Plane 1 BMP decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "jisx0213_1_bmp", JISX0213_C1, JISX0213_C2, jis3decmap)
    print_decmap(omap, filler, "jisx0213_1_bmp", jis3decmap)

    print("Generating JIS X 0213 Plane 2 BMP decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "jisx0213_2_bmp", JISX0213_C1, JISX0213_C2, jis4decmap)
    print_decmap(omap, filler, "jisx0213_2_bmp", jis4decmap)

    print("Generating JIS X 0213 BMP encode map...")
    filler = BufferedFiller()
    genmap_encode(filler, "jisx0213_bmp", jisx0213bmpencmap)
    print_encmap(omap, filler, "jisx0213_bmp", jisx0213bmpencmap)

    print("Generating JIS X 0213 Plane 1 EMP decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "jisx0213_1_emp",
                  JISX0213_C1, JISX0213_C2, jis3_2_decmap)
    print_decmap(omap, filler, "jisx0213_1_emp", jis3_2_decmap)

    print("Generating JIS X 0213 Plane 2 EMP decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "jisx0213_2_emp",
                  JISX0213_C1, JISX0213_C2, jis4_2_decmap)
    print_decmap(omap, filler, "jisx0213_2_emp", jis4_2_decmap)

    print("Generating JIS X 0213 EMP encode map...")
    filler = BufferedFiller()
    genmap_encode(filler, "jisx0213_emp", jisx0213empencmap)
    print_encmap(omap, filler, "jisx0213_emp", jisx0213empencmap)

with open('mappings_jisx0213_pair.h', 'w') as omap:
    print_autogen(omap, os.path.basename(__file__))
    omap.write(f"#define JISX0213_ENCPAIRS {len(jisx0213pairencmap)}\n")
    omap.write("""\
#ifdef EXTERN_JISX0213_PAIR
static const struct widedbcs_index *jisx0213_pair_decmap;
static const struct pair_encodemap *jisx0213_pair_encmap;
#else
""")

    print("Generating JIS X 0213 unicode-pair decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "jisx0213_pair", JISX0213_C1, JISX0213_C2,
                  jisx0213pairdecmap, wide=1)
    print_decmap(omap, filler, "jisx0213_pair",
                jisx0213pairdecmap, wide=1)

    print("Generating JIS X 0213 unicode-pair encode map...")
    jisx0213pairencmap.sort()
    omap.write("static const struct pair_encodemap jisx0213_pair_encmap[JISX0213_ENCPAIRS] = {\n")
    filler = BufferedFiller()
    for body, modifier, jis in jisx0213pairencmap:
        filler.write('{', '0x%04x%04x,' % (body, modifier), '0x%04x' % jis, '},')
    filler.printout(omap)
    omap.write("};\n")
    omap.write("#endif\n")

print("Done!")
