"""mailerdaemon - classes to parse mailer-daemon messages"""

import string
import rfc822
import regex
import os
import sys

Unparseable = 'mailerdaemon.Unparseable'

class ErrorMessage(rfc822.Message):
    def __init__(self, fp):
	rfc822.Message.__init__(self, fp)

    def is_warning(self):
	sub = self.getheader('Subject')
	if not sub:
	    return 0
	sub = string.lower(sub)
	if sub == 'waiting mail': return 1
	if string.find(sub, 'warning') >= 0: return 1
	return 0

    def get_errors(self):
	for p in EMPARSERS:
	    self.rewindbody()
	    try:
		return p(self.fp)
	    except Unparseable:
		pass
	raise Unparseable

sendmail_pattern = regex.compile('[0-9][0-9][0-9] ')
def emparse_sendmail(fp):
    while 1:
	line = fp.readline()
	if not line:
	    raise Unparseable
	line = line[:-1]

	# Check that we're not in the returned message yet
	if string.lower(line)[:5] == 'from:':
	    raise Unparseable
	line = string.split(line)
	if len(line) > 3 and \
	   ((line[0] == '-----' and line[1] == 'Transcript') or
	    (line[0] == '---' and line[1] == 'The' and
	     line[2] == 'transcript') or
	    (line[0] == 'While' and line[1] == 'talking' and line[2] == 'to')):
	    # Yes, found it!
	    break

    errors = []
    found_a_line = 0
    warnings = 0
    while 1:
	line = fp.readline()
	if not line:
	    break
	line = line[:-1]
	if not line:
	    continue
	found_a_line = 1
	if sendmail_pattern.match(line) == 4:
	    # Yes, an error/warning line. Ignore 4, remember 5, stop on rest
	    if line[0] == '5':
		errors.append(line)
	    elif line[0] == '4':
		warnings = 1
	    else:
		raise Unparseable
	line = string.split(line)
	if line and line[0][:3] == '---':
	    break
    # Empty transcripts are ok, others without an error are not.
    if found_a_line and not (errors or warnings):
	raise Unparseable
    return errors
    
def emparse_cts(fp):
    while 1:
	line = fp.readline()
	if not line:
	    raise Unparseable
	line = line[:-1]

	# Check that we're not in the returned message yet
	if string.lower(line)[:5] == 'from:':
	    raise Unparseable
	line = string.split(line)
	if len(line) > 3 and line[0][:2] == '|-' and line[1] == 'Failed' \
	   and line[2] == 'addresses':
	    # Yes, found it!
	    break

    errors = []
    while 1:
	line = fp.readline()
	if not line:
	    break
	line = line[:-1]
	if not line:
	    continue
	if line[:2] == '|-':
	    break
	errors.append(line)
    return errors

def emparse_aol(fp):
    while 1:
	line = fp.readline()
	if not line:
	    raise Unparseable
	line = line[:-1]
	if line:
	    break
    exp = 'The mail you sent could not be delivered to:'
    if line[:len(exp)] != exp:
	raise Unparseable
    errors = []
    while 1:
	line = fp.readline()
	if sendmail_pattern.match(line) == 4:
	    # Yes, an error/warning line. Ignore 4, remember 5, stop on rest
	    if line[0] == '5':
		errors.append(line)
	    elif line[0] != '4':
		raise Unparseable
	elif line == '\n':
	    break
	else:
	    raise Unparseable
    return errors
    
def emparse_compuserve(fp):
    while 1:
	line = fp.readline()
	if not line:
	    raise Unparseable
	line = line[:-1]
	if line:
	    break
    exp = 'Your message could not be delivered for the following reason:'
    if line[:len(exp)] != exp:
	raise Unparseable
    errors = []
    while 1:
	line = fp.readline()
	if not line: break
	if line[:3] == '---': break
	line = line[:-1]
	if not line: continue
	if line == 'Please resend your message at a later time.':
	    continue
	line = 'Compuserve: ' + line
	errors.append(line)
    return errors

prov_pattern = regex.compile('.* | \(.*\)')

def emparse_providence(fp):
    while 1:
	line = fp.readline()
	if not line:
	    raise Unparseable
	line = line[:-1]

	# Check that we're not in the returned message yet
	if string.lower(line)[:5] == 'from:':
	    raise Unparseable
	exp = 'The following errors occurred'
	if line[:len(exp)] == exp:
	    break

    errors = []
    while 1:
	line = fp.readline()
	if not line:
	    break
	line = line[:-1]
	if not line:
	    continue
	if line[:4] == '----':
	    break
	if prov_pattern.match(line) > 0:
	    errors.append(prov_pattern.group(1))

    if not errors:
	raise Unparseable
    return errors

EMPARSERS = [emparse_sendmail, emparse_aol, emparse_cts, emparse_compuserve,
	     emparse_providence]

def parsedir(dir, modify):
    os.chdir(dir)
    files = os.listdir('.')
    pat = regex.compile('^[0-9]*$')
    errordict = {}
    errorlast = {}
    nok = nwarn = nbad = 0
    
    for fn in files:
	if pat.match(fn) > 0:
	    # Ok, so it's a numeric filename. Lets try to parse it.
	    fp = open(fn)
	    m = ErrorMessage(fp)
	    sender = m.getaddr('From')
	    print '%s\t%-40s\t'%(fn, sender[1]),
	    
	    if m.is_warning():
		print 'warning only'
		nwarn = nwarn + 1
		if modify:
		    os.unlink(fn)
		continue

	    try:
		errors = m.get_errors()
	    except Unparseable:
		print '** Not parseable'
		nbad = nbad + 1
		continue
	    print len(errors), 'errors'

	    # Remember them
	    for e in errors:
		if not errordict.has_key(e):
		    errordict[e] = 1
		else:
		    errordict[e] = errordict[e] + 1
		errorlast[e] = fn

	    nok = nok + 1
	    if modify:
		os.unlink(fn)

    print '--------------'
    print nok, 'files parsed,',nwarn,'files warning-only,',
    print nbad,'files unparseable'
    print '--------------'
    for e in errordict.keys():
	print errordict[e], '\t', errorlast[e], '\t', e

def main():
    modify = 0
    if len(sys.argv) > 1 and sys.argv[1] == '-d':
	modify = 1
	del sys.argv[1]
    if len(sys.argv) > 1:
	for folder in sys.argv[1:]:
	    parsedir(folder, modify)
    else:
	parsedir('/ufs/jack/Mail/errorsinbox', modify)

if __name__ == '__main__' or sys.argv[0] == __name__:
    main()
