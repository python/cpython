from test_support import verbose, TestFailed
import nis

print 'nis.maps()'
try:
    # the following could fail if NIS isn't active
    maps = nis.maps()
except nis.error, msg:
    raise TestFailed, msg 

done = 0
for nismap in maps:
    if verbose:
        print nismap
    mapping = nis.cat(nismap)
    for k, v in mapping.items():
        if verbose:
            print '    ', k, v
        if not k:
            continue
        if nis.match(k, nismap) <> v:
            print "NIS match failed for key `%s' in map `%s'" % (k, nismap)
        else:
            # just test the one key, otherwise this test could take a
            # very long time
            done = 1
            break
    if done:
        break
