#
# Minimal "re" compatibility wrapper
#
# If your regexps don't work well under 2.0b1, you can switch
# to the old engine ("pre") down below.
#
# To help us fix any remaining bugs in the new engine, please
# report what went wrong.  You can either use the following web
# page:
#
#    http://sourceforge.net/bugs/?group_id=5470
#
# or send a mail to SRE's author:
#
#    Fredrik Lundh <effbot@telia.com>
#
# Make sure to include the pattern, the string SRE failed to
# match, and what result you expected.
#
# thanks /F
#

engine = "sre"
# engine = "pre"

if engine == "sre":
    # New unicode-aware engine
    from sre import *
else:
    # Old 1.5.2 engine.  This one supports 8-bit strings only,
    # and will be removed in 2.0 final.
    from pre import *
