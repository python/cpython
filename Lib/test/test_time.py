import time

time.altzone
time.clock()
t = time.time()
time.asctime(time.gmtime(t))
if time.ctime(t) <> time.asctime(time.localtime(t)):
    print 'time.ctime(t) <> time.asctime(time.localtime(t))'

time.daylight
if int(time.mktime(time.localtime(t))) <> int(t):
    print 'time.mktime(time.localtime(t)) <> t'

time.sleep(1.2)
tt = time.gmtime(t)
for directive in ('a', 'A', 'b', 'B', 'c', 'd', 'H', 'I',
		  'j', 'm', 'M', 'p', 'S',
		  'U', 'w', 'W', 'x', 'X', 'y', 'Y', 'Z', '%'):
    format = ' %' + directive
    try:
	time.strftime(format, tt)
    except ValueError:
	print 'conversion specifier:', format, ' failed.'

time.timezone
time.tzname

# expected errors
try:
    time.asctime(0)
except TypeError:
    pass

try:
    time.mktime((999999, 999999, 999999, 999999,
		 999999, 999999, 999999, 999999,
		 999999))
except OverflowError:
    pass
