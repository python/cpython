#
# A VCR index.
#
import os
import string

error='VcrIndex.error'
VERSION_STRING='#!VcrIndex 1.1\n'
PREV_VERSION_STRING='#!VcrIndex 1.0\n'

class VcrIndex:
	def __init__(self, name):
		self.curmovie = None
		self.curscene = None
		self.modified = 0
		self.filename = name
		self.basename = os.path.basename(name)
		self.editable = []
		if not name:
			self.movies = {}
			return
		try:
			fp = open(name, 'r')
		except IOError:
			self.movies = {}
			return
		header = fp.readline()
		if header == PREV_VERSION_STRING:
			print 'Converting old-format database...'
			data = fp.read(100000)
			self.movies = eval(data)
			for m in self.movies.keys():
				d = self.movies[m]
				newd = {}
				for s in d.keys():
					newsd = {}
					newsd['START'] = d[s]
					if s == 'START':
						s = '-ALL-'
					newd[s] = newsd
				
				self.movies[m] = newd
			print 'Done.'
			return
		if header <> VERSION_STRING:
			print 'VcrIndex: incorrect version string:', header
			self.movies = {}
			return
		data = fp.read(100000)
		self.movies = eval(data)
	#
	# Save database to given file (or same file as read from if no
	# filename given).
	# 
	def save(self, name):
		if not name:
			name = self.filename
		if not name:
			raise error, 'No filename given'
		self.filename = name
		bupname = name + '~'
		try:
			os.unlink(bupname)
		except os.error:
			pass
		try:
			os.rename(name, bupname)
		except os.error:
			pass
		fp = open(name, 'w')
		data = str(self.movies)
		fp.write(VERSION_STRING)
		fp.write(data)
		fp.write('\n')
		fp.close()
		self.modified = 0
	#
	# Get a list of movie names in tape order
	#
	def get_movienames(self):
		names = self.movies.keys()
		sorted = []
		for name in names:
			sorted.append(self.movies[name]['-ALL-']['START'], name)
		sorted.sort()
		rv = []
		for pos, name in sorted:
			rv.append(name)
		return rv
	#
	# Get a list of scene names in tape order
	#
	def get_scenenames(self):
		if not self.curmovie:
			return []
		scenedict = self.movies[self.curmovie]
		names = scenedict.keys()
		sorted = []
		for name in names:
			sorted.append(scenedict[name], name)
		sorted.sort()
		rv = []
		for pos, name in sorted:
			rv.append(name)
		return rv
	#
	# Get a list of scene ids (format '00:02:32:12 name') in tape order
	#
	def get_sceneids(self):
		if not self.curmovie:
			return []
		scenedict = self.movies[self.curmovie]
		names = scenedict.keys()
		sorted = []
		for name in names:
			sorted.append(scenedict[name]['START'], name)
		sorted.sort()
		rv = []
		for pos, name in sorted:
			str = '%02d:%02d:%02d:%02d ' % pos
			rv.append(str + name)
		return rv
	#
	# Does a movie exist?
	#
	def movie_exists(self, name):
		return self.movies.has_key(name)
	#
	# Select a movie.
	#
	def movie_select(self, name):
		if not self.movies.has_key(name):
			raise error, 'No such movie: '+name
		self.curmovie = name
		self.curscene = None
	#
	# Get movie dictionary, or raise an error if no current movie.
	#
	def _getmoviedict(self):
		if not self.curmovie:
			raise error, 'No current movie'
		return self.movies[self.curmovie]
	#
	# Rename a movie.
	#
	def movie_rename(self, newname):
		scenedict = self._getmoviedict()
		if self.movie_exists(newname):
			raise error, 'Movie already exists: '+newname
		del self.movies[self.curmovie]
		self.movies[newname] = scenedict
		self.curmovie = newname
		self.modified = 1
	#
	# Copy a movie.
	#
	def movie_copy(self, newname):
		scenedict = self._getmoviedict()
		if self.movie_exists(newname):
			raise error, 'Movie already exists: '+newname
		newdict = {}
		for k in scenedict.keys():
			olddata = scenedict[k]
			newdata = {}
			for i in olddata.keys():
				newdata[i] = olddata[i]
			newdict[k] = newdata
		self.movies[newname] = newdict
		self.curmovie = newname
		self.modified = 1
	#
	# Delete a movie.
	#
	def movie_delete(self):
		if not self.curmovie:
			raise error, 'No current movie'
		del self.movies[self.curmovie]
		self.curmovie = None
		self.curscene = None
		self.modified = 1
	#
	# Create a new movie.
	#
	def movie_new(self, name, pos):
		if self.movie_exists(name):
			raise error, 'Movie already exists: '+name
		newdict = {}
		newsdict = {}
		newsdict['START'] = pos
		newdict['-ALL-'] = newsdict
		self.movies[name] = newdict
		self.curmovie = name
		self.curscene = None
		self.modified = 1
	#
	# Does a scene exist?
	#
	def scene_exists(self, name):
		scenedict = self._getmoviedict()
		return scenedict.has_key(name)
	#
	# Select a current scene.
	#
	def scene_select(self, name):
		scenedict = self._getmoviedict()
		if not scenedict.has_key(name):
			raise error, 'No such scene: '+name
		self.curscene = name
	#
	# Rename a scene.
	#
	def scene_rename(self, newname):
		scenedict = self._getmoviedict()
		if not self.curscene:
			raise error, 'No current scene'
		if scenedict.has_key(newname):
			raise error, 'Scene already exists: '+newname
		if self.curscene == '-ALL-':
			raise error, 'Cannot rename -ALL-'
		scenedict[newname] = scenedict[self.curscene]
		del scenedict[self.curscene]
		self.curscene = newname
		self.modified = 1
	#
	# Copy a scene.
	#
	def scene_copy(self, newname):
		scenedict = self._getmoviedict()
		if not self.curscene:
			raise error, 'No current scene'
		if scenedict.has_key(newname):
			raise error, 'Scene already exists: '+newname
		scenedict[newname] = scenedict[self.curscene]
		self.curscene = newname
		self.modified = 1
	#
	# Delete a scene.
	#
	def scene_delete(self):
		scenedict = self._getmoviedict()
		if not self.curscene:
			raise error, 'No current scene'
		if self.curscene == '-ALL-':
			raise error, 'Cannot delete -ALL-'
		del scenedict[self.curscene]
		self.curscene = None
		self.modified = 1
	#
	# Add a new scene.
	#
	def scene_new(self, newname, pos):
		scenedict = self._getmoviedict()
		if scenedict.has_key(newname):
			raise error, 'Scene already exists: '+newname
		newdict = {}
		newdict['START'] = pos
		scenedict[newname] = newdict
		self.curscene = newname
		self.modified = 1
	#
	# Get scene data.
	#
	def _getscenedata(self):
		scenedict = self._getmoviedict()
		if not self.curscene:
			raise error, 'No current scene'
		return scenedict[self.curscene]
	#
	# Data manipulation routines.
	#
	def pos_get(self):
		return self._getscenedata()['START']
	#
	def pos_set(self, pos):
		data = self._getscenedata()
		data['START'] = pos
		self.modified = 1
	#
	def comment_get(self):
		data = self._getscenedata()
		if data.has_key('COMMENT'):
			return data['COMMENT']
		else:
			return ''
	#
	def comment_set(self, comment):
		data = self._getscenedata()
		data['COMMENT'] = comment
		self.modified = 1
	#
	# Get the scene id of the current scene.
	#
	def get_cursceneid(self):
		pos = self._getscenedata()['START']
		str = '%02d:%02d:%02d:%02d ' % pos
		return str + self.curscene
	#
	# Convert a scene id to a scene name.
	#
	def scene_id2name(self, id):
		pos = string.find(id, ' ')
		if pos <= 0:
			raise error, 'Not a scene id: '+id
		return id[pos+1:]
	#
	# Select a scene given a position.
	#
	def pos_select(self, pos):
		prevmovie = None
		movies = self.get_movienames()
		for movie in movies:
			mpos = self.movies[movie]['-ALL-']['START']
			if mpos > pos:
				break
			prevmovie = movie
		if not prevmovie:
			raise error, 'Scene before BOT'
			
		self.movie_select(prevmovie)
		scenes = self.get_scenenames()
		scenedict = self._getmoviedict()
		prevscene = 'START'
		for scene in scenes:
			if scenedict[scene]['START'] > pos:
				break
			prevscene = scene
		self.scene_select(prevscene)
