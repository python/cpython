import os
import gettext

def get_qualified_path(name):
    """Return a more qualified path to name"""
    import sys
    import os
    path = sys.path
    try:
        path = [os.path.dirname(__file__)] + path
    except NameError:
        pass
    for dir in path:
        fullname = os.path.join(dir, name)
        if os.path.exists(fullname):
            return fullname
    return name

# Test basic interface
os.environ['LANGUAGE'] = 'xx'

mofile = get_qualified_path('xx')
localedir = os.path.dirname(mofile)

print 'installing gettext'
gettext.install()

print _('calling bindtextdomain with localedir %s') % localedir
print gettext.bindtextdomain('gettext', localedir)
print gettext.bindtextdomain()

print gettext.textdomain('gettext')
print gettext.textdomain()

# test some translations
print _(u'mullusk')
print _(r'Raymond Luxury Yach-t')
print _(ur'nudge nudge')

# double quotes
print _(u"mullusk")
print _(r"Raymond Luxury Yach-t")
print _(ur"nudge nudge")

# triple single quotes
print _(u'''mullusk''')
print _(r'''Raymond Luxury Yach-t''')
print _(ur'''nudge nudge''')

# triple double quotes
print _(u"""mullusk""")
print _(r"""Raymond Luxury Yach-t""")
print _(ur"""nudge nudge""")

# multiline strings
print _('''This module provides internationalization and localization
support for your Python programs by providing an interface to the GNU
gettext message catalog library.''')

print gettext.dgettext('gettext', 'nudge nudge')

# dcgettext
##import locale
##if gettext.dcgettext('gettext', 'nudge nudge',
##                     locale.LC_MESSAGES) <> 'wink wink':
##    print _('dcgettext failed')

# test the alternative interface
fp = open(os.path.join(mofile, 'LC_MESSAGES', 'gettext.mo'), 'rb')
t = gettext.GNUTranslations(fp)
fp.close()

gettext.set(t)
print t == gettext.get()

print _('nudge nudge')
