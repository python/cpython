#!/usr/bin/env python
# This file is meant as an executable script for running applets.
# BuildApplet will use it as the main executable in the .app bundle if
# we are not running in a framework build.

import os
import sys
for name in ["__rawmain__.py", "__rawmain__.pyc", "__main__.py", "__main__.pyc"]:
    realmain = os.path.join(os.path.dirname(os.path.dirname(sys.argv[0])),
                      "Resources", name)
    if os.path.exists(realmain):
        break
else:
    sys.stderr.write("%s: cannot find applet main program\n" % sys.argv[0])
    sys.exit(1)
sys.argv.insert(1, realmain)
os.execve(sys.executable, sys.argv, os.environ)
