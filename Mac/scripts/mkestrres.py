#
# Create 'Estr' resource from error dictionary
from Res import *
import Res
from Resources import *
import MacOS
import string

READ = 1
WRITE = 2
smAllScripts = -3

def Pstring(str):
	if len(str) > 255:
		raise ValueError, 'String too large'
	return chr(len(str))+str
	
def writeestr(dst, edict):
	"""Create Estr resource file given a dictionary of errors."""
	

	FSpCreateResFile(dst, 'RSED', 'rsrc', smAllScripts)
	output = FSpOpenResFile(dst, WRITE)
	UseResFile(output)
	for num in edict.keys():
		res = Resource(Pstring(edict[num]))
		res.AddResource('Estr', num, '')
		res.WriteResource()
	CloseResFile(output)
	
def parsefile(src):
	fp = open(src)
	lines = []
	while 1:
		x = fp.readline()
		if not x:
			break
		x = x[:-1]
		words = string.split(x)
		if x[0] in (' ', '\t'):
			# continuation line
			x = string.join(words)
			lines[-1] = lines[-1] + ' ' + x
		else:
			x = string.join(words)
			lines.append(x)
	dict = {}
	for line in lines:
		words = string.split(line)
		index = eval(words[0])
		if dict.has_key(index):
			print '** Duplicate key:', index
		x = string.join(words[2:])
		if not x:
			x = words[1]
		dict[index] = x
	return dict
			
	
if __name__ == '__main__':
	dict = parsefile('errors.txt')
	writeestr('errors.rsrc', dict)
