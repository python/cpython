# 'grep'

import regexp
import string

def grep(expr, filename):
	prog = regexp.compile(expr)
	fp = open(filename, 'r')
	lineno = 0
	while 1:
		line = fp.readline()
		if not line: break
		lineno = lineno + 1
		res = prog.exec(line)
		if res:
			#print res
			start, end = res[0]
			if line[-1:] = '\n': line = line[:-1]
			prefix = string.rjust(`lineno`, 3) + ': '
			print prefix + line
			if 0:
				line = line[:start]
				if '\t' not in line:
					prefix = ' ' * (len(prefix) + start)
				else:
					prefix = ' ' * len(prefix)
					for c in line:
						if c <> '\t': c = ' '
						prefix = prefix + c
				if start = end: prefix = prefix + '\\'
				else: prefix = prefix + '^'*(end-start)
				print prefix
