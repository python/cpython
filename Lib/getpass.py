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

	try:
		fd = sys.stdin.fileno()
	except:
		return default_getpass(prompt)
	old = termios.tcgetattr(fd)	# a copy to save
	new = old[:]

	new[3] = new[3] & ~TERMIOS.ECHO	# 3 == 'lflags'
	try:
		termios.tcsetattr(fd, TERMIOS.TCSADRAIN, new)
		passwd = _raw_input(prompt)
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
		if c == '\003':
			raise KeyboardInterrupt
		if c == '\b':
			pw = pw[:-1]
		else:
			pw = pw + c
	msvcrt.putch('\r')
	msvcrt.putch('\n')
	return pw


def default_getpass(prompt='Password: '):
	return _raw_input(prompt)


def _raw_input(prompt=""):
	# A raw_input() replacement that doesn't save the string in the
	# GNU readline history.
	import sys
	prompt = str(prompt)
	if prompt:
		sys.stdout.write(prompt)
	line = sys.stdin.readline()
	if not line:
		raise EOFError
	if line[-1] == '\n':
		line = line[:-1]
	return line


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
