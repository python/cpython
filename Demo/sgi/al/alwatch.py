import time
import al
dev = 1
name = ['input source', 'left input atten', 'right input atten', \
	'input rate', 'output rate', \
	'left speaker gain', 'right speaker gain', \
	'input count', 'output count', 'unused count', \
	'sync input to aes', 'sync output to aes', \
	]
x = al.queryparams(dev)
al.getparams(dev, x)
while 1:
	time.millisleep(100)
	y = x[:]
	al.getparams(dev, x)
	if x <> y:
		for i in range(0, len(x), 2):
			if x[i+1] <> y[i+1]:
				print name[x[i]], ':', y[i+1], '-->', x[i+1]
