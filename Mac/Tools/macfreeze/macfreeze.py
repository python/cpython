"""macfreeze - Main program and GUI

macfreeze allows you to turn Python scripts into fully self-contained
Mac applications, by including all the Python and C code needed in a single
executable. Like unix/windows freeze it can produce a config.c allowing you
to build the application with a development environment (CodeWarrior, to be
precise), but unlike the standard freeze it is also possible to create frozen
applications without a development environment, by glueing all the
shared libraries and extension modules needed together in a single
executable, using some Code Fragment Manager tricks."""

import macfs
import sys
import EasyDialogs
import string

import macfreezegui
import macmodulefinder

#
# Here are the macfreeze directives, used when freezing macfreeze itself
# (see directives.py for an explanation)
#
# macfreeze: path ::::Tools:freeze
# macfreeze: exclude win32api
#

def main():
	if len(sys.argv) < 2:
		gentype, program, output, debug = macfreezegui.dialog()
	elif len(sys.argv) == 2:
		gentype, program, output, debug = macfreezegui.dialog(sys.argv[1])
	else:
		EasyDialog.Message(
		  "Please pass a single script. Additional modules can be specified with directives")
		sys.exit(0)
	mustwait = process(gentype, program, output, debug=debug)
	if mustwait:
		sys.exit(1)

def process(gentype, program, output, modules=[], module_files=[], debug=0):
	try:
		module_dict = macmodulefinder.process(program, modules, module_files, debug)
	except macmodulefinder.Missing, arg:
		arg.sort()
		print '** Missing modules:', string.join(arg, ' ')
		sys.exit(1)
	#
	# And generate
	#
	if gentype == 'info':
		import macgen_info
		macgen_info.generate(output, module_dict)
		return 1		# So the user can inspect it
	elif gentype == 'source':
		import macgen_src
		warnings = macgen_src.generate(output, module_dict, debug)
		return warnings
	elif gentype == 'resource':
		import macgen_rsrc
		macgen_rsrc.generate(output, module_dict, debug)
		warnings = macgen_rsrc.warnings(module_dict)
		return warnings
	elif gentype == 'applet':
		import macgen_bin
		architecture = 'fat' # user should choose
		macgen_bin.generate(program, output, module_dict, architecture, debug)
	else:
		raise 'unknown gentype', gentype

if __name__ == '__main__':
	main()
