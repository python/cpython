import nis

verbose = 0
if __name__ == '__main__':
    verbose = 1

maps = nis.maps()
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
