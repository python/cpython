_orig_open = open

class _BkFile:
	def __init__(self, file, mode, bufsize):
		import os
		self.__filename = file
		self.__backup = file + '~'
		try:
			os.unlink(self.__backup)
		except os.error:
			pass
		try:
			os.rename(file, self.__backup)
		except os.error:
			self.__backup = None
		self.__file = _orig_open(file, mode, bufsize)
		self.closed = self.__file.closed
		self.fileno = self.__file.fileno
		self.flush = self.__file.flush
		self.isatty = self.__file.isatty
		self.mode = self.__file.mode
		self.name = self.__file.name
		self.read = self.__file.read
		self.readinto = self.__file.readinto
		self.readline = self.__file.readline
		self.readlines = self.__file.readlines
		self.seek = self.__file.seek
		self.softspace = self.__file.softspace
		self.tell = self.__file.tell
		self.truncate = self.__file.truncate
		self.write = self.__file.write
		self.writelines = self.__file.writelines

	def close(self):
		self.__file.close()
		if self.__backup is None:
			return
		import filecmp
		if filecmp.cmp(self.__backup, self.__filename, shallow = 0):
			import os
			os.unlink(self.__filename)
			os.rename(self.__backup, self.__filename)

def open(file, mode = 'r', bufsize = -1):
	if 'w' not in mode:
		return _orig_open(file, mode, bufsize)
	return _BkFile(file, mode, bufsize)
