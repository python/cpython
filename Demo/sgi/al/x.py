# Demonstrate that rsh exits when the remote end closes std{in,out,err}.
# rsh voorn exec /ufs/guido/bin/sgi/python /ufs/guido/mm/demo/audio/x.py

print 'hoi!'
import sys
sys.stdin.close()
sys.stdout.close()
sys.stderr.close()
import time
time.sleep(5)
sys.stdout = open('@', 'w')
sys.stdout.write('Hello\n')
