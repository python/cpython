"""Miscellaneous support code shared by some of the tool scripts.

This includes option parsing code, HTML formatting code, and a couple of
useful helpers.

"""
__version__ = '$Revision$'


import getopt
import string
import sys


class Options:
    __short_args = "a:c:ho:"
    __long_args = [
        # script controls
        "columns=", "help", "output=",

        # content components
        "address=", "iconserver=",
        "title=", "uplink=", "uptitle="]

    outputfile = "-"
    columns = 1
    letters = 0
    uplink = "index.html"
    uptitle = "Python Documentation Index"

    # The "Aesop Meta Tag" is poorly described, and may only be used
    # by the Aesop search engine (www.aesop.com), but doesn't hurt.
    #
    # There are a number of values this may take to roughly categorize
    # a page.  A page should be marked according to its primary
    # category.  Known values are:
    #   'personal'    -- personal-info
    #   'information' -- information
    #   'interactive' -- interactive media
    #   'multimedia'  -- multimedia presenetation (non-sales)
    #   'sales'       -- sales material
    #   'links'       -- links to other information pages
    #
    # Setting the aesop_type value to one of these strings will cause
    # get_header() to add the appropriate <meta> tag to the <head>.
    #
    aesop_type = None

    def __init__(self):
        self.args = []
        self.variables = {"address": "",
                          "iconserver": "icons",
                          "imgtype": "gif",
                          "title": "Global Module Index",
                          }

    def add_args(self, short=None, long=None):
        if short:
            self.__short_args = self.__short_args + short
        if long:
            self.__long_args = self.__long_args + long

    def parse(self, args):
        try:
            opts, args = getopt.getopt(args, self.__short_args,
                                       self.__long_args)
        except getopt.error:
            sys.stdout = sys.stderr
            self.usage()
            sys.exit(2)
        self.args = self.args + args
        for opt, val in opts:
            if opt in ("-a", "--address"):
                val = string.strip(val)
                if val:
                    val = "<address>\n%s\n</address>\n" % val
                    self.variables["address"] = val
            elif opt in ("-h", "--help"):
                self.usage()
                sys.exit()
            elif opt in ("-o", "--output"):
                self.outputfile = val
            elif opt in ("-c", "--columns"):
                self.columns = int(val)
            elif opt == "--title":
                self.variables["title"] = val.strip()
            elif opt == "--uplink":
                self.uplink = val.strip()
            elif opt == "--uptitle":
                self.uptitle = val.strip()
            elif opt == "--iconserver":
                self.variables["iconserver"] = val.strip() or "."
            else:
                self.handle_option(opt, val)
        if self.uplink and self.uptitle:
            self.variables["uplinkalt"] = "up"
            self.variables["uplinkicon"] = "up"
        else:
            self.variables["uplinkalt"] = ""
            self.variables["uplinkicon"] = "blank"
        self.variables["uplink"] = self.uplink
        self.variables["uptitle"] = self.uptitle

    def handle_option(self, opt, val):
        raise getopt.error("option %s not recognized" % opt)

    def get_header(self):
        s = HEAD % self.variables
        if self.uplink:
            if self.uptitle:
                link = ('<link rel="up" href="%s" title="%s">'
                        % (self.uplink, self.uptitle))
            else:
                link = '<link rel="up" href="%s">' % self.uplink
            repl = "  %s\n</head>" % link
            s = s.replace("</head>", repl, 1)
        if self.aesop_type:
            meta = '\n  <meta name="aesop" content="%s">'
            # Insert this in the middle of the head that's been
            # generated so far, keeping <meta> and <link> elements in
            # neat groups:
            s = s.replace("<link ", meta + "<link ", 1)
        return s

    def get_footer(self):
        return TAIL % self.variables

    def get_output_file(self, filename=None):
        if filename is None:
            filename = self.outputfile
        if filename == "-":
            return sys.stdout
        else:
            return open(filename, "w")


NAVIGATION = '''\
<div class="navigation">
<table width="100%%" cellpadding="0" cellspacing="2">
<tr>
<td><img width="32" height="32" align="bottom" border="0" alt=""
 src="%(iconserver)s/blank.%(imgtype)s"></td>
<td><a href="%(uplink)s"
 title="%(uptitle)s"><img width="32" height="32" align="bottom" border="0"
 alt="%(uplinkalt)s"
 src="%(iconserver)s/%(uplinkicon)s.%(imgtype)s"></a></td>
<td><img width="32" height="32" align="bottom" border="0" alt=""
 src="%(iconserver)s/blank.%(imgtype)s"></td>
<td align="center" width="100%%">%(title)s</td>
<td><img width="32" height="32" align="bottom" border="0" alt=""
 src="%(iconserver)s/blank.%(imgtype)s"></td>
<td><img width="32" height="32" align="bottom" border="0" alt=""
 src="%(iconserver)s/blank.%(imgtype)s"></td>
<td><img width="32" height="32" align="bottom" border="0" alt=""
 src="%(iconserver)s/blank.%(imgtype)s"></td>
</tr></table>
<b class="navlabel">Up:</b> <span class="sectref"><a href="%(uplink)s"
 title="%(uptitle)s">%(uptitle)s</A></span>
<br></div>
'''

HEAD = '''\
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
  <title>%(title)s</title>
  <meta name="description" content="%(title)s">
  <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
  <link rel="STYLESHEET" href="lib/lib.css">
</head>
<body>
''' + NAVIGATION + '''\
<hr>

<h2>%(title)s</h2>

'''

TAIL = "<hr>\n" + NAVIGATION + '''\
%(address)s</body>
</html>
'''
