# Testing select module

def test():
	import select
	import os
	cmd = 'for i in 0 1 2 3 4 5 6 7 8 9; do date; sleep 3; done'
	p = os.popen(cmd, 'r')
	for tout in (0, 1, 2, 4, 8, 16) + (None,)*10:
		print 'timeout =', tout
		rfd, wfd, xfd = select.select([p], [], [], tout)
		print rfd, wfd, xfd
		if (rfd, wfd, xfd) == ([], [], []):
			continue
		if (rfd, wfd, xfd) == ([p], [], []):
			line = p.readline()
			print `line`
			if not line:
				print 'EOF'
				break
			continue
		print 'Heh?'

test()
