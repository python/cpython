# This file implements a class which forms an interface to the .cddb
# directory that is maintained by SGI's cdman program.
#
# Usage is as follows:
#
# import readcd
# r = readcd.Readcd().init()
# c = Cddb().init(r.gettrackinfo())
#
# Now you can use c.artist, c.title and c.track[trackno] (where trackno
# starts at 1).  When the CD is not recognized, all values will be the empty
# string.
# It is also possible to set the above mentioned variables to new values.
# You can then use c.write() to write out the changed values to the
# .cdplayerrc file.

import string

_cddbrc = '.cddb/'
_DB_ID_NTRACKS = 5
_dbid_map = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ@_=+abcdefghijklmnopqrstuvwxyz'
def _dbid(v):
	if v >= len(_dbid_map):
		return string.zfill(v, 2)
	else:
		return _dbid_map[v]

class Cddb():
	def init(self, tracklist):
		self.artist = ''
		self.title = ''
		if type(tracklist) == type(''):
			t = []
			for i in range(2, len(tracklist), 4):
				t.append((None, \
					  (string.atoi(tracklist[i:i+2]), \
					   string.atoi(tracklist[i+2:i+4]))))
			tracklist = t
		ntracks = len(tracklist)
		self.track = [None] + [''] * ntracks
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
		try:
			import posix
			file = posix.environ['HOME'] + '/' + _cddbrc + self.id + '.rdb'
			f = open(file, 'r')
		except IOError:
			return self
		import regex
		reg = regex.compile('^\\([^.]*\\)\\.\\([^:]*\\):\t\\(.*\\)')
		while 1:
			line = f.readline()
			if not line:
				break
			if reg.match(line) == -1:
				print 'syntax error in ' + file
				continue
			name1 = line[reg.regs[1][0]:reg.regs[1][1]]
			name2 = line[reg.regs[2][0]:reg.regs[2][1]]
			value = line[reg.regs[3][0]:reg.regs[3][1]]
			if name1 == 'album':
				if name2 == 'artist':
					self.artist = value
				elif name2 == 'title':
					self.title = value
				elif name2 == 'toc':
					if self.toc != value:
						print 'toc\'s don\'t match'
			elif name1[:5] == 'track':
				trackno = string.atoi(name1[5:])
				self.track[trackno] = value
		f.close()
		return self

	def write(self):
		import posix
		file = posix.environ['HOME'] + '/' + _cddbrc + self.id + '.rdb'
		f = open(file, 'w')
		f.write('album.title:\t' + self.title + '\n')
		f.write('album.artist:\t' + self.artist + '\n')
		f.write('album.toc:\t' + self.toc + '\n')
		for i in range(1, len(self.track)):
			f.write('track' + `i` + '.title:\t' + self.track[i] + '\n')
		f.close()
