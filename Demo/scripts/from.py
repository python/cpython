#! /usr/local/python

# Print From and Subject of messages in $MAIL.
# Extension to multiple mailboxes and other bells & whistles are left
# as exercises for the reader.

import posix

# Open mailbox file.  Exits with exception when this fails.

mail = open(posix.environ['MAIL'], 'r')

while 1:
	line = mail.readline()
	if not line: break # EOF
	if line[:5] = 'From ':
		# Start of message found
		print line[:-1],
		while 1:
			line = mail.readline()
			if not line: break # EOF
			if line = '\n': break # Blank line ends headers
			if line[:8] = 'Subject:':
				print `line[9:-1]`,
		print
