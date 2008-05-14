# This file implements a class which forms an interface to the .cddb
# directory that is maintained by SGI's cdman program.
#
# Usage is as follows:
#
# import readcd
# r = readcd.Readcd()
# c = Cddb(r.gettrackinfo())
#
# Now you can use c.artist, c.title and c.track[trackno] (where trackno
# starts at 1).  When the CD is not recognized, all values will be the empty
# string.
# It is also possible to set the above mentioned variables to new values.
# You can then use c.write() to write out the changed values to the
# .cdplayerrc file.
from warnings import warnpy3k
warnpy3k("the cddb module has been removed in Python 3.0", stacklevel=2)
del warnpy3k

import string, posix, os

_cddbrc = '.cddb'
_DB_ID_NTRACKS = 5
_dbid_map = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ@_=+abcdefghijklmnopqrstuvwxyz'
def _dbid(v):
    if v >= len(_dbid_map):
        return string.zfill(v, 2)
    else:
        return _dbid_map[v]

def tochash(toc):
    if type(toc) == type(''):
        tracklist = []
        for i in range(2, len(toc), 4):
            tracklist.append((None,
                      (int(toc[i:i+2]),
                       int(toc[i+2:i+4]))))
    else:
        tracklist = toc
    ntracks = len(tracklist)
    hash = _dbid((ntracks >> 4) & 0xF) + _dbid(ntracks & 0xF)
    if ntracks <= _DB_ID_NTRACKS:
        nidtracks = ntracks
    else:
        nidtracks = _DB_ID_NTRACKS - 1
        min = 0
        sec = 0
        for track in tracklist:
            start, length = track
            min = min + length[0]
            sec = sec + length[1]
        min = min + sec / 60
        sec = sec % 60
        hash = hash + _dbid(min) + _dbid(sec)
    for i in range(nidtracks):
        start, length = tracklist[i]
        hash = hash + _dbid(length[0]) + _dbid(length[1])
    return hash

class Cddb:
    def __init__(self, tracklist):
        if os.environ.has_key('CDDB_PATH'):
            path = os.environ['CDDB_PATH']
            cddb_path = path.split(',')
        else:
            home = os.environ['HOME']
            cddb_path = [home + '/' + _cddbrc]

        self._get_id(tracklist)

        for dir in cddb_path:
            file = dir + '/' + self.id + '.rdb'
            try:
                f = open(file, 'r')
                self.file = file
                break
            except IOError:
                pass
        ntracks = int(self.id[:2], 16)
        self.artist = ''
        self.title = ''
        self.track = [None] + [''] * ntracks
        self.trackartist = [None] + [''] * ntracks
        self.notes = []
        if not hasattr(self, 'file'):
            return
        import re
        reg = re.compile(r'^([^.]*)\.([^:]*):[\t ]+(.*)')
        while 1:
            line = f.readline()
            if not line:
                break
            match = reg.match(line)
            if not match:
                print 'syntax error in ' + file
                continue
            name1, name2, value = match.group(1, 2, 3)
            if name1 == 'album':
                if name2 == 'artist':
                    self.artist = value
                elif name2 == 'title':
                    self.title = value
                elif name2 == 'toc':
                    if not self.toc:
                        self.toc = value
                    if self.toc != value:
                        print 'toc\'s don\'t match'
                elif name2 == 'notes':
                    self.notes.append(value)
            elif name1[:5] == 'track':
                try:
                    trackno = int(name1[5:])
                except ValueError:
                    print 'syntax error in ' + file
                    continue
                if trackno > ntracks:
                    print 'track number %r in file %s out of range' % (trackno, file)
                    continue
                if name2 == 'title':
                    self.track[trackno] = value
                elif name2 == 'artist':
                    self.trackartist[trackno] = value
        f.close()
        for i in range(2, len(self.track)):
            track = self.track[i]
            # if track title starts with `,', use initial part
            # of previous track's title
            if track and track[0] == ',':
                try:
                    off = self.track[i - 1].index(',')
                except ValueError:
                    pass
                else:
                    self.track[i] = self.track[i-1][:off] \
                                    + track

    def _get_id(self, tracklist):
        # fill in self.id and self.toc.
        # if the argument is a string ending in .rdb, the part
        # upto the suffix is taken as the id.
        if type(tracklist) == type(''):
            if tracklist[-4:] == '.rdb':
                self.id = tracklist[:-4]
                self.toc = ''
                return
            t = []
            for i in range(2, len(tracklist), 4):
                t.append((None, \
                          (int(tracklist[i:i+2]), \
                           int(tracklist[i+2:i+4]))))
            tracklist = t
        ntracks = len(tracklist)
        self.id = _dbid((ntracks >> 4) & 0xF) + _dbid(ntracks & 0xF)
        if ntracks <= _DB_ID_NTRACKS:
            nidtracks = ntracks
        else:
            nidtracks = _DB_ID_NTRACKS - 1
            min = 0
            sec = 0
            for track in tracklist:
                start, length = track
                min = min + length[0]
                sec = sec + length[1]
            min = min + sec / 60
            sec = sec % 60
            self.id = self.id + _dbid(min) + _dbid(sec)
        for i in range(nidtracks):
            start, length = tracklist[i]
            self.id = self.id + _dbid(length[0]) + _dbid(length[1])
        self.toc = string.zfill(ntracks, 2)
        for track in tracklist:
            start, length = track
            self.toc = self.toc + string.zfill(length[0], 2) + \
                      string.zfill(length[1], 2)

    def write(self):
        import posixpath
        if os.environ.has_key('CDDB_WRITE_DIR'):
            dir = os.environ['CDDB_WRITE_DIR']
        else:
            dir = os.environ['HOME'] + '/' + _cddbrc
        file = dir + '/' + self.id + '.rdb'
        if posixpath.exists(file):
            # make backup copy
            posix.rename(file, file + '~')
        f = open(file, 'w')
        f.write('album.title:\t' + self.title + '\n')
        f.write('album.artist:\t' + self.artist + '\n')
        f.write('album.toc:\t' + self.toc + '\n')
        for note in self.notes:
            f.write('album.notes:\t' + note + '\n')
        prevpref = None
        for i in range(1, len(self.track)):
            if self.trackartist[i]:
                f.write('track%r.artist:\t%s\n' % (i, self.trackartist[i]))
            track = self.track[i]
            try:
                off = track.index(',')
            except ValueError:
                prevpref = None
            else:
                if prevpref and track[:off] == prevpref:
                    track = track[off:]
                else:
                    prevpref = track[:off]
            f.write('track%r.title:\t%s\n' % (i, track))
        f.close()
