"""mailerdaemon - classes to parse mailer-daemon messages"""

import string
import rfc822
import calendar
import re
import os
import sys

Unparseable = 'mailerdaemon.Unparseable'

class ErrorMessage(rfc822.Message):
    def __init__(self, fp):
        rfc822.Message.__init__(self, fp)
        self.sub = ''

    def is_warning(self):
        sub = self.getheader('Subject')
        if not sub:
            return 0
        sub = string.lower(sub)
        if sub[:12] == 'waiting mail': return 1
        if string.find(sub, 'warning') >= 0: return 1
        self.sub = sub
        return 0

    def get_errors(self):
        for p in EMPARSERS:
            self.rewindbody()
            try:
                return p(self.fp, self.sub)
            except Unparseable:
                pass
        raise Unparseable

# List of re's or tuples of re's.
# If a re, it should contain at least a group (?P<email>...) which
# should refer to the email address.  The re can also contain a group
# (?P<reason>...) which should refer to the reason (error message).
# If no reason is present, the emparse_list_reason list is used to
# find a reason.
# If a tuple, the tuple should contain 2 re's.  The first re finds a
# location, the second re is repeated one or more times to find
# multiple email addresses.  The second re is matched (not searched)
# where the previous match ended.
# The re's are compiled using the re module.
emparse_list_list = [
    'error: (?P<reason>unresolvable): (?P<email>.+)',
    ('----- The following addresses had permanent fatal errors -----\n',
     '(?P<email>[^ \n].*)\n( .*\n)?'),
    'remote execution.*\n.*rmail (?P<email>.+)',
    ('The following recipients did not receive your message:\n\n',
     ' +(?P<email>.*)\n(The following recipients did not receive your message:\n\n)?'),
    '------- Failure Reasons  --------\n\n(?P<reason>.*)\n(?P<email>.*)',
    '^<(?P<email>.*)>:\n(?P<reason>.*)',
    '^(?P<reason>User mailbox exceeds allowed size): (?P<email>.+)',
    '^5\\d{2} <(?P<email>[^\n>]+)>\\.\\.\\. (?P<reason>.+)',
    '^Original-Recipient: rfc822;(?P<email>.*)',
    '^did not reach the following recipient\\(s\\):\n\n(?P<email>.*) on .*\n +(?P<reason>.*)',
    '^ <(?P<email>[^\n>]+)> \\.\\.\\. (?P<reason>.*)',
    '^Report on your message to: (?P<email>.*)\nReason: (?P<reason>.*)',
    '^Your message was not delivered to +(?P<email>.*)\n +for the following reason:\n +(?P<reason>.*)',
    '^ was not +(?P<email>[^ \n].*?) *\n.*\n.*\n.*\n because:.*\n +(?P<reason>[^ \n].*?) *\n',
    ]
# compile the re's in the list and store them in-place.
for i in range(len(emparse_list_list)):
    x = emparse_list_list[i]
    if type(x) is type(''):
        x = re.compile(x, re.MULTILINE)
    else:
        xl = []
        for x in x:
            xl.append(re.compile(x, re.MULTILINE))
        x = tuple(xl)
        del xl
    emparse_list_list[i] = x
    del x
del i

# list of re's used to find reasons (error messages).
# if a string, "<>" is replaced by a copy of the email address.
# The expressions are searched for in order.  After the first match,
# no more expressions are searched for.  So, order is important.
emparse_list_reason = [
    r'^5\d{2} <>\.\.\. (?P<reason>.*)',
    '<>\.\.\. (?P<reason>.*)',
    re.compile(r'^<<< 5\d{2} (?P<reason>.*)', re.MULTILINE),
    re.compile('===== stderr was =====\nrmail: (?P<reason>.*)'),
    re.compile('^Diagnostic-Code: (?P<reason>.*)', re.MULTILINE),
    ]
emparse_list_from = re.compile('^From:', re.IGNORECASE|re.MULTILINE)
def emparse_list(fp, sub):
    data = fp.read()
    res = emparse_list_from.search(data)
    if res is None:
        from_index = len(data)
    else:
        from_index = res.start(0)
    errors = []
    emails = []
    reason = None
    for regexp in emparse_list_list:
        if type(regexp) is type(()):
            res = regexp[0].search(data, 0, from_index)
            if res is not None:
                try:
                    reason = res.group('reason')
                except IndexError:
                    pass
                while 1:
                    res = regexp[1].match(data, res.end(0), from_index)
                    if res is None:
                        break
                    emails.append(res.group('email'))
                break
        else:
            res = regexp.search(data, 0, from_index)
            if res is not None:
                emails.append(res.group('email'))
                try:
                    reason = res.group('reason')
                except IndexError:
                    pass
                break
    if not emails:
        raise Unparseable
    if not reason:
        reason = sub
        if reason[:15] == 'returned mail: ':
            reason = reason[15:]
        for regexp in emparse_list_reason:
            if type(regexp) is type(''):
                for i in range(len(emails)-1,-1,-1):
                    email = emails[i]
                    exp = re.compile(string.join(string.split(regexp, '<>'), re.escape(email)), re.MULTILINE)
                    res = exp.search(data)
                    if res is not None:
                        errors.append(string.join(string.split(string.strip(email)+': '+res.group('reason'))))
                        del emails[i]
                continue
            res = regexp.search(data)
            if res is not None:
                reason = res.group('reason')
                break
    for email in emails:
        errors.append(string.join(string.split(string.strip(email)+': '+reason)))
    return errors

EMPARSERS = [emparse_list, ]

def sort_numeric(a, b):
    a = string.atoi(a)
    b = string.atoi(b)
    if a < b: return -1
    elif a > b: return 1
    else: return 0

def parsedir(dir, modify):
    os.chdir(dir)
    pat = re.compile('^[0-9]*$')
    errordict = {}
    errorfirst = {}
    errorlast = {}
    nok = nwarn = nbad = 0

    # find all numeric file names and sort them
    files = filter(lambda fn, pat=pat: pat.match(fn) is not None, os.listdir('.'))
    files.sort(sort_numeric)
    
    for fn in files:
        # Lets try to parse the file.
        fp = open(fn)
        m = ErrorMessage(fp)
        sender = m.getaddr('From')
        print '%s\t%-40s\t'%(fn, sender[1]),

        if m.is_warning():
            fp.close()
            print 'warning only'
            nwarn = nwarn + 1
            if modify:
                os.rename(fn, ','+fn)
##              os.unlink(fn)
            continue

        try:
            errors = m.get_errors()
        except Unparseable:
            print '** Not parseable'
            nbad = nbad + 1
            fp.close()
            continue
        print len(errors), 'errors'

        # Remember them
        for e in errors:
            try:
                mm, dd = m.getdate('date')[1:1+2]
                date = '%s %02d' % (calendar.month_abbr[mm], dd)
            except:
                date = '??????'
            if not errordict.has_key(e):
                errordict[e] = 1
                errorfirst[e] = '%s (%s)' % (fn, date)
            else:
                errordict[e] = errordict[e] + 1
            errorlast[e] = '%s (%s)' % (fn, date)

        fp.close()
        nok = nok + 1
        if modify:
            os.rename(fn, ','+fn)
##          os.unlink(fn)

    print '--------------'
    print nok, 'files parsed,',nwarn,'files warning-only,',
    print nbad,'files unparseable'
    print '--------------'
    list = []
    for e in errordict.keys():
        list.append((errordict[e], errorfirst[e], errorlast[e], e))
    list.sort()
    for num, first, last, e in list:
        print '%d %s - %s\t%s' % (num, first, last, e)

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
