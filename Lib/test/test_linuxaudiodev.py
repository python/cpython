from test_support import verbose, findfile, TestFailed, TestSkipped
import linuxaudiodev
import errno
import os

def play_sound_file(path):
    fp = open(path, 'r')
    data = fp.read()
    fp.close()
    try:
        a = linuxaudiodev.open('w')
    except linuxaudiodev.error, msg:
	if msg[0] in (errno.EACCES, errno.ENODEV):
		raise TestSkipped, msg
        raise TestFailed, msg
    else:
        a.write(data)
        a.close()

def test():
    play_sound_file(findfile('audiotest.au'))

test()
