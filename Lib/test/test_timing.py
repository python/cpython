from test.test_support import verbose
import timing

r = range(100000)
if verbose:
    print 'starting...'
timing.start()
for i in r:
    pass
timing.finish()
if verbose:
    print 'finished'

secs = timing.seconds()
milli = timing.milli()
micro = timing.micro()

if verbose:
    print 'seconds:', secs
    print 'milli  :', milli
    print 'micro  :', micro
