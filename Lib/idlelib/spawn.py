# spawn - This is ugly, OS-specific code to spawn a separate process.  It
#         also defines a function for getting the version of a path most
#         likely to work with cranky API functions.

import os

def hardpath(path):
    path = os.path.normcase(os.path.abspath(path))
    try:
        import win32api
        path = win32api.GetShortPathName( path )
    except:
        pass
    return path

if hasattr(os, 'fork'):

  # UNIX-ish operating system: we fork() and exec(), and we have to track
  #   the pids of our children and call waitpid() on them to avoid leaving
  #   zombies in the process table.  kill_zombies() does the dirty work, and
  #   should be called periodically.

  zombies = []

  def spawn(bin, *args):
    pid = os.fork()
    if pid:
      zombies.append(pid)
    else:
      os.execv( bin, (bin, ) + args )

  def kill_zombies():
      for z in zombies[:]:
          stat = os.waitpid(z, os.WNOHANG)
          if stat[0]==z:
              zombies.remove(z)
elif hasattr(os, 'spawnv'):

  # Windows-ish OS: we use spawnv(), and stick quotes around arguments
  #   in case they contains spaces, since Windows will jam all the
  #   arguments to spawn() or exec() together into one string.  The
  #   kill_zombies function is a noop.

  def spawn(bin, *args):
    nargs = ['"'+bin+'"']
    for arg in args:
      nargs.append( '"'+arg+'"' )
    os.spawnv( os.P_NOWAIT, bin, nargs )

  def kill_zombies(): pass

else:
  # If you get here, you may be able to write an alternative implementation
  # of these functions for your OS.

  def kill_zombies(): pass

  raise OSError, 'This OS does not support fork() or spawnv().'
