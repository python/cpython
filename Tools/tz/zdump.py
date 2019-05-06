import sys
import os
import struct
from array import array
from collections import namedtuple
from datetime import datetime

ttinfo = namedtuple('ttinfo', ['tt_gmtoff', 'tt_isdst', 'tt_abbrind'])

class TZInfo:
    def __init__(self, transitions, type_indices, ttis, abbrs):
        self.transitions = transitions
        self.type_indices = type_indices
        self.ttis = ttis
        self.abbrs = abbrs

    @classmethod
    def fromfile(cls, fileobj):
        if fileobj.read(4).decode() != "TZif":
            raise ValueError("not a zoneinfo file")
        fileobj.seek(20)
        header = fileobj.read(24)
        tzh = (tzh_ttisgmtcnt, tzh_ttisstdcnt, tzh_leapcnt,
               tzh_timecnt, tzh_typecnt, tzh_charcnt) = struct.unpack(">6l", header)
        transitions = array('i')
        transitions.fromfile(fileobj, tzh_timecnt)
        if sys.byteorder != 'big':
            transitions.byteswap()

        type_indices = array('B')
        type_indices.fromfile(fileobj, tzh_timecnt)

        ttis = []
        for i in range(tzh_typecnt):
            ttis.append(ttinfo._make(struct.unpack(">lbb", fileobj.read(6))))

        abbrs = fileobj.read(tzh_charcnt)

        self = cls(transitions, type_indices, ttis, abbrs)
        self.tzh = tzh

        return self

    def dump(self, stream, start=None, end=None):
        for j, (trans, i) in enumerate(zip(self.transitions, self.type_indices)):
            utc = datetime.utcfromtimestamp(trans)
            tti = self.ttis[i]
            lmt = datetime.utcfromtimestamp(trans + tti.tt_gmtoff)
            abbrind = tti.tt_abbrind
            abbr = self.abbrs[abbrind:self.abbrs.find(0, abbrind)].decode()
            if j > 0:
                prev_tti = self.ttis[self.type_indices[j - 1]]
                shift = " %+g" % ((tti.tt_gmtoff - prev_tti.tt_gmtoff) / 3600)
            else:
                shift = ''
            print("%s UTC = %s %-5s isdst=%d" % (utc, lmt, abbr, tti[1]) + shift, file=stream)

    @classmethod
    def zonelist(cls, zonedir='/usr/share/zoneinfo'):
        zones = []
        for root, _, files in os.walk(zonedir):
            for f in files:
                p = os.path.join(root, f)
                with open(p, 'rb') as o:
                    magic =  o.read(4)
                if magic == b'TZif':
                    zones.append(p[len(zonedir) + 1:])
        return zones

if __name__ == '__main__':
    if len(sys.argv) < 2:
        zones = TZInfo.zonelist()
        for z in zones:
            print(z)
        sys.exit()
    filepath = sys.argv[1]
    if not filepath.startswith('/'):
        filepath = os.path.join('/usr/share/zoneinfo', filepath)
    with open(filepath, 'rb') as fileobj:
        tzi = TZInfo.fromfile(fileobj)
    tzi.dump(sys.stdout)
