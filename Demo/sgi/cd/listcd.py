# List track info from CD player.

import cd

def main():
	c = cd.open()
	info = []
	while 1:
		try:
			info.append(c.gettrackinfo(len(info) + 1))
		except RuntimeError:
			break
	for i in range(len(info)):
		start, total = info[i]
		print 'Track', zfill(i+1), triple(start), triple(total)

def triple((a, b, c)):
	return zfill(a) + ':' + zfill(b) + ':' + zfill(c)

def zfill(n):
	s = `n`
	return '0' * (2 - len(s)) + s

main()
