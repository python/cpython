#
# Instant Python
# $Id$
#
# tk common colour chooser dialogue
#
# this module provides an interface to the native color dialogue
# available in Tk 4.2 and newer.
#
# written by Fredrik Lundh, May 1997
#

#
# options (all have default values):
#
# - initialcolor: colour to mark as selected when dialog is displayed
#   (given as an RGB triplet or a Tk color string)
#
# - parent: which window to place the dialog on top of
#
# - title: dialog title
#

# FIXME: as of Tk 8.0a2, the Unix colour picker is really ugly, and
# doesn't seem to work properly on true colour displays.  maybe we
# should use the instant python version instead?

from tkCommonDialog import Dialog


#
# color chooser class

class Chooser(Dialog):
    "Ask for a color"

    command = "tk_chooseColor"

    def _fixoptions(self):
        try:
            # make sure initialcolor is a tk color string
            color = self.options["initialcolor"]
            if type(color) == type(()):
                # assume an RGB triplet
                self.options["initialcolor"] = "%02x%02x%02x" % color
        except KeyError:
            pass

    def _fixresult(self, widget, result):
        # to simplify application code, the color chooser returns
        # an RGB tuple together with the Tk color string
        if not result:
            return None, None # cancelled
        r, g, b = widget.winfo_rgb(result)
        return (r/256, g/256, b/256), result


#
# convenience stuff

def askcolor(color = None, **options):
    "Ask for a color"

    return apply(Chooser, (), options).show()


# --------------------------------------------------------------------
# test stuff

if __name__ == "__main__":

    print "color", askcolor()

