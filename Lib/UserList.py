# A more or less complete user-defined wrapper around list objects

class UserList:
	def __init__(self, *args):
		if len(args) > 1: raise TypeError, 'too many args'
		self.data = []
		if args:
			list = args[0]
			if type(list) == type(self.data):
				self.data[:] = list
			else:
				self.data[:] = list.data[:]
	def __repr__(self): return repr(self.data)
	def __cmp__(self, list):
		if type(list) == type(self.data):
			return cmp(self.data, list)
		else:
			return cmp(self.data, list.data)
	def __len__(self): return len(self.data)
	def __getitem__(self, i): return self.data[i]
	def __setitem__(self, i, item): self.data[i] = item
	def __delitem__(self, i): del self.data[i]
	def __getslice__(self, i, j):
		userlist = UserList()
		userlist.data[:] = self.data[i:j]
		return userlist
	def __setslice__(self, i, j, list):
		if type(list) == type(self.data):
			self.data[i:j] = list
		else:
			self.data[i:j] = list.data
	def __delslice__(self, i, j): del self.data[i:j]
	def append(self, item): self.data.append(item)
	def insert(self, i, item): self.data.insert(i, item)
	def remove(self, item): self.data.remove(item)
	def count(self, item): return self.data.count(item)
	def index(self, item): return self.data.index(item)
	def reverse(self): self.data.reverse()
	def sort(self, *args): apply(self.data.sort, args)
