#! /usr/bin/env python

import os
import sys
import IdleConf

idle_dir = os.path.dirname(IdleConf.__file__)
IdleConf.load(idle_dir)

# defer importing Pyshell until IdleConf is loaded
import PyShell
PyShell.main()
