# If you put 'import stdwin_gl' in front of the main program of a program
# using stdwin (before it has a chance to import the real stdwin!),
# it will use glstdwin and think it is stdwin.

import sys
if sys.modules.has_key('stdwin'):
	raise RuntimeError, 'too late -- stdwin has already been imported'

import glstdwin
sys.modules['stdwin'] = glstdwin
