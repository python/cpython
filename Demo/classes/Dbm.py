# A wrapper around the (optional) built-in class dbm, supporting keys
# and values of almost any type instead of just string.
# (Actually, this works only for keys and values that can be read back
# correctly after being converted to a string.)


def opendbm(filename, mode, perm):
	return Dbm().init(filename, mode, perm)


class Dbm:

	def init(self, filename, mode, perm):
		import dbm
		self.db = dbm.open(filename, mode, perm)
		return self

	def __repr__(self):
		s = ''
		for key in self.keys():
			t = `key` + ': ' + `self[key]`
			if s: t = t + ', '
			s = s + t
		return '{' + s + '}'

	def __len__(self):
		return len(self.db)

	def __getitem__(self, key):
		return eval(self.db[`key`])

	def __setitem__(self, key, value):
		self.db[`key`] = `value`

	def __delitem__(self, key):
		del self.db[`key`]

	def keys(self):
		res = []
		for key in self.db.keys():
			res.append(eval(key))
		return res

	def has_key(self, key):
		return self.db.has_key(`key`)


def test():
	d = opendbm('@dbm', 'rw', 0666)
	print d
	while 1:
		try:
			key = eval(raw_input('key: '))
			if d.has_key(key):
				value = d[key]
				print 'currently:', value
			value = eval(raw_input('value: '))
			if value == None:
				del d[key]
			else:
				d[key] = value
		except KeyboardInterrupt:
			print ''
			print d
		except EOFError:
			print '[eof]'
			break
	print d


test()
