import Disk_Copy
import macfs
import sys

talker = Disk_Copy.Disk_Copy(start=1)
talker.activate()
filespec = macfs.FSSpec('my disk image.img')
try:
    objref = talker.create('my disk image', saving_as=filespec, leave_image_mounted=1)
except Disk_Copy.Error, arg:
    print "ERROR: my disk image:", arg
else:
    print 'objref=', objref
print 'Type return to exit-'
sys.stdin.readline()
