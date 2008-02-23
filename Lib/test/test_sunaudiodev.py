from test.test_support import findfile, TestFailed, TestSkipped
import sunaudiodev
import os

try:
    audiodev = os.environ["AUDIODEV"]
except KeyError:
    audiodev = "/dev/audio"

if not os.path.exists(audiodev):
    raise TestSkipped("no audio device found!")

def play_sound_file(path):
    fp = open(path, 'r')
    data = fp.read()
    fp.close()
    try:
        a = sunaudiodev.open('w')
    except sunaudiodev.error, msg:
        raise TestFailed, msg
    else:
        a.write(data)
        a.close()

def test():
    play_sound_file(findfile('audiotest.au'))

test()
