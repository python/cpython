"""curses.wrapper

Contains one function, wrapper(), which runs another function which
should be the rest of your curses-based application.  If the
application raises an exception, wrapper() will restore the terminal
to a sane state so you can read the resulting traceback.

"""

import sys, curses

def wrapper(func, *rest):
    """Wrapper function that initializes curses and calls another function,
    restoring normal keyboard/screen behavior on error.
    The callable object 'func' is then passed the main window 'stdscr'
    as its first argument, followed by any other arguments passed to
    wrapper().
    """
    
    try:
	# Initialize curses
	stdscr=curses.initscr()
	# Turn off echoing of keys, and enter cbreak mode,
	# where no buffering is performed on keyboard input
	curses.noecho() ; curses.cbreak()

	# In keypad mode, escape sequences for special keys
	# (like the cursor keys) will be interpreted and
	# a special value like curses.KEY_LEFT will be returned
        stdscr.keypad(1)

	return apply(func, (stdscr,) + rest)

    finally:
	# Restore the terminal to a sane state on the way out.
	stdscr.keypad(0)
	curses.echo() ; curses.nocbreak()
	curses.endwin()

