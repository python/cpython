:mod:`!curses` --- Terminal handling for character-cell displays
================================================================

.. module:: curses
   :synopsis: An interface to the curses library, providing portable
              terminal handling.

**Source code:** :source:`Lib/curses`

--------------

The :mod:`!curses` module provides an interface to the curses library, the
de-facto standard for portable advanced terminal handling.

While curses is most widely used in the Unix environment, versions are available
for Windows, DOS, and possibly other systems as well.  This extension module is
designed to match the API of ncurses, an open-source curses library hosted on
Linux and the BSD variants of Unix.

.. include:: ../includes/wasm-mobile-notavail.rst

.. include:: ../includes/optional-module.rst

.. availability:: Unix.

.. note::

   Whenever the documentation mentions a *character* it can be specified
   as an integer, a one-character Unicode string or a one-byte byte string.

   Whenever the documentation mentions a *character string* it can be specified
   as a Unicode string or a byte string.

.. seealso::

   Module :mod:`curses.ascii`
      Utilities for working with ASCII characters, regardless of your locale settings.

   Module :mod:`curses.panel`
      A panel stack extension that adds depth to  curses windows.

   Module :mod:`curses.textpad`
      Editable text widget for curses supporting  :program:`Emacs`\ -like bindings.

   :ref:`curses-howto`
      Tutorial material on using curses with Python, by Andrew Kuchling and Eric
      Raymond.


.. _curses-functions:

Functions
---------

The module :mod:`!curses` defines the following exception:


.. exception:: error

   Exception raised when a curses library function returns an error.

.. note::

   Whenever *x* or *y* arguments to a function or a method are optional, they
   default to the current cursor location. Whenever *attr* is optional, it defaults
   to :const:`A_NORMAL`.

The module :mod:`!curses` defines the following functions:


Initialization and termination
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. function:: initscr()

   Initialize the library. Return a :ref:`window <curses-window-objects>` object
   which represents the whole screen.

   See :func:`setupterm` for a caveat about calling it before this function.

   .. note::

      If there is an error opening the terminal, the underlying curses library may
      cause the interpreter to exit.

.. function:: endwin()

   De-initialize the library, and return terminal to normal status.

.. function:: isendwin()

   Return ``True`` if :func:`endwin` has been called (that is, the  curses library has
   been deinitialized).

.. function:: newterm(type=None, fd=None, infd=None, /)

   Initialize a new terminal in addition to the one initialized by
   :func:`initscr`,
   and return a :ref:`screen <curses-screen-objects>` for it.
   This allows a program to drive more than one terminal.

   *type* is the terminal name, as in :func:`setupterm`;
   if ``None``, the value of the :envvar:`TERM` environment variable is used.
   *fd* and *infd* are the output and input files for the terminal:
   either a file object or a file descriptor.
   They default to :data:`sys.stdout` and :data:`sys.stdin`.

   The new screen becomes the current one.
   Use :func:`set_term` to switch between screens.

   See :func:`setupterm` for a caveat about calling it before this function.

   .. versionadded:: next

.. function:: new_prescr()

   Return a new :ref:`screen <curses-screen-objects>`
   that can be used to call functions that affect global state
   before :func:`initscr` or :func:`newterm` is called.

   Availability: if the underlying curses library provides ``new_prescr()``.

   .. versionadded:: next

.. function:: set_term(screen, /)

   Make *screen*, a :ref:`screen <curses-screen-objects>` returned by
   :func:`newterm`, the current terminal,
   and return the previously current screen.
   Returns ``None`` if the previous screen was the one created by
   :func:`initscr`.

   .. versionadded:: next

.. function:: wrapper(func, /, *args, **kwargs)

   Initialize curses and call another callable object, *func*, which should be the
   rest of your curses-using application.  If the application raises an exception,
   this function will restore the terminal to a sane state before re-raising the
   exception and generating a traceback.  The callable object *func* is then passed
   the main window 'stdscr' as its first argument, followed by any other arguments
   passed to :func:`!wrapper`.  Before calling *func*, :func:`!wrapper` turns on
   cbreak mode, turns off echo, enables the terminal keypad, and initializes colors
   if the terminal has color support.  On exit (whether normally or by exception)
   it restores cooked mode, turns on echo, and disables the terminal keypad.


Terminal mode control
~~~~~~~~~~~~~~~~~~~~~

.. function:: def_prog_mode()

   Save the current terminal mode as the "program" mode, the mode when the running
   program is using curses.  (Its counterpart is the "shell" mode, for when the
   program is not in curses.)  Subsequent calls to :func:`reset_prog_mode` will
   restore this mode.

.. function:: def_shell_mode()

   Save the current terminal mode as the "shell" mode, the mode when the running
   program is not using curses.  (Its counterpart is the "program" mode, when the
   program is using curses capabilities.) Subsequent calls to
   :func:`reset_shell_mode` will restore this mode.

.. function:: reset_prog_mode()

   Restore the  terminal  to "program" mode, as previously saved  by
   :func:`def_prog_mode`.

.. function:: reset_shell_mode()

   Restore the  terminal  to "shell" mode, as previously saved  by
   :func:`def_shell_mode`.

.. function:: savetty()

   Save the current state of the terminal modes in a buffer, usable by
   :func:`resetty`.

.. function:: resetty()

   Restore the state of the terminal modes to what it was at the last call to
   :func:`savetty`.

.. function:: curs_set(visibility)

   Set the cursor state.  *visibility* can be set to ``0``, ``1``, or ``2``, for invisible,
   normal, or very visible.  If the terminal supports the visibility requested, return the
   previous cursor state; otherwise raise an exception.  On many
   terminals, the "visible" mode is an underline cursor and the "very visible" mode
   is a block cursor.

.. function:: getsyx()

   Return the current coordinates of the virtual screen cursor as a tuple
   ``(y, x)``.  If :meth:`leaveok <window.leaveok>` is currently ``True``, then return ``(-1, -1)``.

.. function:: setsyx(y, x)

   Set the virtual screen cursor to *y*, *x*. If *y* and *x* are both ``-1``, then
   :meth:`leaveok <window.leaveok>` is set ``True``.

.. function:: doupdate()

   Update the physical screen.  The curses library keeps two data structures, one
   representing the current physical screen contents and a virtual screen
   representing the desired next state.  The :func:`doupdate` function updates the
   physical screen to match the virtual screen.

   The virtual screen may be updated by a :meth:`~window.noutrefresh` call after write
   operations such as :meth:`~window.addstr` have been performed on a window.  The normal
   :meth:`~window.refresh` call is simply :meth:`!noutrefresh` followed by :func:`!doupdate`;
   if you have to update multiple windows, you can speed performance and perhaps
   reduce screen flicker by issuing :meth:`!noutrefresh` calls on all windows,
   followed by a single :func:`!doupdate`.


Input options
~~~~~~~~~~~~~

.. function:: cbreak()

   Enter cbreak mode.  In cbreak mode (sometimes called "rare" mode) normal tty
   line buffering is turned off and characters are available to be read one by one.
   However, unlike raw mode, special characters (interrupt, quit, suspend, and flow
   control) retain their effects on the tty driver and calling program.  Calling
   first :func:`raw` then :func:`cbreak` leaves the terminal in cbreak mode.

.. function:: nocbreak()

   Leave cbreak mode.  Return to normal "cooked" mode with line buffering.

.. function:: echo()

   Enter echo mode.  In echo mode, each character input is echoed to the screen as
   it is entered.

.. function:: noecho()

   Leave echo mode.  Echoing of input characters is turned off.

.. function:: raw()

   Enter raw mode.  In raw mode, normal line buffering and  processing of
   interrupt, quit, suspend, and flow control keys are turned off; characters are
   presented to curses input functions one by one.

.. function:: noraw()

   Leave raw mode. Return to normal "cooked" mode with line buffering.

.. function:: halfdelay(tenths)

   Used for half-delay mode, which is similar to cbreak mode in that characters
   typed by the user are immediately available to the program. However, after
   blocking for *tenths* tenths of seconds, raise an exception if nothing has
   been typed.  The value of *tenths* must be a number between ``1`` and ``255``.  Use
   :func:`nocbreak` to leave half-delay mode.

.. function:: meta(flag)

   If *flag* is ``True``, allow 8-bit characters to be input.  If
   *flag* is ``False``,  allow only 7-bit chars.

.. function:: nl(flag=True)

   Enter newline mode.  This mode translates the return key into newline on input,
   and translates newline into return and line-feed on output. Newline mode is
   initially on.

   If *flag* is ``False``, the effect is the same as calling :func:`nonl`.

.. function:: nonl()

   Leave newline mode.  Disable translation of return into newline on input, and
   disable low-level translation of newline into newline/return on output (but this
   does not change the behavior of ``addch('\n')``, which always does the
   equivalent of return and line feed on the virtual screen).  With translation
   off, curses can sometimes speed up vertical motion a little; also, it will be
   able to detect the return key on input.

.. function:: intrflush(flag)

   If *flag* is ``True``, pressing an interrupt key (interrupt, break, or quit)
   will flush all output in the terminal driver queue.  If *flag* is ``False``,
   no flushing is done.

.. function:: qiflush([flag])

   If *flag* is ``False``, the effect is the same as calling :func:`noqiflush`. If
   *flag* is ``True``, or no argument is provided, the queues will be flushed when
   these control characters are read.

.. function:: noqiflush()

   When the :func:`!noqiflush` routine is used, normal flush of input and output queues
   associated with the ``INTR``, ``QUIT`` and ``SUSP`` characters will not be done.  You may
   want to call :func:`!noqiflush` in a signal handler if you want output to
   continue as though the interrupt had not occurred, after the handler exits.

.. function:: typeahead(fd)

   Specify that the file descriptor *fd* be used for typeahead checking.  If *fd*
   is ``-1``, then no typeahead checking is done.

   The curses library does "line-breakout optimization" by looking for typeahead
   periodically while updating the screen.  If input is found, and it is coming
   from a tty, the current update is postponed until refresh or doupdate is called
   again, allowing faster response to commands typed in advance. This function
   allows specifying a different file descriptor for typeahead checking.

.. function:: is_cbreak()

   Return ``True`` if cbreak mode (see :func:`cbreak`) is enabled,
   ``False`` otherwise.
   Availability: ncurses 6.5 or later.

   .. versionadded:: next

.. function:: is_echo()

   Return ``True`` if echo mode (see :func:`echo`) is enabled,
   ``False`` otherwise.
   Availability: ncurses 6.5 or later.

   .. versionadded:: next

.. function:: is_nl()

   Return ``True`` if nl mode (see :func:`nl`) is enabled, ``False`` otherwise.
   Availability: ncurses 6.5 or later.

   .. versionadded:: next

.. function:: is_raw()

   Return ``True`` if raw mode (see :func:`raw`) is enabled,
   ``False`` otherwise.
   Availability: ncurses 6.5 or later.

   .. versionadded:: next


Keyboard input
~~~~~~~~~~~~~~

.. function:: ungetch(ch)

   Push *ch* so the next :meth:`~window.getch` or :meth:`~window.get_wch` will
   return it.

   *ch* may be an integer (a key code or character code), a byte, or a string of
   length 1.  A one-character string is pushed like :func:`unget_wch`; on a
   narrow build it must encode to a single byte.

   .. note::

      Only one *ch* can be pushed before :meth:`!getch` is called.

   .. versionchanged:: next
      A one-character string argument is no longer required to encode to a single
      byte, except on a narrow build.

.. function:: unget_wch(ch)

   Push *ch* so the next :meth:`~window.get_wch` will return it.

   .. note::

      Only one *ch* can be pushed before :meth:`!get_wch` is called.

   .. versionadded:: 3.3

   .. versionchanged:: next
      Also available on a narrow build, where *ch* must encode to a single byte
      (an 8-bit locale).

.. function:: flushinp()

   Flush all input buffers.  This throws away any  typeahead  that  has been typed
   by the user and has not yet been processed by the program.

.. function:: has_key(ch)

   Take a key value *ch*, and return ``True`` if the current terminal type recognizes
   a key with that value.

.. function:: keyname(k)

   Return the name of the key numbered *k* as a bytes object.  The name of a key generating printable
   ASCII character is the key's character.  The name of a control-key combination
   is a two-byte bytes object consisting of a caret (``b'^'``) followed by the corresponding
   printable ASCII character.  The name of an alt-key combination (128--255) is a
   bytes object consisting of the prefix ``b'M-'`` followed by the name of the corresponding
   ASCII character.

   Raise a :exc:`ValueError` if *k* is negative.

.. function:: define_key(definition, keycode)

   Define an escape sequence *definition*, a string, as a key that generates
   the key code *keycode*, so that :mod:`curses` interprets it like one of the
   keys predefined in the terminal database.

   If *definition* is ``None``, any existing binding for *keycode* is removed.
   If *keycode* is zero or negative, any existing binding for *definition* is
   removed.

   .. versionadded:: next

.. function:: key_defined(definition)

   Return the key code bound to the escape sequence *definition*, a string,
   ``0`` if no key code is bound to it, or ``-1`` if *definition* is a prefix
   of a longer bound sequence (and so is ambiguous).

   .. versionadded:: next

.. function:: keyok(keycode, enable)

   Enable (if *enable* is true) or disable (otherwise) interpretation of the
   key code *keycode*.  Unlike :meth:`window.keypad`, this affects a single
   key code rather than all of them.

   .. versionadded:: next


Mouse
~~~~~

.. function:: getmouse()

   After :meth:`~window.getch` returns :const:`KEY_MOUSE` to signal a mouse event, this
   method should be called to retrieve the queued mouse event, represented as a
   5-tuple ``(id, x, y, z, bstate)``. *id* is an ID value used to distinguish
   multiple devices, and *x*, *y*, *z* are the event's coordinates.  (*z* is
   currently unused.)  *bstate* is an integer value whose bits will be set to
   indicate the type of event, and will be the bitwise OR of one or more of the
   following constants, where *n* is the button number from 1 to 5:
   :const:`BUTTONn_PRESSED`, :const:`BUTTONn_RELEASED`, :const:`BUTTONn_CLICKED`,
   :const:`BUTTONn_DOUBLE_CLICKED`, :const:`BUTTONn_TRIPLE_CLICKED`,
   :const:`BUTTON_SHIFT`, :const:`BUTTON_CTRL`, :const:`BUTTON_ALT`.

   .. versionchanged:: 3.10
      The ``BUTTON5_*`` constants are now exposed if they are provided by the
      underlying curses library.

.. function:: ungetmouse(id, x, y, z, bstate)

   Push a :const:`KEY_MOUSE` event onto the input queue, associating the given
   state data with it.

.. function:: mousemask(mousemask)

   Set the mouse events to be reported, and return a tuple ``(availmask,
   oldmask)``.   *availmask* indicates which of the specified mouse events can be
   reported; on complete failure it returns ``0``.  *oldmask* is the previous value of
   the mouse event mask.  If this function is never called, no mouse
   events are ever reported.

.. function:: mouseinterval(interval)

   Set the maximum time in milliseconds that can elapse between press and release
   events in order for them to be recognized as a click, and return the previous
   interval value.  The default value is 166 milliseconds, or one sixth of a second.
   Use a negative *interval* to obtain the interval value without changing it.

.. function:: has_mouse()

   Return ``True`` if the mouse driver has been successfully initialized.

   Availability: if the underlying curses library provides ``has_mouse()``.

   .. versionadded:: next


Color
~~~~~

.. function:: start_color()

   Must be called if the programmer wants to use colors, and before any other color
   manipulation routine is called.  It is good practice to call this routine right
   after :func:`initscr`.

   :func:`start_color` initializes eight basic colors (black, red,  green, yellow,
   blue, magenta, cyan, and white), and two global variables in the :mod:`!curses`
   module, :const:`COLORS` and :const:`COLOR_PAIRS`, containing the maximum number
   of colors and color-pairs the terminal can support.  It also restores the colors
   on the terminal to the values they had when the terminal was just turned on.

.. function:: has_colors()

   Return ``True`` if the terminal can display colors; otherwise, return ``False``.

.. function:: has_extended_color_support()

   Return ``True`` if the module supports extended colors; otherwise, return
   ``False``. Extended color support allows more than 256 color pairs for
   terminals that support more than 16 colors (for example, xterm-256color).

   Extended color support requires ncurses version 6.1 or later.

   .. versionadded:: 3.10

.. function:: can_change_color()

   Return ``True`` or ``False``, depending on whether the programmer can change the colors
   displayed by the terminal.

.. function:: init_color(color_number, r, g, b)

   Change the definition of a color, taking the number of the color to be changed
   followed by three RGB values (for the amounts of red, green, and blue
   components).  The value of *color_number* must be between ``0`` and
   ``COLORS - 1``.  Each of *r*, *g*, *b*, must be a value between ``0`` and
   ``1000``.  When :func:`init_color` is used, all occurrences of that color on the
   screen immediately change to the new definition.  This function is a no-op on
   most terminals; it is active only if :func:`can_change_color` returns ``True``.

.. function:: color_content(color_number)

   Return the intensity of the red, green, and blue (RGB) components in the color
   *color_number*, which must be between ``0`` and ``COLORS - 1``.  Return a 3-tuple,
   containing the R,G,B values for the given color, which will be between
   ``0`` (no component) and ``1000`` (maximum amount of component).  Raise an
   exception if the color is not supported.

.. function:: init_pair(pair_number, fg, bg)

   Change the definition of a color-pair.  It takes three arguments: the number of
   the color-pair to be changed, the foreground color number, and the background
   color number.  The value of *pair_number* must be between ``1`` and
   ``COLOR_PAIRS - 1`` (the ``0`` color pair can only be changed by
   :func:`use_default_colors` and :func:`assume_default_colors`).
   The value of *fg* and *bg* arguments must be between ``0`` and
   ``COLORS - 1``, or, after calling :func:`!use_default_colors` or
   :func:`!assume_default_colors`, ``-1``.
   If the color-pair was previously initialized, the screen is
   refreshed and all occurrences of that color-pair are changed to the new
   definition.

.. function:: pair_content(pair_number)

   Return a tuple ``(fg, bg)`` containing the colors for the requested color pair.
   The value of *pair_number* must be between ``0`` and ``COLOR_PAIRS - 1``.

.. function:: color_pair(pair_number)

   Return the attribute value for displaying text in the specified color pair.
   Only color pairs that fit in the color-pair field of the returned value can
   be represented (usually the first 256); a larger *pair_number* raises
   :exc:`OverflowError` rather than being silently masked to a different pair.
   Use :meth:`~window.color_set` or :meth:`~window.attr_set` to display higher
   pairs.  This attribute value can be combined with :const:`A_STANDOUT`,
   :const:`A_REVERSE`, and the other :const:`!A_\*` attributes.
   :func:`pair_number` is the counterpart to this function.

.. function:: pair_number(attr)

   Return the number of the color-pair set by the attribute value *attr*.
   :func:`color_pair` is the counterpart to this function.

.. function:: use_default_colors()

   Equivalent to ``assume_default_colors(-1, -1)``.

.. function:: assume_default_colors(fg, bg, /)

   Allow use of default values for colors on terminals supporting this feature.
   Use this to support transparency in your application.

   * Assign terminal default foreground/background colors to color number ``-1``.
     So ``init_pair(x, COLOR_RED, -1)`` will initialize pair *x* as red
     on default background and ``init_pair(x, -1, COLOR_BLUE)`` will
     initialize pair *x* as default foreground on blue.

   * Change the definition of the color-pair ``0`` to ``(fg, bg)``.

   This is an ncurses extension.

   .. versionadded:: 3.14

.. function:: alloc_pair(fg, bg)

   Allocate a color pair for foreground color *fg* and background color *bg*,
   and return its number.  If a color pair for the same combination of colors
   already exists, return its number.  Otherwise allocate a new color pair and
   return its number.

   This function is only available if Python was built against a wide-character
   version of the underlying curses library with extended-color support (see
   :func:`has_extended_color_support`).

   .. versionadded:: next

.. function:: free_pair(pair_number)

   Free the color pair *pair_number*, which must have been allocated by
   :func:`alloc_pair`.  The pair must not be in use.

   This function is only available if Python was built against a wide-character
   version of the underlying curses library with extended-color support (see
   :func:`has_extended_color_support`).

   .. versionadded:: next

.. function:: find_pair(fg, bg)

   Return the number of a color pair for foreground color *fg* and background
   color *bg*, or ``-1`` if no color pair for this combination of colors has
   been allocated.

   This function is only available if Python was built against a wide-character
   version of the underlying curses library with extended-color support (see
   :func:`has_extended_color_support`).

   .. versionadded:: next

.. function:: reset_color_pairs()

   Discard all color-pair definitions, releasing the color pairs allocated by
   :func:`init_pair` and :func:`alloc_pair`.

   This function is only available if Python was built against a wide-character
   version of the underlying curses library with extended-color support (see
   :func:`has_extended_color_support`).

   .. versionadded:: next


Windows and pads
~~~~~~~~~~~~~~~~

.. function:: newwin(nlines, ncols)
              newwin(nlines, ncols, begin_y, begin_x)

   Return a new :ref:`window <curses-window-objects>`, whose left-upper corner
   is at  ``(begin_y, begin_x)``, and whose height/width is  *nlines*/*ncols*.

   By default, the window will extend from the  specified position to the lower
   right corner of the screen.

.. function:: newpad(nlines, ncols)

   Create and return a pointer to a new pad data structure with the given number
   of lines and columns.  Return a pad as a window object.

   A pad is like a window, except that it is not restricted by the screen size, and
   is not necessarily associated with a particular part of the screen.  Pads can be
   used when a large window is needed, and only a part of the window will be on the
   screen at one time.  Automatic refreshes of pads (such as from scrolling or
   echoing of input) do not occur.  The :meth:`~window.refresh` and :meth:`~window.noutrefresh`
   methods of a pad require 6 arguments to specify the part of the pad to be
   displayed and the location on the screen to be used for the display. The
   arguments are *pminrow*, *pmincol*, *sminrow*, *smincol*, *smaxrow*, *smaxcol*; the *p*
   arguments refer to the upper-left corner of the pad region to be displayed and
   the *s* arguments define a clipping box on the screen within which the pad region
   is to be displayed.


Soft labels
~~~~~~~~~~~

.. _curses-slk:

The following functions manage *soft-label keys*, a row of labels displayed
along the bottom line of the screen, typically used to label a row of function
keys.  :func:`slk_init` must be called before :func:`initscr` or
:func:`newterm`; it takes one screen line away from the standard window for the
labels.


.. function:: slk_init(fmt=0)

   Reserve a screen line for the soft labels and choose their layout.  *fmt*
   selects the arrangement: ``0`` for 3-2-3 (eight labels), ``1`` for 4-4
   (eight labels).  Where the underlying curses library supports them, ``2``
   gives 4-4-4 (twelve labels) and ``3`` gives 4-4-4 with an index line.

   Must be called before :func:`initscr` or :func:`newterm`.

   .. versionadded:: next


.. function:: slk_set(labnum, label, justify)

   Set the text of soft label number *labnum*, in the range ``1`` through ``8``
   (or ``12`` in a twelve-label layout).  *justify* controls how *label* is
   placed within the label: ``0`` for left, ``1`` for centered, ``2`` for right.

   .. versionadded:: next


.. function:: slk_label(labnum)

   Return the current text of soft label number *labnum*, justified as it was
   set, or an empty string if it has no label.

   .. versionadded:: next


.. function:: slk_refresh()

   Update the soft labels on the physical screen, like
   :meth:`~curses.window.refresh` for a window.

   .. versionadded:: next


.. function:: slk_noutrefresh()

   Update the soft labels on the virtual screen, like
   :meth:`window.noutrefresh`.  Use it together with :func:`doupdate` to batch
   screen updates.

   .. versionadded:: next


.. function:: slk_clear()

   Remove the soft labels from the screen.

   .. versionadded:: next


.. function:: slk_restore()

   Restore the soft labels to the screen after a :func:`slk_clear`.

   .. versionadded:: next


.. function:: slk_touch()

   Force all the soft labels to be redrawn by the next :func:`slk_refresh` or
   :func:`slk_noutrefresh`.

   .. versionadded:: next


.. function:: slk_attron(attr)
              slk_attroff(attr)
              slk_attrset(attr)

   Add, remove, or set the attributes used to display the soft labels, given as
   packed ``A_*`` attributes.

   .. versionadded:: next


.. function:: slk_attr()

   Return the current attributes of the soft labels as packed ``A_*``
   attributes.  Availability depends on the underlying curses library.

   .. versionadded:: next


.. function:: slk_attr_on(attr)
              slk_attr_off(attr)

   Turn the given attributes on or off without affecting any others.  Like the
   ``attr_*`` window methods, these work with the
   :ref:`WA_* attributes <curses-wa-constants>` rather than packed ``A_*``
   attributes.

   .. versionadded:: next


.. function:: slk_attr_set(attr, pair=0)

   Set the attributes and color pair of the soft labels.  *attr* is given as
   :ref:`WA_* attributes <curses-wa-constants>` and *pair* as a color pair
   number.

   .. versionadded:: next


.. function:: slk_color(pair)

   Set the color pair of the soft labels to color pair number *pair*.

   .. versionadded:: next


Saving and restoring
~~~~~~~~~~~~~~~~~~~~

.. function:: getwin(file)

   Read window-related data stored in the file by an earlier :meth:`window.putwin` call.
   The routine then creates and initializes a new window using that data, returning
   the new window object.  The *file* argument must be a file object opened for
   reading in binary mode.

.. function:: scr_dump(filename)

   Write the current contents of the virtual screen to *filename*, which may be
   a string or a :term:`path-like object`.  The file can later be read by
   :func:`scr_restore`, :func:`scr_init` or :func:`scr_set`.  This is the
   whole-screen counterpart of :meth:`window.putwin`.

   .. versionadded:: next

.. function:: scr_restore(filename)

   Set the virtual screen to the contents of *filename*, which must have been
   written by :func:`scr_dump`.  The next call to :func:`doupdate` or
   :meth:`window.refresh` restores the screen to those contents.

   .. versionadded:: next

.. function:: scr_init(filename)

   Initialize the assumed contents of the terminal from *filename*, which must
   have been written by :func:`scr_dump`.  Use it when the terminal already
   displays those contents, for example after another program has drawn the
   screen, so that curses does not redraw what is already there.

   .. versionadded:: next

.. function:: scr_set(filename)

   Use *filename*, which must have been written by :func:`scr_dump`, as both
   the virtual screen and the assumed terminal contents.  This combines the
   effects of :func:`scr_restore` and :func:`scr_init`.

   .. versionadded:: next


Querying the terminal
~~~~~~~~~~~~~~~~~~~~~

.. function:: baudrate()

   Return the output speed of the terminal in bits per second.  On software
   terminal emulators it will have a fixed high value. Included for historical
   reasons; in former times, it was used to  write output loops for time delays and
   occasionally to change interfaces depending on the line speed.

.. function:: erasechar()

   Return the user's current erase character as a raw byte, a :class:`bytes`
   object of length 1.  See also :func:`erasewchar`.

.. function:: erasewchar()

   Return the user's current erase character as a one-character :class:`str`.
   Under Unix operating systems this is a property of the controlling tty of the
   curses program, and is not set by the curses library itself.

   .. versionadded:: next

.. function:: killchar()

   Return the user's current line kill character as a raw byte, a :class:`bytes`
   object of length 1.  See also :func:`killwchar`.

.. function:: killwchar()

   Return the user's current line kill character as a one-character :class:`str`.
   Under Unix operating systems this is a property of the controlling tty of the
   curses program, and is not set by the curses library itself.

   .. versionadded:: next

.. function:: termname()

   Return the value of the environment variable :envvar:`TERM`, as a bytes object,
   truncated to 14 characters.

.. function:: longname()

   Return a bytes object containing the terminfo long name field describing the current
   terminal.  The maximum length of a verbose description is 128 characters.  It is
   defined only after the call to :func:`initscr`.

.. function:: termattrs()

   Return a logical OR of all video attributes supported by the terminal.  This
   information is useful when a curses program needs complete control over the
   appearance of the screen.

.. function:: term_attrs()

   Like :func:`termattrs`, but return the attributes as :ref:`WA_*
   <curses-wa-constants>` values rather than ``A_*`` values.

   .. versionadded:: next

.. function:: has_ic()

   Return ``True`` if the terminal has insert- and delete-character capabilities.
   This function is included for historical reasons only, as all modern software
   terminal emulators have such capabilities.

.. function:: has_il()

   Return ``True`` if the terminal has insert- and delete-line capabilities, or can
   simulate  them  using scrolling regions. This function is included for
   historical reasons only, as all modern software terminal emulators have such
   capabilities.


Bell and screen flash
~~~~~~~~~~~~~~~~~~~~~

.. function:: beep()

   Emit a short attention sound.

.. function:: flash()

   Flash the screen.  That is, change it to reverse-video and then change it back
   in a short interval.  Some people prefer such as 'visible bell' to the audible
   attention signal produced by :func:`beep`.


Terminal resizing
~~~~~~~~~~~~~~~~~

.. function:: resizeterm(nlines, ncols)

   Resize the standard and current windows to the specified dimensions, and
   adjusts other bookkeeping data used by the curses library that record the
   window dimensions (in particular the SIGWINCH handler).

.. function:: resize_term(nlines, ncols)

   Backend function used by :func:`resizeterm`, performing most of the work;
   when resizing the windows, :func:`resize_term` blank-fills the areas that are
   extended.  The calling application should fill in these areas with
   appropriate data.  The :func:`!resize_term` function attempts to resize all
   windows.  However, due to the calling convention of pads, it is not possible
   to resize these without additional interaction with the application.

.. function:: is_term_resized(nlines, ncols)

   Return ``True`` if :func:`resize_term` would modify the window structure,
   ``False`` otherwise.

.. function:: update_lines_cols()

   Update the :const:`LINES` and :const:`COLS` module variables.
   Useful for detecting manual screen resize.

   .. versionadded:: 3.5


Terminfo database
~~~~~~~~~~~~~~~~~

.. function:: setupterm(term=None, fd=-1)

   Initialize the terminal.  *term* is a string giving
   the terminal name, or ``None``; if omitted or ``None``, the value of the
   :envvar:`TERM` environment variable will be used.  *fd* is the
   file descriptor to which any initialization sequences will be sent; if not
   supplied or ``-1``, the file descriptor for ``sys.stdout`` will be used.

   Raise a :exc:`curses.error` if the terminal could not be found or its
   terminfo database entry could not be read.  If the terminal has already
   been initialized, this function has no effect.

   .. note::

      Calling :func:`initscr` or :func:`newterm` after :func:`setupterm`
      leaks the terminal that :func:`setupterm` allocated:
      the curses library keeps only a single current terminal
      and does not free the previously allocated one.

.. function:: tigetflag(capname)

   Return the value of the Boolean capability corresponding to the terminfo
   capability name *capname* as an integer.  Return the value ``-1`` if *capname* is not a
   Boolean capability, or ``0`` if it is canceled or absent from the terminal
   description.

   :func:`setupterm` (or :func:`initscr`) must be called first.

.. function:: tigetnum(capname)

   Return the value of the numeric capability corresponding to the terminfo
   capability name *capname* as an integer.  Return the value ``-2`` if *capname* is not a
   numeric capability, or ``-1`` if it is canceled or absent from the terminal
   description.

   :func:`setupterm` (or :func:`initscr`) must be called first.

.. function:: tigetstr(capname)

   Return the value of the string capability corresponding to the terminfo
   capability name *capname* as a bytes object.  Return ``None`` if *capname*
   is not a terminfo "string capability", or is canceled or absent from the
   terminal description.

   :func:`setupterm` (or :func:`initscr`) must be called first.

.. function:: tparm(str[, ...])

   Instantiate the bytes object *str* with the supplied parameters, where *str* should
   be a parameterized string obtained from the terminfo database.  For example,
   ``tparm(tigetstr("cup"), 5, 3)`` could result in ``b'\033[6;4H'``, the exact
   result depending on terminal type.  Up to nine integer parameters may be supplied.

   :func:`setupterm` (or :func:`initscr`) must be called first.

.. function:: putp(str)

   Equivalent to ``tputs(str, 1, putchar)``; emit the value of a specified
   terminfo capability for the current terminal.  Note that the output of :func:`putp`
   always goes to standard output.

   :func:`setupterm` (or :func:`initscr`) must be called first.


Utilities
~~~~~~~~~

.. function:: unctrl(ch)

   Return a bytes object which is a printable representation of the character *ch*.
   *ch* cannot be a character that does not fit in a single byte; use
   :func:`wunctrl` for those.

.. function:: wunctrl(ch)

   Return a string which is a printable representation of the character *ch*;
   any attributes and color pair are ignored.
   ASCII control characters are represented as a caret followed by a character,
   for example as ``'^C'``.  Printing characters, including non-ASCII characters
   printable in the locale, are left as they are.  The representation of other
   characters is defined by the underlying curses library.

   .. versionadded:: next

.. function:: filter()

   The :func:`.filter` routine, if used, must be called before :func:`initscr` is
   called.  The effect is that, during the initialization, :envvar:`LINES` is set to ``1``; the
   capabilities ``clear``, ``cup``, ``cud``, ``cud1``, ``cuu1``, ``cuu``, ``vpa`` are disabled; and the ``home``
   string is set to the value of ``cr``. The effect is that the cursor is confined to
   the current line, and so are screen updates.  This may be used for enabling
   character-at-a-time  line editing without touching the rest of the screen.

.. function:: nofilter()

   Undo the effect of a previous :func:`.filter` call.
   Like :func:`.filter`, it must be called before :func:`initscr` (or
   :func:`newterm`) so that the next initialization uses the full screen
   again.

   Availability: if the underlying curses library provides ``nofilter()``.

   .. versionadded:: next

.. function:: use_env(flag)

   If used, this function should be called before :func:`initscr` or newterm are
   called.  When *flag* is ``False``, the values of lines and columns specified in the
   terminfo database will be used, even if environment variables :envvar:`LINES`
   and :envvar:`COLUMNS` (used by default) are set, or if curses is running in a
   window (in which case default behavior would be to use the window size if
   :envvar:`LINES` and :envvar:`COLUMNS` are not set).

.. function:: get_escdelay()

   Retrieves the value set by :func:`set_escdelay`.

   .. versionadded:: 3.9

.. function:: set_escdelay(ms)

   Sets the number of milliseconds to wait after reading an escape character,
   to distinguish between an individual escape character entered on the
   keyboard from escape sequences sent by cursor and function keys.

   .. versionadded:: 3.9

.. function:: get_tabsize()

   Retrieves the value set by :func:`set_tabsize`.

   .. versionadded:: 3.9

.. function:: set_tabsize(size)

   Sets the number of columns used by the curses library when converting a tab
   character to spaces as it adds the tab to a window.

   .. versionadded:: 3.9

.. function:: napms(ms)

   Sleep for *ms* milliseconds.

.. function:: delay_output(ms)

   Insert an *ms* millisecond pause in output.

.. _curses-window-objects:

Window objects
--------------

.. class:: window

   Window objects, as returned by :func:`initscr` and :func:`newwin` above, have
   the following methods and attributes:


Adding and inserting text
~~~~~~~~~~~~~~~~~~~~~~~~~

.. method:: window.addch(ch[, attr])
            window.addch(y, x, ch[, attr])

   Paint character *ch* at ``(y, x)`` with attributes *attr*, overwriting any
   character previously painted at that location.  By default, the character
   position and attributes are the current settings for the window object.

   *ch* may be a single character, optionally followed by combining
   characters, that together occupy one character cell.

   .. note::

      Writing outside the window, subwindow, or pad raises a :exc:`curses.error`.
      Attempting to write to the lower-right corner of a window, subwindow,
      or pad will cause an exception to be raised after the character is printed.

   .. versionchanged:: next
      A character may now be given as a string of a base character followed
      by combining characters, instead of only a single character, or as a
      :class:`complexchar` cell.

.. method:: window.addstr(str[, attr])
            window.addstr(y, x, str[, attr])

   Paint the character string *str* at ``(y, x)`` with attributes
   *attr*, overwriting anything previously on the display.

   *str* may also be a :class:`complexstr`, in which case each cell carries its
   own attributes and color pair, so *attr* must not be given.  A
   :class:`complexstr` obtained from :meth:`in_wchstr` is written back
   unchanged.

   .. note::

      * Writing outside the window, subwindow, or pad raises :exc:`curses.error`.
        Attempting to write to the lower-right corner of a window, subwindow,
        or pad will cause an exception to be raised after the string is printed.

      * A bug in ncurses, the backend for this Python module, could cause
        segfaults when resizing windows.  This was fixed in ncurses-6.1-20190511.
        If you are stuck with an earlier ncurses, you can avoid triggering it by
        not calling :meth:`!addstr` with a *str* that has embedded newlines;
        instead, call :meth:`!addstr` separately for each line.

   .. versionchanged:: next
      *str* may now also be a :class:`complexstr`, as described above.

.. method:: window.addnstr(str, n[, attr])
            window.addnstr(y, x, str, n[, attr])

   Paint at most *n* characters of the character string *str* at
   ``(y, x)`` with attributes
   *attr*, overwriting anything previously on the display.

   .. versionchanged:: next
      *str* may now also be a :class:`complexstr`; see :meth:`addstr`.

.. method:: window.echochar(ch[, attr])

   Add character *ch* with attribute *attr*, and immediately  call :meth:`refresh`
   on the window.

   .. versionchanged:: next
      Wide and combining characters, and :class:`complexchar` cells, are now
      accepted.

.. method:: window.insch(ch[, attr])
            window.insch(y, x, ch[, attr])

   Insert character *ch* with attributes *attr* before the character under the
   cursor, or at ``(y, x)`` if specified.  All characters to the right of the
   cursor are shifted one position right, with the rightmost character on the
   line being lost.  The cursor position does not change.

   .. versionchanged:: next
      Wide and combining characters, and :class:`complexchar` cells, are now
      accepted.

.. method:: window.insstr(str[, attr])
            window.insstr(y, x, str[, attr])

   Insert a character string (as many characters as will fit on the line) before
   the character under the cursor.  All characters to the right of the cursor are
   shifted right, with the rightmost characters on the line being lost.  The cursor
   position does not change (after moving to *y*, *x*, if specified).

   *str* may also be a :class:`complexstr`, in which case each cell carries its
   own attributes and color pair, so *attr* must not be given.

   .. versionchanged:: next
      *str* may now also be a :class:`complexstr`, as described above.

.. method:: window.insnstr(str, n[, attr])
            window.insnstr(y, x, str, n[, attr])

   Insert a character string (as many characters as will fit on the line) before
   the character under the cursor, up to *n* characters.   If *n* is zero or
   negative, the entire string is inserted. All characters to the right of the
   cursor are shifted right, with the rightmost characters on the line being lost.
   The cursor position does not change (after moving to *y*, *x*, if specified).

   .. versionchanged:: next
      *str* may now also be a :class:`complexstr`; see :meth:`insstr`.


Deleting and inserting lines
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. method:: window.delch([y, x])

   Delete the character under the cursor, or at ``(y, x)`` if specified.  All
   characters to the right on the same line are shifted one position left.

.. method:: window.deleteln()

   Delete the line under the cursor. All following lines are moved up by one line.

.. method:: window.insertln()

   Insert a blank line under the cursor. All following lines are moved down by one
   line.

.. method:: window.insdelln(nlines)

   Insert *nlines* lines into the specified window above the current line.  The
   *nlines* bottom lines are lost.  For negative *nlines*, delete *nlines* lines
   starting with the one under the cursor, and move the remaining lines up.  The
   bottom *nlines* lines are cleared.  The current cursor position remains the
   same.


Reading input
~~~~~~~~~~~~~

.. method:: window.getch([y, x])

   Get a character. Note that the integer returned does *not* have to be in ASCII
   range: function keys, keypad keys and so on are represented by numbers higher
   than 255.  In no-delay mode, return ``-1`` if there is no input, otherwise
   wait until a key is pressed.
   A multibyte character is returned as its encoded bytes one at a time; use
   :meth:`get_wch` to read it as a single character.

.. method:: window.get_wch([y, x])

   Get a wide character. Return a character for most keys, or an integer for
   function keys, keypad keys, and other special keys.  Unlike :meth:`getch`, an
   ordinary key is returned as a one-character :class:`str`.
   In no-delay mode, raise an exception if there is no input.

   .. versionadded:: 3.3

   .. versionchanged:: next
      Also available on a narrow build, where only a character representable as a
      single byte (an 8-bit locale) can be returned.

.. method:: window.getkey([y, x])

   Get a character, returning a string instead of an integer, as :meth:`getch`
   does. Function keys, keypad keys and other special keys return a multibyte
   string containing the key name.  In no-delay mode, raise an exception if
   there is no input.

.. method:: window.getstr()
            window.getstr(n)
            window.getstr(y, x)
            window.getstr(y, x, n)

   Read a bytes object from the user, with primitive line editing capacity.
   At most *n* characters are read;
   *n* defaults to and cannot exceed 2047.
   A multibyte character is returned as its encoded bytes; use :meth:`get_wstr`
   to read the input as a :class:`str`.

   .. versionchanged:: 3.14
      The maximum value for *n* was increased from 1023 to 2047.

.. method:: window.get_wstr()
            window.get_wstr(n)
            window.get_wstr(y, x)
            window.get_wstr(y, x, n)

   Read a string from the user, with primitive line editing capacity.
   Unlike :meth:`getstr`, it can return characters that are not representable in
   the window's encoding.
   At most *n* characters are read; *n* defaults to and cannot exceed 2047.

   .. versionadded:: next

.. method:: window.getdelay()

   Return the window's read timeout in milliseconds,
   as set by :meth:`nodelay` or :meth:`timeout`:
   ``-1`` for blocking, ``0`` for non-blocking,
   or a positive number of milliseconds.

   .. versionadded:: next


Reading window contents
~~~~~~~~~~~~~~~~~~~~~~~

.. method:: window.inch([y, x])

   Return the character at the given position in the window.
   The bottom 8 bits are the character proper and the upper bits are the attributes;
   extract them with the :data:`A_CHARTEXT` and :data:`A_ATTRIBUTES` bit-masks,
   and the color pair with :func:`pair_number`.
   The character byte is the locale-encoded byte of the cell's character,
   consistent with :meth:`instr`.
   It cannot represent a cell holding combining characters, a character that does
   not fit in a single byte, or a color pair outside the :func:`color_pair`
   range; use :meth:`in_wch` for those, which returns it as a :class:`complexchar`.

.. method:: window.in_wch([y, x])

   Return the complex character at the given position in the window as a
   :class:`complexchar`.  Unlike :meth:`inch`, the returned object carries the
   cell's text (a spacing character optionally followed by combining characters)
   together with its attributes and color pair.

   .. versionadded:: next

.. method:: window.instr([n])
            window.instr(y, x[, n])

   Return a bytes object of characters, extracted from the window starting at the
   current cursor position, or at *y*, *x* if specified, and stopping at the end
   of the line. Attributes and color information are stripped
   from the characters.  If *n* is specified, :meth:`instr` returns a string
   at most *n* characters long (exclusive of the trailing NUL).
   The maximum value for *n* is 2047.
   A character not representable in the window's encoding cannot be returned;
   use :meth:`in_wstr` for those.

   .. versionchanged:: 3.14
      The maximum value for *n* was increased from 1023 to 2047.

.. method:: window.in_wstr([n])
            window.in_wstr(y, x[, n])

   Return a string of characters, extracted from the window starting at the
   current cursor position, or at *y*, *x* if specified.  Unlike :meth:`instr`,
   it can return characters that are not representable in the window's encoding.
   Attributes and color information are stripped from the characters.  The
   maximum value for *n* is 2047.

   .. versionadded:: next

.. method:: window.in_wchstr([n])
            window.in_wchstr(y, x[, n])

   Return a :class:`complexstr` of the styled cells extracted from the window
   starting at the current cursor position, or at *y*, *x* if specified, and
   stopping at the end of the line.  This is the variant of :meth:`instr` and
   :meth:`in_wstr` that *keeps* each cell's attributes and color pair (those
   methods strip the rendition).  If *n* is specified, at most *n* cells are
   returned.  The maximum value for *n* is 2047.

   The result can be written back unchanged with :meth:`addstr` (a read and a
   re-write is a round-trip that preserves every cell's rendition).

   .. versionadded:: next


Attributes
~~~~~~~~~~

.. method:: window.attroff(attr)

   Remove attribute *attr* from the "background" set applied to all writes to the
   current window.

.. method:: window.attron(attr)

   Add attribute *attr* to the "background" set applied to all writes to the
   current window.

.. method:: window.attrset(attr)

   Set the "background" set of attributes to *attr*.  This set is initially
   ``0`` (no attributes).

.. method:: window.attr_get()

   Return the window's current rendition as a ``(attrs, pair)`` tuple,
   where *attrs* is the set of attributes and *pair* is the color pair number.

   Unlike :meth:`attron` and friends, which take packed ``A_*`` attributes,
   this method and the other ``attr_*`` methods work with the
   :ref:`WA_* attributes <curses-wa-constants>` and keep the color pair as a
   separate number, which lets them use color pairs that do not fit alongside
   the attributes in a single value.

   .. versionadded:: next

.. method:: window.attr_set(attr, pair=0)

   Set the window's rendition to the attributes *attr* and the color pair *pair*.

   .. versionadded:: next

.. method:: window.attr_on(attr)

   Turn on the attributes *attr* without affecting any others.

   .. versionadded:: next

.. method:: window.attr_off(attr)

   Turn off the attributes *attr* without affecting any others.

   .. versionadded:: next

.. method:: window.color_set(pair)

   Set the window's color pair to *pair*, leaving the other attributes unchanged.

   .. versionadded:: next

.. method:: window.getattrs()

   Return the window's current attributes.

   .. versionadded:: next

.. method:: window.standout()

   Turn on attribute *A_STANDOUT*.

.. method:: window.standend()

   Turn off the standout attribute.  On some terminals this has the side effect of
   turning off all attributes.

.. method:: window.chgat(attr)
            window.chgat(num, attr)
            window.chgat(y, x, attr)
            window.chgat(y, x, num, attr)

   Set the attributes of *num* characters at the current cursor position, or at
   position ``(y, x)`` if supplied. If *num* is not given or is ``-1``,
   the attribute will be set on all the characters to the end of the line.  This
   function moves cursor to position ``(y, x)`` if supplied. The changed line
   will be touched using the :meth:`touchline` method so that the contents will
   be redisplayed by the next window refresh.


Background
~~~~~~~~~~

.. method:: window.bkgd(ch[, attr])

   Set the background property of the window to the character *ch*, with
   attributes *attr*.  The change is then applied to every character position in
   that window:

   * The attribute of every character in the window  is changed to the new
     background attribute.

   * Wherever  the  former background character appears, it is changed to the new
     background character.

   .. versionchanged:: next
      Wide and combining characters, and :class:`complexchar` cells, are now
      accepted.

.. method:: window.bkgdset(ch[, attr])

   Set the window's background.  A window's background consists of a character and
   any combination of attributes.  The attribute part of the background is combined
   (OR'ed) with all non-blank characters that are written into the window.  Both
   the character and attribute parts of the background are combined with the blank
   characters.  The background becomes a property of the character and moves with
   the character through any scrolling and insert/delete line/character operations.

   .. versionchanged:: next
      Wide and combining characters, and :class:`complexchar` cells, are now
      accepted.

.. method:: window.getbkgd()

   Return the given window's current background character/attribute pair.
   Its components can be extracted like those of :meth:`inch`.
   It cannot represent a background set with a wide character or with a color
   pair outside the :func:`color_pair` range; use :meth:`getbkgrnd` for those.

.. method:: window.getbkgrnd()

   Return the given window's current background as a :class:`complexchar`.
   Unlike :meth:`getbkgd`, the returned object carries the background character
   together with its attributes and color pair, and the color pair is not limited
   to the value that fits in a :func:`color_pair`.

   .. versionadded:: next


Clearing and erasing
~~~~~~~~~~~~~~~~~~~~

.. method:: window.erase()

   Clear the window.

.. method:: window.clear()

   Like :meth:`erase`, but also cause the whole window to be repainted upon next
   call to :meth:`refresh`.

.. method:: window.clrtobot()

   Erase from cursor to the end of the window: all lines below the cursor are
   deleted, and then the equivalent of :meth:`clrtoeol` is performed.

.. method:: window.clrtoeol()

   Erase from cursor to the end of the line.


Borders and lines
~~~~~~~~~~~~~~~~~

.. method:: window.border([ls[, rs[, ts[, bs[, tl[, tr[, bl[, br]]]]]]]])

   Draw a border around the edges of the window. Each parameter specifies  the
   character to use for a specific part of the border; see the table below for more
   details.

   .. note::

      A ``0`` value for any parameter will cause the default character to be used for
      that parameter.  Keyword parameters can *not* be used.  The defaults are listed
      in this table:

   +-----------+---------------------+-----------------------+
   | Parameter | Description         | Default value         |
   +===========+=====================+=======================+
   | *ls*      | Left side           | :const:`ACS_VLINE`    |
   +-----------+---------------------+-----------------------+
   | *rs*      | Right side          | :const:`ACS_VLINE`    |
   +-----------+---------------------+-----------------------+
   | *ts*      | Top                 | :const:`ACS_HLINE`    |
   +-----------+---------------------+-----------------------+
   | *bs*      | Bottom              | :const:`ACS_HLINE`    |
   +-----------+---------------------+-----------------------+
   | *tl*      | Upper-left corner   | :const:`ACS_ULCORNER` |
   +-----------+---------------------+-----------------------+
   | *tr*      | Upper-right corner  | :const:`ACS_URCORNER` |
   +-----------+---------------------+-----------------------+
   | *bl*      | Bottom-left corner  | :const:`ACS_LLCORNER` |
   +-----------+---------------------+-----------------------+
   | *br*      | Bottom-right corner | :const:`ACS_LRCORNER` |
   +-----------+---------------------+-----------------------+

   .. versionchanged:: next
      Wide and combining characters, and :class:`complexchar` cells, are now
      accepted.  A single call cannot mix
      them with integer or byte characters.

.. method:: window.box([vertch, horch])

   Similar to :meth:`border`, but both *ls* and *rs* are *vertch* and both *ts* and
   *bs* are *horch*.  The default corner characters are always used by this function.

   .. versionchanged:: next
      Wide and combining characters, and :class:`complexchar` cells, are now
      accepted.  A single call cannot mix
      them with integer or byte characters.

.. method:: window.hline(ch, n[, attr])
            window.hline(y, x, ch, n[, attr])

   Display a horizontal line starting at ``(y, x)`` with length *n* consisting of
   the character *ch* with attributes *attr*.  The line stops at the right edge
   of the window if fewer than *n* cells are available.

   .. versionchanged:: next
      Wide and combining characters, and :class:`complexchar` cells, are now
      accepted.

.. method:: window.vline(ch, n[, attr])
            window.vline(y, x, ch, n[, attr])

   Display a vertical line starting at ``(y, x)`` with length *n* consisting of the
   character *ch* with attributes *attr*.

   .. versionchanged:: next
      Wide and combining characters, and :class:`complexchar` cells, are now
      accepted.


Cursor position and window geometry
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. method:: window.move(new_y, new_x)

   Move cursor to ``(new_y, new_x)``.

.. method:: window.getyx()

   Return a tuple ``(y, x)`` of current cursor position  relative to the window's
   upper-left corner.

.. method:: window.getbegyx()

   Return a tuple ``(y, x)`` of coordinates of upper-left corner.

.. method:: window.getmaxyx()

   Return a tuple ``(y, x)`` of the height and width of the window.

.. method:: window.getparyx()

   Return the beginning coordinates of this window relative to its parent window
   as a tuple ``(y, x)``.  Return ``(-1, -1)`` if this window has no
   parent.

.. method:: window.getparent()

   Return the parent window of this subwindow,
   or ``None`` if this window is not a subwindow.

   .. versionadded:: next


Creating and resizing windows
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. method:: window.subwin(begin_y, begin_x)
            window.subwin(nlines, ncols, begin_y, begin_x)

   Return a sub-window, whose upper-left corner is at the screen-relative
   coordinates ``(begin_y, begin_x)``, and whose width/height is *ncols*/*nlines*.

   By default, the sub-window will extend from the specified position to the lower
   right corner of the window.

.. method:: window.derwin(begin_y, begin_x)
            window.derwin(nlines, ncols, begin_y, begin_x)

   An abbreviation for "derive window", :meth:`derwin` is the same as calling
   :meth:`subwin`, except that *begin_y* and *begin_x* are relative to the origin
   of the window, rather than relative to the entire screen.  Return a window
   object for the derived window.

.. method:: window.subpad(begin_y, begin_x)
            window.subpad(nlines, ncols, begin_y, begin_x)

   Return a sub-pad, whose upper-left corner is at ``(begin_y, begin_x)``, and
   whose width/height is *ncols*/*nlines*.  The coordinates are relative to the
   parent pad (unlike :meth:`subwin`, which uses screen coordinates).  This
   method is only available for pads created with :func:`newpad`.

.. method:: window.dupwin()

   Return a new window that is an exact duplicate of the window: it has the same
   size, position, contents and attributes.  Unlike a window created by
   :meth:`subwin` or :meth:`derwin`, the duplicate is independent of the
   original -- it has its own cell buffer, so later changes to one do not affect
   the other.

   .. versionadded:: next

.. method:: window.mvwin(new_y, new_x)

   Move the window so its upper-left corner is at ``(new_y, new_x)``.

   Moving the window so that any part of it would be off the screen is an error:
   the window is not moved and :exc:`curses.error` is raised.

.. method:: window.mvderwin(y, x)

   Move the window inside its parent window.  The screen-relative parameters of
   the window are not changed.  This routine is used to display different parts of
   the parent window at the same physical position on the screen.

.. method:: window.resize(nlines, ncols)

   Reallocate storage for a curses window to adjust its dimensions to the
   specified values.  If either dimension is larger than the current values, the
   window's data is filled with blanks that have the current background
   rendition (as set by :meth:`bkgdset`) merged into them.


Refreshing and redrawing
~~~~~~~~~~~~~~~~~~~~~~~~

.. method:: window.refresh([pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol])

   Update the display immediately (sync actual screen with previous
   drawing/deleting methods).

   The 6 arguments can only be specified, and are then required, when the window
   is a pad created with :func:`newpad`.  The additional parameters are needed to indicate what part
   of the pad and screen are involved. *pminrow* and *pmincol* specify the
   upper-left corner of the rectangle to be displayed in the pad.  *sminrow*,
   *smincol*, *smaxrow*, and *smaxcol* specify the edges of the rectangle to be
   displayed on the screen.  The lower-right corner of the rectangle to be
   displayed in the pad is calculated from the screen coordinates, since the
   rectangles must be the same size.  Both rectangles must be entirely contained
   within their respective structures.  Negative values of *pminrow*, *pmincol*,
   *sminrow*, or *smincol* are treated as if they were zero.

.. method:: window.noutrefresh()
            window.noutrefresh(pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol)

   Mark for refresh but wait.  This function updates the data structure
   representing the desired state of the window, but does not force an update of
   the physical screen.  To accomplish that, call  :func:`doupdate`.

   The 6 arguments can only be specified, and are then required, when the window
   is a pad created with :func:`newpad`; they have the same meaning as for
   :meth:`refresh`.

.. method:: window.redrawln(beg, num)

   Indicate that the *num* screen lines, starting at line *beg*, are corrupted and
   should be completely redrawn on the next :meth:`refresh` call.

.. method:: window.redrawwin()

   Touch the entire window, causing it to be completely redrawn on the next
   :meth:`refresh` call.


Output options
~~~~~~~~~~~~~~

.. method:: window.clearok(flag)

   If *flag* is ``True``, the next call to :meth:`refresh` will clear the window
   completely.

.. method:: window.idlok(flag)

   If *flag* is ``True``, :mod:`!curses` will try to use hardware line
   editing facilities.  Otherwise, curses will not use them.

.. method:: window.idcok(flag)

   If *flag* is ``False``, curses no longer considers using the hardware insert/delete
   character feature of the terminal; if *flag* is ``True``, use of character insertion
   and deletion is enabled.  When curses is first initialized, use of character
   insert/delete is enabled by default.

.. method:: window.immedok(flag)

   If *flag* is ``True``, any change in the window image automatically causes the
   window to be refreshed; you no longer have to call :meth:`refresh` yourself.
   However, it may degrade performance considerably, due to repeated calls to
   wrefresh.  This option is disabled by default.

.. method:: window.leaveok(flag)

   If *flag* is ``True``, cursor is left where it is on update, instead of being at "cursor
   position."  This reduces cursor movement where possible.

   If *flag* is ``False``, cursor will always be at "cursor position" after an update.

.. method:: window.scrollok(flag)

   Control what happens when the cursor of a window is moved off the edge of the
   window or scrolling region, either as a result of a newline action on the bottom
   line, or typing the last character of the last line.  If *flag* is ``False``, the
   cursor is left on the bottom line.  If *flag* is ``True``, the window is scrolled up
   one line.  Note that in order to get the physical scrolling effect on the
   terminal, it is also necessary to call :meth:`idlok`.

.. method:: window.scroll([lines=1])

   Scroll the screen or scrolling region.  Scroll upward by *lines* lines if
   *lines* is positive, or downward if it is negative.  Scrolling has no effect
   unless it has been enabled for the window with :meth:`scrollok`.

.. method:: window.setscrreg(top, bottom)

   Set the scrolling region from line *top* to line *bottom*. All scrolling actions
   will take place in this region.

.. method:: window.getscrreg()

   Return a tuple ``(top, bottom)`` of the window's current scrolling region,
   as set by :meth:`setscrreg`.

   .. versionadded:: next

.. method:: window.syncok(flag)

   If *flag* is ``True``, then :meth:`syncup` is called automatically
   whenever there is a change in the window.


Input options
~~~~~~~~~~~~~

.. method:: window.keypad(flag)

   If *flag* is ``True``, escape sequences generated by some keys (keypad,  function keys)
   will be interpreted by :mod:`!curses`. If *flag* is ``False``, escape sequences will be
   left as is in the input stream.

.. method:: window.nodelay(flag)

   If *flag* is ``True``, :meth:`getch` will be non-blocking.

.. method:: window.notimeout(flag)

   If *flag* is ``True``, escape sequences will not be timed out.

   If *flag* is ``False``, after a few milliseconds, an escape sequence will not be
   interpreted, and will be left in the input stream as is.

.. method:: window.timeout(delay)

   Set blocking or non-blocking read behavior for the window.  If *delay* is
   negative, blocking read is used (which will wait indefinitely for input).  If
   *delay* is zero, then non-blocking read is used, and :meth:`getch` will
   return ``-1`` if no input is waiting.  If *delay* is positive, then
   :meth:`getch` will block for *delay* milliseconds, and return ``-1`` if there is
   still no input at the end of that time.


Overlapping and touch
~~~~~~~~~~~~~~~~~~~~~

.. method:: window.overlay(destwin[, sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol])

   Overlay the window on top of *destwin*. The windows need not be the same size,
   only the overlapping region is copied. This copy is non-destructive, which means
   that the current background character does not overwrite the old contents of
   *destwin*.

   To get fine-grained control over the copied region, the second form of
   :meth:`overlay` can be used. *sminrow* and *smincol* are the upper-left
   coordinates of the source window, and the other variables mark a rectangle in
   the destination window.

.. method:: window.overwrite(destwin[, sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol])

   Overwrite the window on top of *destwin*. The windows need not be the same size,
   in which case only the overlapping region is copied. This copy is destructive,
   which means that the current background character overwrites the old contents of
   *destwin*.

   To get fine-grained control over the copied region, the second form of
   :meth:`overwrite` can be used. *sminrow* and *smincol* are the upper-left
   coordinates of the source window, the other variables mark a rectangle in the
   destination window.

.. method:: window.touchline(start, count[, changed])

   Pretend *count* lines have been changed, starting with line *start*.  If
   *changed* is supplied, it specifies whether the affected lines are marked as
   having been changed (*changed*\ ``=True``) or unchanged (*changed*\ ``=False``).

.. method:: window.touchwin()

   Pretend the whole window has been changed, for purposes of drawing
   optimizations.

.. method:: window.untouchwin()

   Mark all lines in  the  window  as unchanged since the last call to
   :meth:`refresh`.

.. method:: window.syncup()

   Touch all locations in ancestors of the window that have been changed in  the
   window.

.. method:: window.syncdown()

   Touch each location in the window that has been touched in any of its ancestor
   windows.  This routine is called by :meth:`refresh`, so it should almost never
   be necessary to call it manually.

.. method:: window.cursyncup()

   Update the current cursor position of all the ancestors of the window to
   reflect the current cursor position of the window.


Coordinate conversion
~~~~~~~~~~~~~~~~~~~~~

.. method:: window.enclose(y, x)

   Test whether the given pair of screen-relative character-cell coordinates are
   enclosed by the given window, returning ``True`` or ``False``.  It is useful for
   determining what subset of the screen windows enclose the location of a mouse
   event.

   .. versionchanged:: 3.10
      Previously it returned ``1`` or ``0`` instead of ``True`` or ``False``.

.. method:: window.mouse_trafo(y, x, to_screen)

   Convert between window-relative and screen-relative (``stdscr``-relative) character-cell coordinates.
   If *to_screen* is true, convert the window-relative coordinates *y*, *x* to screen-relative coordinates;
   otherwise convert in the opposite direction.
   The two coordinate systems differ when lines are reserved on the screen, for example for soft labels.

   Return the converted coordinates as a ``(y, x)`` tuple, or ``None`` if they lie outside the window.

   .. versionadded:: next


Other
~~~~~

.. attribute:: window.encoding

   Encoding used to encode method arguments (Unicode strings and characters).
   The encoding attribute is inherited from the parent window when a subwindow
   is created, for example with :meth:`window.subwin`.
   By default, current locale encoding is used (see :func:`locale.getencoding`).

   .. versionadded:: 3.3

.. method:: window.putwin(file)

   Write all data associated with the window into the provided file object.  This
   information can be later retrieved using the :func:`getwin` function.

.. method:: window.use(func, /, *args, **kwargs)

   Call ``func(window, *args, **kwargs)`` with the lock of the window held,
   and return its result.
   This provides automatic protection for the window
   against concurrent access from another thread.

   Availability: if the underlying curses library provides ``use_window()``.

   .. versionadded:: next


State queries
~~~~~~~~~~~~~

.. method:: window.is_cleared()

   Return the current value set by :meth:`clearok`.

   .. versionadded:: next

.. method:: window.is_idcok()

   Return the current value set by :meth:`idcok`.

   .. versionadded:: next

.. method:: window.is_idlok()

   Return the current value set by :meth:`idlok`.

   .. versionadded:: next

.. method:: window.is_immedok()

   Return the current value set by :meth:`immedok`.

   .. versionadded:: next

.. method:: window.is_keypad()

   Return the current value set by :meth:`keypad`.

   .. versionadded:: next

.. method:: window.is_leaveok()

   Return the current value set by :meth:`leaveok`.

   .. versionadded:: next

.. method:: window.is_linetouched(line)

   Return ``True`` if the specified line was modified since the last call to
   :meth:`refresh`; otherwise return ``False``.  Raise a :exc:`curses.error`
   exception if *line* is not valid for the given window.

.. method:: window.is_nodelay()

   Return the current value set by :meth:`nodelay`.

   .. versionadded:: next

.. method:: window.is_notimeout()

   Return the current value set by :meth:`notimeout`.

   .. versionadded:: next

.. method:: window.is_pad()

   Return ``True`` if the window is a pad created by :func:`newpad`.

   .. versionadded:: next

.. method:: window.is_scrollok()

   Return the current value set by :meth:`scrollok`.

   .. versionadded:: next

.. method:: window.is_subwin()

   Return ``True`` if the window is a subwindow created by :meth:`subwin`
   or :meth:`derwin`.

   .. versionadded:: next

.. method:: window.is_syncok()

   Return the current value set by :meth:`syncok`.

   .. versionadded:: next

.. method:: window.is_wintouched()

   Return ``True`` if the specified window was modified since the last call to
   :meth:`refresh`; otherwise return ``False``.

.. _curses-screen-objects:

Screen objects
--------------

.. class:: screen

   A *screen* object represents a terminal initialized by :func:`newterm`
   (or :func:`new_prescr`),
   in addition to the default screen created by :func:`initscr`.
   Screen objects are returned by those functions;
   they cannot be instantiated directly.

   A screen is freed automatically once it is no longer referenced,
   either directly or through one of its windows.
   Each window keeps its screen alive,
   so a screen remains valid as long as any of its windows does.

   .. versionadded:: next


.. method:: screen.close()

   Detach the screen's standard window,
   breaking the reference cycle between them
   so the screen can be reclaimed promptly instead of waiting for a
   garbage collection.
   Afterwards :attr:`~screen.stdscr` is ``None``
   and the window it returned earlier can no longer be used.
   The screen's resources are released
   once it and all its windows are no longer referenced.

   .. versionadded:: next


.. attribute:: screen.stdscr

   The standard :ref:`window <curses-window-objects>` of the screen,
   covering the whole terminal,
   or ``None`` for a screen created by :func:`new_prescr`.


.. method:: screen.use(func, /, *args, **kwargs)

   Call ``func(screen, *args, **kwargs)`` with the lock of the screen held,
   and return its result.
   This provides automatic protection for the screen
   against concurrent access from another thread.

   Availability: if the underlying curses library provides ``use_screen()``.

   .. versionadded:: next


.. _curses-complexchar-objects:

Complex character objects
-------------------------

.. class:: complexchar(text, /, attr=0, pair=0)

   A *complex character* (or *complexchar*) is an immutable styled
   character cell: a spacing character optionally followed by combining
   characters, together with a set of attributes and a color pair.

   *text* is the cell's text, *attr* a combination of the
   :ref:`WA_* attributes <curses-wa-constants>` (equivalent to the matching
   ``A_*`` constants), and *pair* a color pair number.  Unlike the packed
   :class:`chtype <int>` used by :meth:`~window.inch` and the ``A_*`` methods,
   the color pair is stored separately and is not limited to the value that
   fits in a :func:`color_pair`.

   Complex characters are returned by :meth:`window.in_wch` and
   :meth:`window.getbkgrnd`, and are accepted (along with an integer, a byte
   or a string) by the character-cell methods such as :meth:`window.addch`,
   :meth:`window.insch`, :meth:`window.bkgd`, :meth:`window.border`,
   :meth:`window.hline` and :meth:`window.vline`.  A complex character already
   carries its own rendition, so it cannot be combined with an explicit *attr*
   argument.

   :func:`str` returns the cell's text; two complex characters are equal when
   their text, attributes and color pair all match.

   The same code works on both wide- and narrow-character builds.  On a narrow
   build a cell holds a single character (no combining marks) that must encode to
   one byte in the window's encoding (8-bit locales only), and *pair* is limited
   to the value that fits in a :func:`color_pair`.

   .. attribute:: attr

      The attributes of the character cell (read-only).

   .. attribute:: pair

      The color pair number of the character cell (read-only).

   .. versionadded:: next


.. class:: complexstr(cells[, attr[, pair]])

   A *complex character string* (or *complexstr*) is an immutable sequence of
   styled character cells -- the string counterpart of
   :class:`complexchar` (as :class:`str` is to a single character).

   If *cells* is a string, it is split into character cells (each a spacing
   character optionally followed by combining characters), and *attr* (a
   combination of the :ref:`WA_* attributes <curses-wa-constants>`) and *pair*
   (a color pair number), if given, are applied to every cell.

   Otherwise *cells* is an iterable whose items are themselves cells, each a
   :class:`complexchar` or a string; each item then carries its own rendition,
   and *attr* and *pair* must be omitted.

   It is returned by :meth:`window.in_wchstr`, and accepted by
   :meth:`window.addstr`, :meth:`~window.addnstr`, :meth:`~window.insstr` and
   :meth:`~window.insnstr`, so a run read from a window can be written back
   unchanged.

   It behaves like an immutable sequence: ``len(s)`` is the number of cells,
   ``s[i]`` is the *i*-th cell as a :class:`complexchar`, slicing and
   concatenation produce new :class:`!complexstr` instances, and iterating
   yields the cells.  :func:`str` returns the cells' text joined together, and
   two complex character strings are equal when their cells all match.  It is
   hashable.

   To build or edit a run of cells, use an ordinary :class:`list` of
   :class:`complexchar` (or strings); a :class:`!complexstr` is the immutable
   form returned by a read.

   Like :class:`complexchar`, this type works on both wide- and narrow-character
   builds, with the same per-cell limitations on a narrow build.

   .. versionadded:: next


Constants
---------

General
~~~~~~~

The :mod:`!curses` module defines the following data members:


.. data:: ERR

   Some curses routines  that  return  an integer, such as :meth:`~window.getch`, return
   :const:`ERR` upon failure.


.. data:: OK

   Some curses routines  that  return  an integer, such as  :func:`napms`, return
   :const:`OK` upon success.


.. data:: version

   A bytes object representing the current version of the module.


.. data:: ncurses_version

   A named tuple containing the three components of the ncurses library
   version: *major*, *minor*, and *patch*.  All values are integers.  The
   components can also be accessed by name,  so ``curses.ncurses_version[0]``
   is equivalent to ``curses.ncurses_version.major`` and so on.

   Availability: if the ncurses library is used.

   .. versionadded:: 3.8

.. data:: COLORS

   The maximum number of colors the terminal can support.
   It is defined only after the call to :func:`start_color`.

.. data:: COLOR_PAIRS

   The maximum number of color pairs the terminal can support.
   It is defined only after the call to :func:`start_color`.

.. data:: COLS

   The width of the screen, that is, the number of columns.
   It is defined only after the call to :func:`initscr`.
   Updated by :func:`update_lines_cols`, :func:`resizeterm` and
   :func:`resize_term`.

.. data:: LINES

   The height of the screen, that is, the number of lines.
   It is defined only after the call to :func:`initscr`.
   Updated by :func:`update_lines_cols`, :func:`resizeterm` and
   :func:`resize_term`.


Attributes
~~~~~~~~~~

Some constants are available to specify character cell attributes.
The exact constants available are system dependent.

+------------------------+-------------------------------+
| Attribute              | Meaning                       |
+========================+===============================+
| .. data:: A_ALTCHARSET | Alternate character set mode  |
+------------------------+-------------------------------+
| .. data:: A_BLINK      | Blink mode                    |
+------------------------+-------------------------------+
| .. data:: A_BOLD       | Bold mode                     |
+------------------------+-------------------------------+
| .. data:: A_DIM        | Dim mode                      |
+------------------------+-------------------------------+
| .. data:: A_INVIS      | Invisible or blank mode       |
+------------------------+-------------------------------+
| .. data:: A_ITALIC     | Italic mode                   |
+------------------------+-------------------------------+
| .. data:: A_NORMAL     | Normal attribute              |
+------------------------+-------------------------------+
| .. data:: A_PROTECT    | Protected mode                |
+------------------------+-------------------------------+
| .. data:: A_REVERSE    | Reverse background and        |
|                        | foreground colors             |
+------------------------+-------------------------------+
| .. data:: A_STANDOUT   | Standout mode                 |
+------------------------+-------------------------------+
| .. data:: A_UNDERLINE  | Underline mode                |
+------------------------+-------------------------------+
| .. data:: A_HORIZONTAL | Horizontal highlight          |
+------------------------+-------------------------------+
| .. data:: A_LEFT       | Left highlight                |
+------------------------+-------------------------------+
| .. data:: A_LOW        | Low highlight                 |
+------------------------+-------------------------------+
| .. data:: A_RIGHT      | Right highlight               |
+------------------------+-------------------------------+
| .. data:: A_TOP        | Top highlight                 |
+------------------------+-------------------------------+
| .. data:: A_VERTICAL   | Vertical highlight            |
+------------------------+-------------------------------+

.. versionadded:: 3.7
   ``A_ITALIC`` was added.

.. _curses-wa-constants:

The :meth:`~window.attr_get`, :meth:`~window.attr_set`, :meth:`~window.attr_on`
and :meth:`~window.attr_off` methods use a parallel set of ``WA_*`` constants.
These have the same meaning as the corresponding ``A_*`` attributes above
(``WA_BOLD`` like :const:`A_BOLD`, and so on), but belong to the ``attr_t`` type
rather than being packed into a character.  In ncurses the two sets share the
same values, but other curses implementations may give them different ones, so
use the ``WA_*`` constants with the ``attr_*`` methods.  The available names are
``WA_ATTRIBUTES``, ``WA_NORMAL``, ``WA_STANDOUT``, ``WA_UNDERLINE``,
``WA_REVERSE``, ``WA_BLINK``, ``WA_DIM``, ``WA_BOLD``, ``WA_ALTCHARSET``,
``WA_INVIS``, ``WA_PROTECT``, ``WA_HORIZONTAL``, ``WA_LEFT``, ``WA_LOW``,
``WA_RIGHT``, ``WA_TOP``, ``WA_VERTICAL`` and ``WA_ITALIC`` (each available only
where the platform defines it).

.. versionadded:: next
   The ``WA_*`` constants were added.

Several constants are available to extract corresponding attributes returned
by some methods.

+-------------------------+-------------------------------+
| Bit-mask                | Meaning                       |
+=========================+===============================+
|  .. data:: A_ATTRIBUTES | Bit-mask to extract           |
|                         | attributes                    |
+-------------------------+-------------------------------+
|  .. data:: A_CHARTEXT   | Bit-mask to extract a         |
|                         | character                     |
+-------------------------+-------------------------------+
|  .. data:: A_COLOR      | Bit-mask to extract           |
|                         | color-pair field information  |
+-------------------------+-------------------------------+

Keys
~~~~

Keys are referred to by integer constants with names starting with  ``KEY_``.
The exact keycaps available are system dependent.

+-------------------------+--------------------------------------------+
| Key constant            | Key                                        |
+=========================+============================================+
| .. data:: KEY_MIN       | Minimum key value                          |
+-------------------------+--------------------------------------------+
| .. data:: KEY_BREAK     | Break key (unreliable)                     |
+-------------------------+--------------------------------------------+
| .. data:: KEY_DOWN      | Down-arrow                                 |
+-------------------------+--------------------------------------------+
| .. data:: KEY_UP        | Up-arrow                                   |
+-------------------------+--------------------------------------------+
| .. data:: KEY_LEFT      | Left-arrow                                 |
+-------------------------+--------------------------------------------+
| .. data:: KEY_RIGHT     | Right-arrow                                |
+-------------------------+--------------------------------------------+
| .. data:: KEY_HOME      | Home key (upward+left arrow)               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_BACKSPACE | Backspace (unreliable)                     |
+-------------------------+--------------------------------------------+
| .. data:: KEY_F0        | Function keys.  Up to 64 function keys are |
|                         | supported.                                 |
+-------------------------+--------------------------------------------+
| .. data:: KEY_Fn        | Value of function key *n*                  |
+-------------------------+--------------------------------------------+
| .. data:: KEY_DL        | Delete line                                |
+-------------------------+--------------------------------------------+
| .. data:: KEY_IL        | Insert line                                |
+-------------------------+--------------------------------------------+
| .. data:: KEY_DC        | Delete character                           |
+-------------------------+--------------------------------------------+
| .. data:: KEY_IC        | Insert char or enter insert mode           |
+-------------------------+--------------------------------------------+
| .. data:: KEY_EIC       | Exit insert char mode                      |
+-------------------------+--------------------------------------------+
| .. data:: KEY_CLEAR     | Clear screen                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_EOS       | Clear to end of screen                     |
+-------------------------+--------------------------------------------+
| .. data:: KEY_EOL       | Clear to end of line                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SF        | Scroll 1 line forward                      |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SR        | Scroll 1 line backward (reverse)           |
+-------------------------+--------------------------------------------+
| .. data:: KEY_NPAGE     | Next page                                  |
+-------------------------+--------------------------------------------+
| .. data:: KEY_PPAGE     | Previous page                              |
+-------------------------+--------------------------------------------+
| .. data:: KEY_STAB      | Set tab                                    |
+-------------------------+--------------------------------------------+
| .. data:: KEY_CTAB      | Clear tab                                  |
+-------------------------+--------------------------------------------+
| .. data:: KEY_CATAB     | Clear all tabs                             |
+-------------------------+--------------------------------------------+
| .. data:: KEY_ENTER     | Enter or send (unreliable)                 |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SRESET    | Soft (partial) reset (unreliable)          |
+-------------------------+--------------------------------------------+
| .. data:: KEY_RESET     | Reset or hard reset (unreliable)           |
+-------------------------+--------------------------------------------+
| .. data:: KEY_PRINT     | Print                                      |
+-------------------------+--------------------------------------------+
| .. data:: KEY_LL        | Home down or bottom (lower left)           |
+-------------------------+--------------------------------------------+
| .. data:: KEY_A1        | Upper left of keypad                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_A3        | Upper right of keypad                      |
+-------------------------+--------------------------------------------+
| .. data:: KEY_B2        | Center of keypad                           |
+-------------------------+--------------------------------------------+
| .. data:: KEY_C1        | Lower left of keypad                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_C3        | Lower right of keypad                      |
+-------------------------+--------------------------------------------+
| .. data:: KEY_BTAB      | Back tab                                   |
+-------------------------+--------------------------------------------+
| .. data:: KEY_BEG       | Beg (beginning)                            |
+-------------------------+--------------------------------------------+
| .. data:: KEY_CANCEL    | Cancel                                     |
+-------------------------+--------------------------------------------+
| .. data:: KEY_CLOSE     | Close                                      |
+-------------------------+--------------------------------------------+
| .. data:: KEY_COMMAND   | Cmd (command)                              |
+-------------------------+--------------------------------------------+
| .. data:: KEY_COPY      | Copy                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_CREATE    | Create                                     |
+-------------------------+--------------------------------------------+
| .. data:: KEY_END       | End                                        |
+-------------------------+--------------------------------------------+
| .. data:: KEY_EXIT      | Exit                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_FIND      | Find                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_HELP      | Help                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_MARK      | Mark                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_MESSAGE   | Message                                    |
+-------------------------+--------------------------------------------+
| .. data:: KEY_MOVE      | Move                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_NEXT      | Next                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_OPEN      | Open                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_OPTIONS   | Options                                    |
+-------------------------+--------------------------------------------+
| .. data:: KEY_PREVIOUS  | Prev (previous)                            |
+-------------------------+--------------------------------------------+
| .. data:: KEY_REDO      | Redo                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_REFERENCE | Ref (reference)                            |
+-------------------------+--------------------------------------------+
| .. data:: KEY_REFRESH   | Refresh                                    |
+-------------------------+--------------------------------------------+
| .. data:: KEY_REPLACE   | Replace                                    |
+-------------------------+--------------------------------------------+
| .. data:: KEY_RESTART   | Restart                                    |
+-------------------------+--------------------------------------------+
| .. data:: KEY_RESUME    | Resume                                     |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SAVE      | Save                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SBEG      | Shifted Beg (beginning)                    |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SCANCEL   | Shifted Cancel                             |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SCOMMAND  | Shifted Command                            |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SCOPY     | Shifted Copy                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SCREATE   | Shifted Create                             |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SDC       | Shifted Delete char                        |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SDL       | Shifted Delete line                        |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SELECT    | Select                                     |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SEND      | Shifted End                                |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SEOL      | Shifted Clear line                         |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SEXIT     | Shifted Exit                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SFIND     | Shifted Find                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SHELP     | Shifted Help                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SHOME     | Shifted Home                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SIC       | Shifted Input                              |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SLEFT     | Shifted Left arrow                         |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SMESSAGE  | Shifted Message                            |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SMOVE     | Shifted Move                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SNEXT     | Shifted Next                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SOPTIONS  | Shifted Options                            |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SPREVIOUS | Shifted Prev                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SPRINT    | Shifted Print                              |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SREDO     | Shifted Redo                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SREPLACE  | Shifted Replace                            |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SRIGHT    | Shifted Right arrow                        |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SRSUME    | Shifted Resume                             |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SSAVE     | Shifted Save                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SSUSPEND  | Shifted Suspend                            |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SUNDO     | Shifted Undo                               |
+-------------------------+--------------------------------------------+
| .. data:: KEY_SUSPEND   | Suspend                                    |
+-------------------------+--------------------------------------------+
| .. data:: KEY_UNDO      | Undo                                       |
+-------------------------+--------------------------------------------+
| .. data:: KEY_MOUSE     | Mouse event has occurred                   |
+-------------------------+--------------------------------------------+
| .. data:: KEY_RESIZE    | Terminal resize event                      |
+-------------------------+--------------------------------------------+
| .. data:: KEY_MAX       | Maximum key value                          |
+-------------------------+--------------------------------------------+

On VT100s and their software emulations, such as X terminal emulators, there are
normally at least four function keys (:const:`KEY_F1 <KEY_Fn>`, :const:`KEY_F2 <KEY_Fn>`,
:const:`KEY_F3 <KEY_Fn>`, :const:`KEY_F4 <KEY_Fn>`) available, and the arrow keys mapped to
:const:`KEY_UP`, :const:`KEY_DOWN`, :const:`KEY_LEFT` and :const:`KEY_RIGHT` in
the obvious way.  If your machine has a PC keyboard, it is safe to expect arrow
keys and twelve function keys (older PC keyboards may have only ten function
keys); also, the following keypad mappings are standard:

+------------------+-----------+
| Keycap           | Constant  |
+==================+===========+
| :kbd:`Insert`    | KEY_IC    |
+------------------+-----------+
| :kbd:`Delete`    | KEY_DC    |
+------------------+-----------+
| :kbd:`Home`      | KEY_HOME  |
+------------------+-----------+
| :kbd:`End`       | KEY_END   |
+------------------+-----------+
| :kbd:`Page Up`   | KEY_PPAGE |
+------------------+-----------+
| :kbd:`Page Down` | KEY_NPAGE |
+------------------+-----------+

Alternate character set
~~~~~~~~~~~~~~~~~~~~~~~~

.. _curses-acs-codes:

The following table lists characters from the alternate character set. These are
inherited from the VT100 terminal, and will generally be  available on software
emulations such as X terminals.  When there is no graphic available, curses
falls back on a crude printable ASCII approximation.

.. note::

   These are available only after :func:`initscr` has  been called.

+------------------------+------------------------------------------+
| ACS code               | Meaning                                  |
+========================+==========================================+
| .. data:: ACS_BBSS     | alternate name for upper-right corner    |
+------------------------+------------------------------------------+
| .. data:: ACS_BLOCK    | solid square block                       |
+------------------------+------------------------------------------+
| .. data:: ACS_BOARD    | board of squares                         |
+------------------------+------------------------------------------+
| .. data:: ACS_BSBS     | alternate name for horizontal line       |
+------------------------+------------------------------------------+
| .. data:: ACS_BSSB     | alternate name for upper-left corner     |
+------------------------+------------------------------------------+
| .. data:: ACS_BSSS     | alternate name for top tee               |
+------------------------+------------------------------------------+
| .. data:: ACS_BTEE     | bottom tee                               |
+------------------------+------------------------------------------+
| .. data:: ACS_BULLET   | bullet                                   |
+------------------------+------------------------------------------+
| .. data:: ACS_CKBOARD  | checker board (stipple)                  |
+------------------------+------------------------------------------+
| .. data:: ACS_DARROW   | arrow pointing down                      |
+------------------------+------------------------------------------+
| .. data:: ACS_DEGREE   | degree symbol                            |
+------------------------+------------------------------------------+
| .. data:: ACS_DIAMOND  | diamond                                  |
+------------------------+------------------------------------------+
| .. data:: ACS_GEQUAL   | greater-than-or-equal-to                 |
+------------------------+------------------------------------------+
| .. data:: ACS_HLINE    | horizontal line                          |
+------------------------+------------------------------------------+
| .. data:: ACS_LANTERN  | lantern symbol                           |
+------------------------+------------------------------------------+
| .. data:: ACS_LARROW   | left arrow                               |
+------------------------+------------------------------------------+
| .. data:: ACS_LEQUAL   | less-than-or-equal-to                    |
+------------------------+------------------------------------------+
| .. data:: ACS_LLCORNER | lower-left corner                        |
+------------------------+------------------------------------------+
| .. data:: ACS_LRCORNER | lower-right corner                       |
+------------------------+------------------------------------------+
| .. data:: ACS_LTEE     | left tee                                 |
+------------------------+------------------------------------------+
| .. data:: ACS_NEQUAL   | not-equal sign                           |
+------------------------+------------------------------------------+
| .. data:: ACS_PI       | letter pi                                |
+------------------------+------------------------------------------+
| .. data:: ACS_PLMINUS  | plus-or-minus sign                       |
+------------------------+------------------------------------------+
| .. data:: ACS_PLUS     | big plus sign                            |
+------------------------+------------------------------------------+
| .. data:: ACS_RARROW   | right arrow                              |
+------------------------+------------------------------------------+
| .. data:: ACS_RTEE     | right tee                                |
+------------------------+------------------------------------------+
| .. data:: ACS_S1       | scan line 1                              |
+------------------------+------------------------------------------+
| .. data:: ACS_S3       | scan line 3                              |
+------------------------+------------------------------------------+
| .. data:: ACS_S7       | scan line 7                              |
+------------------------+------------------------------------------+
| .. data:: ACS_S9       | scan line 9                              |
+------------------------+------------------------------------------+
| .. data:: ACS_SBBS     | alternate name for lower-right corner    |
+------------------------+------------------------------------------+
| .. data:: ACS_SBSB     | alternate name for vertical line         |
+------------------------+------------------------------------------+
| .. data:: ACS_SBSS     | alternate name for right tee             |
+------------------------+------------------------------------------+
| .. data:: ACS_SSBB     | alternate name for lower-left corner     |
+------------------------+------------------------------------------+
| .. data:: ACS_SSBS     | alternate name for bottom tee            |
+------------------------+------------------------------------------+
| .. data:: ACS_SSSB     | alternate name for left tee              |
+------------------------+------------------------------------------+
| .. data:: ACS_SSSS     | alternate name for crossover or big plus |
+------------------------+------------------------------------------+
| .. data:: ACS_STERLING | pound sterling                           |
+------------------------+------------------------------------------+
| .. data:: ACS_TTEE     | top tee                                  |
+------------------------+------------------------------------------+
| .. data:: ACS_UARROW   | up arrow                                 |
+------------------------+------------------------------------------+
| .. data:: ACS_ULCORNER | upper-left corner                        |
+------------------------+------------------------------------------+
| .. data:: ACS_URCORNER | upper-right corner                       |
+------------------------+------------------------------------------+
| .. data:: ACS_VLINE    | vertical line                            |
+------------------------+------------------------------------------+

Mouse buttons
~~~~~~~~~~~~~

The following table lists mouse button constants used by :meth:`getmouse`:

+----------------------------------+---------------------------------------------+
| Mouse button constant            | Meaning                                     |
+==================================+=============================================+
| .. data:: BUTTONn_PRESSED        | Mouse button *n* pressed                    |
+----------------------------------+---------------------------------------------+
| .. data:: BUTTONn_RELEASED       | Mouse button *n* released                   |
+----------------------------------+---------------------------------------------+
| .. data:: BUTTONn_CLICKED        | Mouse button *n* clicked                    |
+----------------------------------+---------------------------------------------+
| .. data:: BUTTONn_DOUBLE_CLICKED | Mouse button *n* double clicked             |
+----------------------------------+---------------------------------------------+
| .. data:: BUTTONn_TRIPLE_CLICKED | Mouse button *n* triple clicked             |
+----------------------------------+---------------------------------------------+
| .. data:: BUTTON_SHIFT           | Shift was down during button state change   |
+----------------------------------+---------------------------------------------+
| .. data:: BUTTON_CTRL            | Control was down during button state change |
+----------------------------------+---------------------------------------------+
| .. data:: BUTTON_ALT             | Alt was down during button state change     |
+----------------------------------+---------------------------------------------+

.. versionchanged:: 3.10
   The ``BUTTON5_*`` constants are now exposed if they are provided by the
   underlying curses library.

Colors
~~~~~~

The following table lists the predefined colors:

+-------------------------+----------------------------+
| Constant                | Color                      |
+=========================+============================+
| .. data:: COLOR_BLACK   | Black                      |
+-------------------------+----------------------------+
| .. data:: COLOR_BLUE    | Blue                       |
+-------------------------+----------------------------+
| .. data:: COLOR_CYAN    | Cyan (light greenish blue) |
+-------------------------+----------------------------+
| .. data:: COLOR_GREEN   | Green                      |
+-------------------------+----------------------------+
| .. data:: COLOR_MAGENTA | Magenta (purplish red)     |
+-------------------------+----------------------------+
| .. data:: COLOR_RED     | Red                        |
+-------------------------+----------------------------+
| .. data:: COLOR_WHITE   | White                      |
+-------------------------+----------------------------+
| .. data:: COLOR_YELLOW  | Yellow                     |
+-------------------------+----------------------------+


:mod:`!curses.textpad` --- Text input widget for curses programs
================================================================

.. module:: curses.textpad
   :synopsis: Emacs-like input editing in a curses window.

The :mod:`!curses.textpad` module provides a :class:`Textbox` class that handles
elementary text editing in a curses window, supporting a set of keybindings
resembling those of Emacs (thus, also of Netscape Navigator, BBedit 6.x,
FrameMaker, and many other programs).  The module also provides a
rectangle-drawing function useful for framing text boxes or for other purposes.

The module :mod:`!curses.textpad` defines the following function:


.. function:: rectangle(win, uly, ulx, lry, lrx)

   Draw a rectangle.  The first argument must be a window object; the remaining
   arguments are coordinates relative to that window.  The second and third
   arguments are the y and x coordinates of the upper-left corner of the
   rectangle to be drawn; the fourth and fifth arguments are the y and x
   coordinates of the lower-right corner. The rectangle will be drawn using
   VT100/IBM PC forms characters on terminals that make this possible (including
   xterm and most other software terminal emulators).  Otherwise it will be drawn
   with ASCII  dashes, vertical bars, and plus signs.


.. _curses-textpad-objects:

Textbox objects
---------------

You can instantiate a :class:`Textbox` object as follows:


.. class:: Textbox(win, insert_mode=False)

   Return a textbox widget object.  The *win* argument should be a curses
   :ref:`window <curses-window-objects>` object in which the textbox is to
   be contained.  If *insert_mode* is true, the textbox inserts typed
   characters, shifting existing text to the right, rather than overwriting it.
   The edit cursor of the textbox is initially located at the
   upper-left corner of the containing window, with coordinates ``(0, 0)``.
   The instance's :attr:`stripspaces` flag is initially on.

   .. versionchanged:: next
      Entering and reading back the full Unicode range, including combining
      characters, is now supported when curses is built with wide-character
      support.

   :class:`Textbox` objects have the following methods:


   .. method:: edit(validate=None)

      This is the entry point you will normally use.  It accepts editing
      keystrokes until one of the termination keystrokes is entered.  If
      *validate* is supplied, it must be a function.  It will be called for
      each keystroke entered with the keystroke as a parameter; command dispatch
      is done on the result.  If it returns a false value, the keystroke is
      ignored.  This method returns the window contents as a
      string; whether blanks in the window are included is affected by the
      :attr:`stripspaces` attribute.

      .. versionchanged:: next
         *validate* is now called with a non-ASCII character as a string;
         other keystrokes are still passed as an integer.


   .. method:: do_command(ch)

      Process a single command keystroke.  Returns ``1`` to continue editing,
      or ``0`` if a termination keystroke was processed.  Here are the supported
      special keystrokes:

      +------------------+-------------------------------------------+
      | Keystroke        | Action                                    |
      +==================+===========================================+
      | :kbd:`Control-A` | Go to left edge of window.                |
      +------------------+-------------------------------------------+
      | :kbd:`Control-B` | Cursor left, wrapping to previous line if |
      |                  | appropriate.                              |
      +------------------+-------------------------------------------+
      | :kbd:`Control-D` | Delete character under cursor.            |
      +------------------+-------------------------------------------+
      | :kbd:`Control-E` | Go to right edge (stripspaces off) or end |
      |                  | of line (stripspaces on).                 |
      +------------------+-------------------------------------------+
      | :kbd:`Control-F` | Cursor right, wrapping to next line when  |
      |                  | appropriate.                              |
      +------------------+-------------------------------------------+
      | :kbd:`Control-G` | Terminate, returning the window contents. |
      +------------------+-------------------------------------------+
      | :kbd:`Control-H` | Delete character backward.                |
      +------------------+-------------------------------------------+
      | :kbd:`Control-J` | Terminate if the window is 1 line,        |
      |                  | otherwise move to the start of the next   |
      |                  | line.                                     |
      +------------------+-------------------------------------------+
      | :kbd:`Control-K` | If line is blank, delete it, otherwise    |
      |                  | clear to end of line.                     |
      +------------------+-------------------------------------------+
      | :kbd:`Control-L` | Refresh screen.                           |
      +------------------+-------------------------------------------+
      | :kbd:`Control-N` | Cursor down; move down one line.          |
      +------------------+-------------------------------------------+
      | :kbd:`Control-O` | Insert a blank line at cursor location.   |
      +------------------+-------------------------------------------+
      | :kbd:`Control-P` | Cursor up; move up one line.              |
      +------------------+-------------------------------------------+

      Move operations do nothing if the cursor is at an edge where the movement
      is not possible.  The following synonyms are supported where possible:

      +--------------------------------+------------------+
      | Constant                       | Keystroke        |
      +================================+==================+
      | :const:`~curses.KEY_LEFT`      | :kbd:`Control-B` |
      +--------------------------------+------------------+
      | :const:`~curses.KEY_RIGHT`     | :kbd:`Control-F` |
      +--------------------------------+------------------+
      | :const:`~curses.KEY_UP`        | :kbd:`Control-P` |
      +--------------------------------+------------------+
      | :const:`~curses.KEY_DOWN`      | :kbd:`Control-N` |
      +--------------------------------+------------------+
      | :const:`~curses.KEY_BACKSPACE` | :kbd:`Control-h` |
      +--------------------------------+------------------+

      All other keystrokes are treated as a command to insert the given
      character and move right (with line wrapping).


   .. method:: gather()

      Return the window contents as a string; whether blanks in the
      window are included is affected by the :attr:`stripspaces` member.


   .. attribute:: stripspaces

      This attribute is a flag which controls the interpretation of blanks in
      the window.  When it is on, trailing blanks on each line are ignored; any
      cursor motion that would land the cursor on a trailing blank goes to the
      end of that line instead, and trailing blanks are stripped when the window
      contents are gathered.
