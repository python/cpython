ar - POSIX 1003.2 interface to library files

Here's the source and PowerPC binary for a POSIX 1003.2 interface "ar"
command; this is extremely useful when you're porting complex UNIX/POSIX
software to BeOS for PowerPC (I originally wrote it to support my Python
port).

To build/install ar, do this in a Terminal:

make nodebug install

This will create ar and ar-posix (a symlink to ar) in ~/config/bin.  The
ar-posix symlink is to make things a little easier if you happen to
have GeekGadgets (see www.ninemoons.com) installed; it comes with an
ar that only works on objects/libraries produced by GNU C for BeOS.

To use the POSIX ar with your port, do something like this:

AR=ar-posix ./configure ... normal configure arguments ...

and then:

make AR=ar-posix

You may need to check the Makefiles; people seem to be quite sloppy about
using just plain "ar cr libfoo.a ..." instead of "$(AR) cr libfoo.a ...".

- Chris Herborth, April 18, 1998
  (chrish@kagi.com)
