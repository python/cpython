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

import string, posix

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
				  (string.atoi(toc[i:i+2]),
				   string.atoi(toc[i+2:i+4]))))
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
		if posix.environ.has_key('CDDB_PATH'):
			path = posix.environ['CDDB_PATH']
			cddb_path = string.splitfields(path, ',')
		else:
			home = posix.environ['HOME']
			cddb_path = [home + '/' + _cddbrc]
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
		for dir in cddb_path:
			file = dir + '/' + self.id + '.rdb'
			try:
				f = open(file, 'r')
				self.file = file
				break
			except IOError:
				pass
		if not hasattr(self, 'file'):
			return
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
				try:
					trackno = string.atoi(name1[5:])
				except strings.atoi_error:
					print 'syntax error in ' + file
					continue
				if trackno > ntracks:
					print 'track number ' + `trackno` + \
						  ' in file ' + file + \
						  ' out of range'
					continue
				self.track[trackno] = value
		f.close()
		for i in range(2, len(self.track)):
			track = self.track[i]
			# if track title starts with `,', use initial part
			# of previous track's title
			if track[0] == ',':
				try:
					off = string.index(self.track[i - 1],
							   ',')
				except string.index_error:
					pass
				else:
					self.track[i] = self.track[i-1][:off] \
							+ track

	def write(self):
		import posixpath
		if posix.environ.has_key('CDDB_WRITE_DIR'):
			dir = posix.environ['CDDB_WRITE_DIR']
		else:
			dir = posix.environ['HOME'] + '/' + _cddbrc
		file = dir + '/' + self.id + '.rdb'
		if posixpath.exists(file):
			# make backup copy
			posix.rename(file, file + '~')
		f = open(file, 'w')
		f.write('album.title:\t' + self.title + '\n')
		f.write('album.artist:\t' + self.artist + '\n')
		f.write('album.toc:\t' + self.toc + '\n')
		prevpref = None
		for i in range(1, len(self.track)):
			track = self.track[i]
			try:
				off = string.index(track, ',')
			except string.index_error:
				prevpref = None
			else:
				if prevpref and track[:off] == prevpref:
					track = track[off:]
				else:
					prevpref = track[:off]
			f.write('track' + `i` + '.title:\t' + track + '\n')
		f.close()
