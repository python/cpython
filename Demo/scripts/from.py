#! /usr/bin/env python

# Print From and Subject of messages in $MAIL.
# Extension to multiple mailboxes and other bells & whistles are left
# as exercises for the reader.

import sys, os

# Open mailbox file.  Exits with exception when this fails.

try:
    mailbox = os.environ['MAIL']
except (AttributeError, KeyError):
    sys.stderr.write('No environment variable $MAIL\n')
    sys.exit(2)

try:
    mail = open(mailbox)
except IOError:
    sys.exit('Cannot open mailbox file: ' + mailbox)

while 1:
    line = mail.readline()
    if not line:
        break # EOF
    if line.startswith('From '):
        # Start of message found
        print(line[:-1], end=' ')
        while 1:
            line = mail.readline()
            if not line or line == '\n':
                break
            if line.startswith('Subject: '):
                print(repr(line[9:-1]), end=' ')
        print()
