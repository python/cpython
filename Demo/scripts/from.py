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
	mail = open(mailbox, 'r')
except IOError:
	sys.stderr.write('Cannot open mailbox file: ' + mailbox + '\n')
	sys.exit(2)

while 1:
	line = mail.readline()
	if not line: break # EOF
	if line[:5] == 'From ':
		# Start of message found
		print line[:-1],
		while 1:
			line = mail.readline()
			if not line: break # EOF
			if line == '\n': break # Blank line ends headers
			if line[:8] == 'Subject:':
				print `line[9:-1]`,
		print
