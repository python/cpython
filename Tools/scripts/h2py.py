#! /usr/local/python

# Read #define's from stdin and translate to Python code on stdout.
# Very primitive: non-#define's are ignored, no check for valid Python
# syntax is made -- you will have to edit the output in most cases.

# XXX To do:
# - accept filename arguments
# - turn trailing C comments into Python comments
# - turn C string quotes into Python comments
# - turn C Boolean operators "&& || !" into Python "and or not"
# - what to do about #if(def)?
# - what to do about macros with parameters?
# - reject definitions with semicolons in them

import sys, regex, string

p_define = regex.compile('^#[\t ]*define[\t ]+\([a-zA-Z0-9_]+\)[\t ]+')

p_comment = regex.compile('/\*\([^*]+\|\*+[^/]\)*\(\*+/\)?')

def main():
	process(sys.stdin)

def process(fp):
	lineno = 0
	while 1:
		line = fp.readline()
		if not line: break
		lineno = lineno + 1
		if p_define.match(line) >= 0:
			# gobble up continuation lines
			while line[-2:] == '\\\n':
				nextline = fp.readline()
				if not nextline: break
				lineno = lineno + 1
				line = line + nextline
			regs = p_define.regs
			a, b = regs[1] # where the macro name is
			name = line[a:b]
			a, b = regs[0] # the whole match
			body = line[b:]
			# replace comments by spaces
			while p_comment.search(body) >= 0:
				a, b = p_comment.regs[0]
				body = body[:a] + ' ' + body[b:]
			print name, '=', string.strip(body)

main()
