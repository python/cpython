class Set:
    def __init__(self):
	self.elts = {}
    def add(self, elt):
	self.elts[elt] = elt
    def items(self):
	return self.elts.keys()
    def has_elt(self, elt):
	return self.elts.has_key(elt)

class Stack:
    def __init__(self):
	self.stack = []
	self.pop = self.stack.pop
    def push(self, elt):
	self.stack.append(elt)
    def top(self):
	return self.stack[-1]
