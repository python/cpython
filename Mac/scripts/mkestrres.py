"""Parse sys/errno.h and Errors.h and create Estr resource"""

import regex
import macfs
import string
import Res
import os

READ = 1
WRITE = 2
smAllScripts = -3

ERRNO_PROG="#define[ \t]+" \
		   "\([A-Z0-9a-z_]+\)" \
		   "[ \t]+" \
		   "\([0-9]+\)" \
		   "[ \t]*/\*[ \t]*" \
		   "\(.*\)" \
		   "[ \t]*\*/"
		   
ERRORS_PROG="[ \t]*" \
			"\([A-Z0-9a-z_]+\)" \
			"[ \t]*=[ \t]*" \
			"\([-0-9]+\)" \
			"[, \t]*/\*[ \t]*" \
			"\(.*\)" \
			"[ \t]*\*/"

def Pstring(str):
	if len(str) > 255:
		raise ValueError, 'String too large'
	return chr(len(str))+str
	
def writeestr(dst, edict):
	"""Create Estr resource file given a dictionary of errors."""
	
	os.unlink(dst.as_pathname())
	Res.FSpCreateResFile(dst, 'RSED', 'rsrc', smAllScripts)
	output = Res.FSpOpenResFile(dst, WRITE)
	Res.UseResFile(output)
	for num in edict.keys():
		res = Res.Resource(Pstring(edict[num][0]))
		res.AddResource('Estr', num, '')
		res.WriteResource()
	Res.CloseResFile(output)
	
def writepython(fp, dict):
	k = dict.keys()
	k.sort()
	for i in k:
		fp.write("%s\t=\t%d\t#%s\n"%(dict[i][1], i, dict[i][0]))
	

def parse_errno_h(fp, dict):
	errno_prog = regex.compile(ERRNO_PROG)
	for line in fp.readlines():
		if errno_prog.match(line) > 0:
			number = string.atoi(errno_prog.group(2))
			name = errno_prog.group(1)
			desc = string.strip(errno_prog.group(3))
			
			if not dict.has_key(number):
				dict[number] = desc, name
			else:
				print 'DUPLICATE', number
				print '\t', dict[number]
				print '\t', (desc, name)
								
def parse_errors_h(fp, dict):
	errno_prog = regex.compile(ERRORS_PROG)
	for line in fp.readlines():
		if errno_prog.match(line) > 0:
			number = string.atoi(errno_prog.group(2))
			name = errno_prog.group(1)
			desc = string.strip(errno_prog.group(3))
			if number > 0: continue
			
			if not dict.has_key(number):
				dict[number] = desc, name
			else:
				print 'DUPLICATE', number
				print '\t', dict[number]
				print '\t', (desc, name)
			
def main():
	dict = {}
	fss, ok = macfs.PromptGetFile("Where is errno.h?")
	if not ok: return
	fp = open(fss.as_pathname())
	parse_errno_h(fp, dict)
	fp.close()
	
	fss, ok = macfs.PromptGetFile("Select 2nd errno.h or cancel")
	if not ok: return
	fp = open(fss.as_pathname())
	parse_errno_h(fp, dict)
	fp.close()
	
	fss, ok = macfs.PromptGetFile("Where is Errors.h?")
	if not ok: return
	fp = open(fss.as_pathname())
	parse_errors_h(fp, dict)
	fp.close()
	
	if not dict:
		return
		
	fss, ok = macfs.StandardPutFile("Resource output file?", "errors.rsrc")
	if ok:
		writeestr(fss, dict)
	
	fss, ok = macfs.StandardPutFile("Python output file?", "macerrors.py")
	if ok:
		fp = open(fss.as_pathname(), "w")
		writepython(fp, dict)
		fp.close()
		fss.SetCreatorType('Pyth', 'TEXT')

	fss, ok = macfs.StandardPutFile("Text output file?", "errors.txt")
	if ok:
		fp = open(fss.as_pathname(), "w")
		
		k = dict.keys()
		k.sort()
		for i in k:
			fp.write("%d\t%s\t%s\n"%(i, dict[i][1], dict[i][0]))
		fp.close()

	
if __name__ == '__main__':
	main()
	
