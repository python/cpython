#!/usr/bin/env python3

from Cocoa import NSMutableDictionary, NSMutableArray, NSString, NSDate, NSNumber
from Cocoa import NSPropertyListSerialization, NSPropertyListOpenStepFormat
from Cocoa import NSPropertyListXMLFormat_v1_0, NSPropertyListBinaryFormat_v1_0
from Cocoa import CFUUIDCreateFromString, NSNull, NSUUID, CFPropertyListCreateData
from Cocoa import NSURL

import datetime
from collections import OrderedDict
import binascii

FORMATS=[
#    ('openstep', NSPropertyListOpenStepFormat),
    ('plistlib.FMT_XML', NSPropertyListXMLFormat_v1_0),
    ('plistlib.FMT_BINARY', NSPropertyListBinaryFormat_v1_0),
]

def nsstr(value):
    return NSString.alloc().initWithString_(value)


def main():
    pl = OrderedDict()

    # Note: pl is an OrderedDict to control the order
    # of keys, and hence have some control on the structure
    # of the output file.
    # New keys should be added in alphabetical order.

    seconds = datetime.datetime(2004, 10, 26, 10, 33, 33, tzinfo=datetime.timezone(datetime.timedelta(0))).timestamp()
    pl[nsstr('aBigInt')] = 2 ** 63 - 44
    pl[nsstr('aBigInt2')] = NSNumber.numberWithUnsignedLongLong_(2 ** 63 + 44)
    pl[nsstr('aDate')] = NSDate.dateWithTimeIntervalSince1970_(seconds)

    pl[nsstr('aDict')] = d = OrderedDict()
    d[nsstr('aFalseValue')] = False
    d[nsstr('aTrueValue')] = True
    d[nsstr('aUnicodeValue')] = "M\xe4ssig, Ma\xdf"
    d[nsstr('anotherString')] = "<hello & 'hi' there!>"
    d[nsstr('deeperDict')] = dd = OrderedDict()
    dd[nsstr('a')] = 17
    dd[nsstr('b')] = 32.5
    dd[nsstr('c')] = a = NSMutableArray.alloc().init()
    a.append(1)
    a.append(2)
    a.append(nsstr('text'))

    pl[nsstr('aFloat')] = 0.5

    pl[nsstr('aList')] = a = NSMutableArray.alloc().init()
    a.append(nsstr('A'))
    a.append(nsstr('B'))
    a.append(12)
    a.append(32.5)
    aa = NSMutableArray.alloc().init()
    a.append(aa)
    aa.append(1)
    aa.append(2)
    aa.append(3)

    pl[nsstr('aNegativeBigInt')] = -80000000000
    pl[nsstr('aNegativeInt')] = -5
    pl[nsstr('aString')] = nsstr('Doodah')

    pl[nsstr('anEmptyDict')] = NSMutableDictionary.alloc().init()

    pl[nsstr('anEmptyList')] = NSMutableArray.alloc().init()

    pl[nsstr('anInt')] = 728

    pl[nsstr('nestedData')] = a = NSMutableArray.alloc().init()
    a.append(b'''<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03''')


    pl[nsstr('someData')] = b'<binary gunk>'

    pl[nsstr('someMoreData')] = b'''<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03<lots of binary gunk>\x00\x01\x02\x03'''

    pl[nsstr('\xc5benraa')] = nsstr("That was a unicode key.")

    print("TESTDATA={")
    for fmt_name, fmt_key in FORMATS:
        data, error = NSPropertyListSerialization.dataWithPropertyList_format_options_error_(
            pl, fmt_key, 0, None)
        if data is None:
            print("Cannot serialize", fmt_name, error)

        else:
            print("    %s: binascii.a2b_base64(b'''\n        %s'''),"%(fmt_name, _encode_base64(bytes(data)).decode('ascii')[:-1]))

    print("}")
    print()

def _encode_base64(s, maxlinelength=60):
    maxbinsize = (maxlinelength//4)*3
    pieces = []
    for i in range(0, len(s), maxbinsize):
        chunk = s[i : i + maxbinsize]
        pieces.append(binascii.b2a_base64(chunk))
    return b'        '.join(pieces)

main()
