class Set:
    def __init__(self):
	self.elts = {}
    def __len__(self):
        return len(self.elts)
    def add(self, elt):
	self.elts[elt] = elt
    def items(self):
	return self.elts.keys()
    def has_elt(self, elt):
	return self.elts.has_key(elt)
    def remove(self, elt):
        del self.elts[elt]

class Stack:
    def __init__(self):
	self.stack = []
	self.pop = self.stack.pop
    def __len__(self):
        return len(self.stack)
    def push(self, elt):
	self.stack.append(elt)
    def top(self):
	return self.stack[-1]
