"""Utilities to get a password and/or the current user name.

getpass(prompt) - prompt for a password, with echo turned off
getuser() - get the user name from the environment or password database

Authors: Piers Lauder (original)
         Guido van Rossum (Windows support and cleanup)
"""


def getpass(prompt='Password: '):
	"""Prompt for a password, with echo turned off.

	Restore terminal settings at end.

	On Windows, this calls win_getpass(prompt) which uses the
	msvcrt module to get the same effect.

	"""

	import sys
	try:
		import termios, TERMIOS
	except ImportError:
		try:
			import msvcrt
		except ImportError:
			return default_getpass(prompt)
		else:
			return win_getpass(prompt)

	fd = sys.stdin.fileno()
	old = termios.tcgetattr(fd)	# a copy to save
	new = old[:]

	new[3] = new[3] & ~TERMIOS.ECHO	# 3 == 'lflags'
	try:
		termios.tcsetattr(fd, TERMIOS.TCSADRAIN, new)
		try: passwd = raw_input(prompt)
		except KeyboardInterrupt: passwd = None
	finally:
		termios.tcsetattr(fd, TERMIOS.TCSADRAIN, old)

	sys.stdout.write('\n')
	return passwd


def win_getpass(prompt='Password: '):
	"""Prompt for password with echo off, using Windows getch()."""
	import msvcrt
	for c in prompt:
		msvcrt.putch(c)
	pw = ""
	while 1:
		c = msvcrt.getch()
		if c == '\r' or c == '\n':
			break
		if c == '\b':
			pw = pw[:-1]
		else:
			pw = pw + c
	msvcrt.putch('\r')
	msvcrt.putch('\n')
	return pw


def default_getpass(prompt='Password: '):
	return raw_input(prompt)


def getuser():
	"""Get the username from the environment or password database.

	First try various environment variables, then the password
	database.  This works on Windows as long as USERNAME is set.

	"""

	import os

	for name in ('LOGNAME', 'USER', 'LNAME', 'USERNAME'):
		user = os.environ.get(name)
		if user:
			return user

	# If this fails, the exception will "explain" why
	import pwd
	return pwd.getpwuid(os.getuid())[0]
