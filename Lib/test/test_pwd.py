import pwd
import string

verbose = 0
if __name__ == '__main__':
    verbose = 1

entries = pwd.getpwall()

for e in entries:
    name = e[0]
    uid = e[2]
    if verbose:
	print name, uid
    dbuid = pwd.getpwuid(uid)
    if dbuid[0] <> name:
	print 'Mismatch in pwd.getpwuid()'
    dbname = pwd.getpwnam(name)
    if dbname[2] <> uid:
	print 'Mismatch in pwd.getpwnam()'
    break

# try to get some errors
bynames = {}
byuids = {}
for n, p, u, g, gecos, d, s in entries:
    bynames[n] = u
    byuids[u] = n

allnames = bynames.keys()
namei = 0
fakename = allnames[namei]
while bynames.has_key(fakename):
    chars = map(None, fakename)
    for i in range(len(chars)):
	if chars[i] == 'z':
	    chars[i] = 'A'
	    break
	elif chars[i] == 'Z':
	    continue
	else:
	    chars[i] = chr(ord(chars[i]) + 1)
	    break
    else:
	namei = namei + 1
	try:
	    fakename = allnames[namei]
	except IndexError:
	    # should never happen... if so, just forget it
	    break
    fakename = string.join(map(None, chars), '')
    
try:
    pwd.getpwnam(fakename)
except KeyError:
    pass
else:
    print 'fakename', fakename, 'did not except pwd.getpwnam()'

uids = byuids.keys()
uids.sort()
uids.reverse()
fakeuid = uids[0] + 1

try:
    pwd.getpwuid(fakeuid)
except KeyError:
    pass
else:
    print 'fakeuid', fakeuid, 'did not except pwd.getpwuid()'
