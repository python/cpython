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
		start_min, start_sec, start_frame, \
			total_min, total_sec, total_frame = info[i]
		print 'Track', z(i+1),
		print z(start_min) + ':' + z(start_sec) + ':' + z(start_frame),
		print z(total_min) + ':' + z(total_sec) + ':' + z(total_frame)

def z(n):
	s = `n`
	return '0' * (2 - len(s)) + s
