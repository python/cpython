import time
import al, AL
import string

dev = AL.DEFAULT_DEVICE

parameter_name = [ \
	'input source', 'left input atten', 'right input atten', \
	'input rate', 'output rate', \
	'left speaker gain', 'right speaker gain', \
	'input count', 'output count', 'unused count', \
	'sync input to aes', 'sync output to aes', \
	]
source_name = ['line', 'microphone', 'digital']

params = al.queryparams(dev)
for i in range(1, len(params), 2):
	params[i] = -1
while 1:
	time.millisleep(100)
	old = params[:]
	al.getparams(dev, params)
	if params <> old:
		for i in range(0, len(params), 2):
			if params[i+1] <> old[i+1]:
				name = parameter_name[params[i]]
				if params[i] == AL.INPUT_SOURCE:
					if 0 <= old[i+1] < len(source_name):
						oldval = source_name[old[i+1]]
					else:
						oldval = ''
					newval = source_name[params[i+1]]
				else:
					oldval = `old[i+1]`
					newval = `params[i+1]`
				print string.ljust(name, 20),
				print '(' + string.rjust(oldval, 10) + ')',
				print '-->',
				print string.rjust(newval, 10)
		print
