# Everything is done inside the loader function so that no other names
# are placed in the global namespace.  Before user code is executed,
# even this name is unbound.
def loader():
    import sys, os, protocol, threading, time
    import Remote

##  Use to debug the loading process itself:
##    sys.stdout = open('c:\\windows\\desktop\\stdout.txt','a')
##    sys.stderr = open('c:\\windows\\desktop\\stderr.txt','a')

    # Ensure that there is absolutely no pollution of the global
    # namespace by deleting the global name of this function.
    global loader
    del loader

    # Connect to IDLE
    try:
        client = protocol.Client()
    except protocol.connectionLost, cL:
        print 'loader: Unable to connect to IDLE', cL
        return

    # Connect to an ExecBinding object that needs our help.  If
    # the user is starting multiple programs right now, we might get a
    # different one than the one that started us.  Proving that's okay is
    # left as an exercise to the reader.  (HINT:  Twelve, by the pigeonhole
    # principle)
    ExecBinding = client.getobject('ExecBinding')
    if not ExecBinding:
        print "loader: IDLE does not need me."
        return

    # All of our input and output goes through ExecBinding.
    sys.stdin  = Remote.pseudoIn( ExecBinding.readline )
    sys.stdout = Remote.pseudoOut( ExecBinding.write.void, tag="stdout" )
    sys.stderr = Remote.pseudoOut( ExecBinding.write.void, tag="stderr" )

    # Create a Remote object and start it running.
    remote = Remote.Remote(globals(), ExecBinding)
    rthread = threading.Thread(target=remote.mainloop)
    rthread.setDaemon(1)
    rthread.start()

    # Block until either the client or the user program stops
    user = rthread.isAlive
    while user and client.isAlive():
        time.sleep(0.025)

        if not user():
          user = hasattr(sys, "ready_to_exit") and sys.ready_to_exit
          for t in threading.enumerate():
            if not t.isDaemon() and t.isAlive() and t!=threading.currentThread():
              user = t.isAlive
              break

    # We need to make sure we actually exit, so that the user doesn't get
    #   stuck with an invisible process.  We want to finalize C modules, so
    #   we don't use os._exit(), but we don't call sys.exitfunc, which might
    #   block forever.
    del sys.exitfunc
    sys.exit()

loader()
