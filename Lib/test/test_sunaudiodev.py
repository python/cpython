from test_support import verbose, TestFailed
import sunaudiodev
import os

OS_AUDIO_DIRS = [
    '/usr/demo/SOUND/sounds/',		# Solaris 2.x
    ]


def play_sound_file(path):
    fp = open(path, 'r')
    data = fp.read()
    fp.close()
    a = sunaudiodev.open('w')
    a.write(data)
    a.close()

def test():
    for d in OS_AUDIO_DIRS:
	try:
	    files = os.listdir(d)
	    break
	except os.error:
	    pass
    else:
	# test couldn't be conducted on this platform
	raise ImportError
    for f in files:
	path = os.path.join(d, f)
	try:
	    play_sound_file(path)
	    break
	except:
	    pass
    else:
	raise TestFailed, "couldn't play any sounds"

test()
