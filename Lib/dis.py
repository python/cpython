# Disassembler

import sys
import string

def dis():
	tb = sys.last_traceback
	while tb.tb_next: tb = tb.tb_next
	distb(tb)

def distb(tb):
	disassemble(tb.tb_frame.f_code, tb.tb_lasti)

def disco(co):
	disassemble(co, -1)

def disassemble(co, lasti):
	code = co.co_code
	labels = findlabels(code)
	n = len(code)
	i = 0
	while i < n:
		c = code[i]
		op = ord(c)
		if op = SET_LINENO and i > 0: print # Extra blank line
		if i = lasti: print '-->',
		else: print '   ',
		if i in labels: print '>>',
		else: print '  ',
		print string.rjust(`i`, 4),
		print string.ljust(opname[op], 15),
		i = i+1
		if op >= HAVE_ARGUMENT:
			oparg = ord(code[i]) + ord(code[i+1])*256
			i = i+2
			print string.rjust(`oparg`, 5),
			if op in hasconst:
				print '(' + `co.co_consts[oparg]` + ')',
			elif op in hasname:
				print '(' + co.co_names[oparg] + ')',
			elif op in hasjrel:
				print '(to ' + `i + oparg` + ')',
		print

def findlabels(code):
	labels = []
	n = len(code)
	i = 0
	while i < n:
		c = code[i]
		op = ord(c)
		i = i+1
		if op >= HAVE_ARGUMENT:
			oparg = ord(code[i]) + ord(code[i+1])*256
			i = i+2
			label = -1
			if op in hasjrel:
				label = i+oparg
			elif op in hasjabs:
				label = oparg
			if label >= 0:
				if label not in labels:
					labels.append(label)
	return labels

hasconst = []
hasname = []
hasjrel = []
hasjabs = []

opname = range(256)
for op in opname: opname[op] = '<' + `op` + '>'

def def_op(name, op):
	opname[op] = name

def name_op(name, op):
	opname[op] = name
	hasname.append(op)

def jrel_op(name, op):
	opname[op] = name
	hasjrel.append(op)

def jabs_op(name, op):
	opname[op] = name
	hasjabs.append(op)

# Instruction opcodes for compiled code

def_op('STOP_CODE', 0)
def_op('POP_TOP', 1)
def_op('ROT_TWO', 2)
def_op('ROT_THREE', 3)
def_op('DUP_TOP', 4)

def_op('UNARY_POSITIVE', 10)
def_op('UNARY_NEGATIVE', 11)
def_op('UNARY_NOT', 12)
def_op('UNARY_CONVERT', 13)
def_op('UNARY_CALL', 14)

def_op('BINARY_MULTIPLY', 20)
def_op('BINARY_DIVIDE', 21)
def_op('BINARY_MODULO', 22)
def_op('BINARY_ADD', 23)
def_op('BINARY_SUBTRACT', 24)
def_op('BINARY_SUBSCR', 25)
def_op('BINARY_CALL', 26)

def_op('SLICE+0', 30)
def_op('SLICE+1', 31)
def_op('SLICE+2', 32)
def_op('SLICE+3', 33)

def_op('STORE_SLICE+0', 40)
def_op('STORE_SLICE+1', 41)
def_op('STORE_SLICE+2', 42)
def_op('STORE_SLICE+3', 43)

def_op('DELETE_SLICE+0', 50)
def_op('DELETE_SLICE+1', 51)
def_op('DELETE_SLICE+2', 52)
def_op('DELETE_SLICE+3', 53)

def_op('STORE_SUBSCR', 60)
def_op('DELETE_SUBSCR', 61)

def_op('PRINT_EXPR', 70)
def_op('PRINT_ITEM', 71)
def_op('PRINT_NEWLINE', 72)

def_op('BREAK_LOOP', 80)
def_op('RAISE_EXCEPTION', 81)
def_op('LOAD_LOCALS', 82)
def_op('RETURN_VALUE', 83)
def_op('REQUIRE_ARGS', 84)
def_op('REFUSE_ARGS', 85)
def_op('BUILD_FUNCTION', 86)
def_op('POP_BLOCK', 87)
def_op('END_FINALLY', 88)
def_op('BUILD_CLASS', 89)

HAVE_ARGUMENT = 90		# Opcodes from here have an argument: 

name_op('STORE_NAME', 90)	# Index in name list 
name_op('DELETE_NAME', 91)	# "" 
def_op('UNPACK_TUPLE', 92)	# Number of tuple items 
def_op('UNPACK_LIST', 93)	# Number of list items 
# unused:		94
name_op('STORE_ATTR', 95)	# Index in name list 
name_op('DELETE_ATTR', 96)	# "" 

def_op('LOAD_CONST', 100)	# Index in const list 
hasconst.append(100)
name_op('LOAD_NAME', 101)	# Index in name list 
def_op('BUILD_TUPLE', 102)	# Number of tuple items 
def_op('BUILD_LIST', 103)	# Number of list items 
def_op('BUILD_MAP', 104)	# Always zero for now 
name_op('LOAD_ATTR', 105)	# Index in name list 
def_op('COMPARE_OP', 106)	# Comparison operator 
name_op('IMPORT_NAME', 107)	# Index in name list 
name_op('IMPORT_FROM', 108)	# Index in name list 

jrel_op('JUMP_FORWARD', 110)	# Number of bytes to skip 
jrel_op('JUMP_IF_FALSE', 111)	# "" 
jrel_op('JUMP_IF_TRUE', 112)	# "" 
jabs_op('JUMP_ABSOLUTE', 113)	# Target byte offset from beginning of code 
jrel_op('FOR_LOOP', 114)	# Number of bytes to skip 

jrel_op('SETUP_LOOP', 120)	# Distance to target address
jrel_op('SETUP_EXCEPT', 121)	# ""
jrel_op('SETUP_FINALLY', 122)	# ""

def_op('SET_LINENO', 127)	# Current line number
SET_LINENO = 127
