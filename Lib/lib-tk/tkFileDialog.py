#
# Instant Python
# $Id$
#
# tk common file dialogues
#
# this module provides interfaces to the native file dialogues
# available in Tk 4.2 and newer, and the directory dialogue available
# in Tk 8.3 and newer.
#
# written by Fredrik Lundh, May 1997.
#

#
# options (all have default values):
#
# - defaultextension: added to filename if not explicitly given
#
# - filetypes: sequence of (label, pattern) tuples.  the same pattern
#   may occur with several patterns.  use "*" as pattern to indicate
#   all files.
#
# - initialdir: initial directory.  preserved by dialog instance.
#
# - initialfile: initial file (ignored by the open dialog).  preserved
#   by dialog instance.
#
# - parent: which window to place the dialog on top of
#
# - title: dialog title
#
# options for the directory chooser:
#
# - initialdir, parent, title: see above
#
# - mustexist: if true, user must pick an existing directory
#

from tkCommonDialog import Dialog

class _Dialog(Dialog):

    def _fixoptions(self):
        try:
            # make sure "filetypes" is a tuple
            self.options["filetypes"] = tuple(self.options["filetypes"])
        except KeyError:
            pass

    def _fixresult(self, widget, result):
        if result:
            # keep directory and filename until next time
            import os
            path, file = os.path.split(result)
            self.options["initialdir"] = path
            self.options["initialfile"] = file
        self.filename = result # compatibility
        return result


#
# file dialogs

class Open(_Dialog):
    "Ask for a filename to open"

    command = "tk_getOpenFile"

class SaveAs(_Dialog):
    "Ask for a filename to save as"

    command = "tk_getSaveFile"


# the directory dialog has its own _fix routines.
class Directory(Dialog):
    "Ask for a directory"

    command = "tk_chooseDirectory"

    def _fixresult(self, widget, result):
        if result:
            # keep directory until next time
            self.options["initialdir"] = result
        self.directory = result # compatibility
        return result

#
# convenience stuff

def askopenfilename(**options):
    "Ask for a filename to open"

    return Open(**options).show()

def asksaveasfilename(**options):
    "Ask for a filename to save as"

    return SaveAs(**options).show()

# FIXME: are the following two perhaps a bit too convenient?

def askopenfile(mode = "r", **options):
    "Ask for a filename to open, and returned the opened file"

    filename = Open(**options).show()
    if filename:
        return open(filename, mode)
    return None

def asksaveasfile(mode = "w", **options):
    "Ask for a filename to save as, and returned the opened file"

    filename = SaveAs(**options).show()
    if filename:
        return open(filename, mode)
    return None

def askdirectory (**options):
    "Ask for a directory, and return the file name"
    return Directory(**options).show()

# --------------------------------------------------------------------
# test stuff

if __name__ == "__main__":
    # Since the file name may contain non-ASCII characters, we need
    # to find an encoding that likely supports the file name, and
    # displays correctly on the terminal.

    # Start off with UTF-8
    enc = "utf-8"
    import sys

    # See whether CODESET is defined
    try:
        import locale
        locale.setlocale(locale.LC_ALL,'')
        enc = locale.nl_langinfo(locale.CODESET)
    except (ImportError, AttributeError):
        pass

    # dialog for openening files

    openfilename=askopenfilename(filetypes=[("all files", "*")])
    try:
        fp=open(openfilename,"r")
        fp.close()
    except:
        print "Could not open File: " 
        print sys.exc_info()[1]

    print "open", openfilename.encode(enc)

    # dialog for saving files

    saveasfilename=asksaveasfilename()
    print "saveas", saveasfilename.encode(enc)

