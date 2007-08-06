# Coroutine implementation using Python threads.
#
# Combines ideas from Guido's Generator module, and from the coroutine
# features of Icon and Simula 67.
#
# To run a collection of functions as coroutines, you need to create
# a Coroutine object to control them:
#    co = Coroutine()
# and then 'create' a subsidiary object for each function in the
# collection:
#    cof1 = co.create(f1 [, arg1, arg2, ...]) # [] means optional,
#    cof2 = co.create(f2 [, arg1, arg2, ...]) #... not list
#    cof3 = co.create(f3 [, arg1, arg2, ...])
# etc.  The functions need not be distinct; 'create'ing the same
# function multiple times gives you independent instances of the
# function.
#
# To start the coroutines running, use co.tran on one of the create'd
# functions; e.g., co.tran(cof2).  The routine that first executes
# co.tran is called the "main coroutine".  It's special in several
# respects:  it existed before you created the Coroutine object; if any of
# the create'd coroutines exits (does a return, or suffers an unhandled
# exception), EarlyExit error is raised in the main coroutine; and the
# co.detach() method transfers control directly to the main coroutine
# (you can't use co.tran() for this because the main coroutine doesn't
# have a name ...).
#
# Coroutine objects support these methods:
#
# handle = .create(func [, arg1, arg2, ...])
#    Creates a coroutine for an invocation of func(arg1, arg2, ...),
#    and returns a handle ("name") for the coroutine so created.  The
#    handle can be used as the target in a subsequent .tran().
#
# .tran(target, data=None)
#    Transfer control to the create'd coroutine "target", optionally
#    passing it an arbitrary piece of data. To the coroutine A that does
#    the .tran, .tran acts like an ordinary function call:  another
#    coroutine B can .tran back to it later, and if it does A's .tran
#    returns the 'data' argument passed to B's tran.  E.g.,
#
#    in coroutine coA   in coroutine coC    in coroutine coB
#      x = co.tran(coC)   co.tran(coB)        co.tran(coA,12)
#      print x # 12
#
#    The data-passing feature is taken from Icon, and greatly cuts
#    the need to use global variables for inter-coroutine communication.
#
# .back( data=None )
#    The same as .tran(invoker, data=None), where 'invoker' is the
#    coroutine that most recently .tran'ed control to the coroutine
#    doing the .back.  This is akin to Icon's "&source".
#
# .detach( data=None )
#    The same as .tran(main, data=None), where 'main' is the
#    (unnameable!) coroutine that started it all.  'main' has all the
#    rights of any other coroutine:  upon receiving control, it can
#    .tran to an arbitrary coroutine of its choosing, go .back to
#    the .detach'er, or .kill the whole thing.
#
# .kill()
#    Destroy all the coroutines, and return control to the main
#    coroutine.  None of the create'ed coroutines can be resumed after a
#    .kill().  An EarlyExit exception does a .kill() automatically.  It's
#    a good idea to .kill() coroutines you're done with, since the
#    current implementation consumes a thread for each coroutine that
#    may be resumed.

import thread
import sync

class _CoEvent:
    def __init__(self, func):
        self.f = func
        self.e = sync.event()

    def __repr__(self):
        if self.f is None:
            return 'main coroutine'
        else:
            return 'coroutine for func ' + self.f.__name__

    def __hash__(self):
        return id(self)

    def __cmp__(x,y):
        return cmp(id(x), id(y))

    def resume(self):
        self.e.post()

    def wait(self):
        self.e.wait()
        self.e.clear()

Killed = 'Coroutine.Killed'
EarlyExit = 'Coroutine.EarlyExit'

class Coroutine:
    def __init__(self):
        self.active = self.main = _CoEvent(None)
        self.invokedby = {self.main: None}
        self.killed = 0
        self.value  = None
        self.terminated_by = None

    def create(self, func, *args):
        me = _CoEvent(func)
        self.invokedby[me] = None
        thread.start_new_thread(self._start, (me,) + args)
        return me

    def _start(self, me, *args):
        me.wait()
        if not self.killed:
            try:
                try:
                    me.f(*args)
                except Killed:
                    pass
            finally:
                if not self.killed:
                    self.terminated_by = me
                    self.kill()

    def kill(self):
        if self.killed:
            raise TypeError('kill() called on dead coroutines')
        self.killed = 1
        for coroutine in self.invokedby.keys():
            coroutine.resume()

    def back(self, data=None):
        return self.tran( self.invokedby[self.active], data )

    def detach(self, data=None):
        return self.tran( self.main, data )

    def tran(self, target, data=None):
        if target not in self.invokedby:
            raise TypeError('.tran target %r is not an active coroutine' % (target,))
        if self.killed:
            raise TypeError('.tran target %r is killed' % (target,))
        self.value = data
        me = self.active
        self.invokedby[target] = me
        self.active = target
        target.resume()

        me.wait()
        if self.killed:
            if self.main is not me:
                raise Killed
            if self.terminated_by is not None:
                raise EarlyExit('%r terminated early' % (self.terminated_by,))

        return self.value

# end of module
