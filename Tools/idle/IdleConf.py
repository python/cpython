"""Provides access to configuration information"""

import os
import sys
from ConfigParser import ConfigParser, NoOptionError, NoSectionError

class IdleConfParser(ConfigParser):

    # these conf sections do not define extensions!
    builtin_sections = {}
    for section in ('EditorWindow', 'Colors'):
        builtin_sections[section] = section
    
    def getcolor(self, sec, name):
        """Return a dictionary with foreground and background colors

        The return value is appropriate for passing to Tkinter in, e.g.,
        a tag_config call.
        """
	fore = self.getdef(sec, name + "-foreground")
	back = self.getdef(sec, name + "-background")
        return {"foreground": fore,
                "background": back}

    def getdef(self, sec, options, raw=0, vars=None, default=None):
        """Get an option value for given section or return default"""
	try:
            return self.get(sec, options, raw, vars)
	except (NoSectionError, NoOptionError):
	    return default

    def getsection(self, section):
        """Return a SectionConfigParser object"""
        return SectionConfigParser(section, self)

    def getextensions(self):
        exts = []
        for sec in self.sections():
            if self.builtin_sections.has_key(sec):
		continue
	    # enable is a bool, but it may not be defined
	    if self.getdef(sec, 'enable') != '0':
		exts.append(sec)
        return exts

    def reload(self):
        global idleconf
        idleconf = IdleConfParser()
        load(_dir) # _dir is a global holding the last directory loaded

class SectionConfigParser:
    """A ConfigParser object specialized for one section

    This class has all the get methods that a regular ConfigParser does,
    but without requiring a section argument.
    """
    def __init__(self, section, config):
        self.section = section
        self.config = config

    def options(self):
        return self.config.options(self.section)

    def get(self, options, raw=0, vars=None):
        return self.config.get(self.section, options, raw, vars)

    def getdef(self, options, raw=0, vars=None, default=None):
        return self.config.getdef(self.section, options, raw, vars, default)

    def getint(self, option):
        return self.config.getint(self.section, option)
    
    def getfloat(self, option):
        return self.config.getint(self.section, option)
    
    def getboolean(self, option):
        return self.config.getint(self.section, option)

    def getcolor(self, option):
        return self.config.getcolor(self.section, option)

def load(dir):
    """Load IDLE configuration files based on IDLE install in dir

    Attempts to load two config files:
    dir/config.txt
    dir/config-[win/mac/unix].txt
    dir/config-%(sys.platform)s.txt
    ~/.idle
    """
    global _dir
    _dir = dir

    if sys.platform[:3] == 'win':
        genplatfile = os.path.join(dir, "config-win.txt")
    # XXX don't know what the platform string is on a Mac
    elif sys.platform[:3] == 'mac':
        genplatfile = os.path.join(dir, "config-mac.txt")
    else:
        genplatfile = os.path.join(dir, "config-unix.txt")
        
    platfile = os.path.join(dir, "config-%s.txt" % sys.platform)

    try:
        homedir = os.environ['HOME']
    except KeyError:
        homedir = os.getcwd()

    idleconf.read((os.path.join(dir, "config.txt"), genplatfile, platfile,
                   os.path.join(homedir, ".idle")))

idleconf = IdleConfParser()

