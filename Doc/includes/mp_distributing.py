#
# Module to allow spawning of processes on foreign host
#
# Depends on `multiprocessing` package -- tested with `processing-0.60`
#
# Copyright (c) 2006-2008, R Oudkerk
# All rights reserved.
#

__all__ = ['Cluster', 'Host', 'get_logger', 'current_process']

#
# Imports
#

import sys
import os
import tarfile
import shutil
import subprocess
import logging
import itertools
import Queue

try:
    import cPickle as pickle
except ImportError:
    import pickle

from multiprocessing import Process, current_process, cpu_count
from multiprocessing import util, managers, connection, forking, pool

#
# Logging
#

def get_logger():
    return _logger

_logger = logging.getLogger('distributing')
_logger.propagate = 0

_formatter = logging.Formatter(util.DEFAULT_LOGGING_FORMAT)
_handler = logging.StreamHandler()
_handler.setFormatter(_formatter)
_logger.addHandler(_handler)

info = _logger.info
debug = _logger.debug

#
# Get number of cpus
#

try:
    slot_count = cpu_count()
except NotImplemented:
    slot_count = 1

#
# Manager type which spawns subprocesses
#

class HostManager(managers.SyncManager):
    '''
    Manager type used for spawning processes on a (presumably) foreign host
    '''
    def __init__(self, address, authkey):
        managers.SyncManager.__init__(self, address, authkey)
        self._name = 'Host-unknown'

    def Process(self, group=None, target=None, name=None, args=(), kwargs={}):
        if hasattr(sys.modules['__main__'], '__file__'):
            main_path = os.path.basename(sys.modules['__main__'].__file__)
        else:
            main_path = None
        data = pickle.dumps((target, args, kwargs))
        p = self._RemoteProcess(data, main_path)
        if name is None:
            temp = self._name.split('Host-')[-1] + '/Process-%s'
            name = temp % ':'.join(map(str, p.get_identity()))
        p.set_name(name)
        return p

    @classmethod
    def from_address(cls, address, authkey):
        manager = cls(address, authkey)
        managers.transact(address, authkey, 'dummy')
        manager._state.value = managers.State.STARTED
        manager._name = 'Host-%s:%s' % manager.address
        manager.shutdown = util.Finalize(
            manager, HostManager._finalize_host,
            args=(manager._address, manager._authkey, manager._name),
            exitpriority=-10
            )
        return manager

    @staticmethod
    def _finalize_host(address, authkey, name):
        managers.transact(address, authkey, 'shutdown')

    def __repr__(self):
        return '<Host(%s)>' % self._name

#
# Process subclass representing a process on (possibly) a remote machine
#

class RemoteProcess(Process):
    '''
    Represents a process started on a remote host
    '''
    def __init__(self, data, main_path):
        assert not main_path or os.path.basename(main_path) == main_path
        Process.__init__(self)
        self._data = data
        self._main_path = main_path

    def _bootstrap(self):
        forking.prepare({'main_path': self._main_path})
        self._target, self._args, self._kwargs = pickle.loads(self._data)
        return Process._bootstrap(self)

    def get_identity(self):
        return self._identity

HostManager.register('_RemoteProcess', RemoteProcess)

#
# A Pool class that uses a cluster
#

class DistributedPool(pool.Pool):

    def __init__(self, cluster, processes=None, initializer=None, initargs=()):
        self._cluster = cluster
        self.Process = cluster.Process
        pool.Pool.__init__(self, processes or len(cluster),
                           initializer, initargs)

    def _setup_queues(self):
        self._inqueue = self._cluster._SettableQueue()
        self._outqueue = self._cluster._SettableQueue()
        self._quick_put = self._inqueue.put
        self._quick_get = self._outqueue.get

    @staticmethod
    def _help_stuff_finish(inqueue, task_handler, size):
        inqueue.set_contents([None] * size)

#
# Manager type which starts host managers on other machines
#

def LocalProcess(**kwds):
    p = Process(**kwds)
    p.set_name('localhost/' + p.name)
    return p

class Cluster(managers.SyncManager):
    '''
    Represents collection of slots running on various hosts.

    `Cluster` is a subclass of `SyncManager` so it allows creation of
    various types of shared objects.
    '''
    def __init__(self, hostlist, modules):
        managers.SyncManager.__init__(self, address=('localhost', 0))
        self._hostlist = hostlist
        self._modules = modules
        if __name__ not in modules:
            modules.append(__name__)
        files = [sys.modules[name].__file__ for name in modules]
        for i, file in enumerate(files):
            if file.endswith('.pyc') or file.endswith('.pyo'):
                files[i] = file[:-4] + '.py'
        self._files = [os.path.abspath(file) for file in files]

    def start(self):
        managers.SyncManager.start(self)

        l = connection.Listener(family='AF_INET', authkey=self._authkey)

        for i, host in enumerate(self._hostlist):
            host._start_manager(i, self._authkey, l.address, self._files)

        for host in self._hostlist:
            if host.hostname != 'localhost':
                conn = l.accept()
                i, address, cpus = conn.recv()
                conn.close()
                other_host = self._hostlist[i]
                other_host.manager = HostManager.from_address(address,
                                                              self._authkey)
                other_host.slots = other_host.slots or cpus
                other_host.Process = other_host.manager.Process
            else:
                host.slots = host.slots or slot_count
                host.Process = LocalProcess

        self._slotlist = [
            Slot(host) for host in self._hostlist for i in range(host.slots)
            ]
        self._slot_iterator = itertools.cycle(self._slotlist)
        self._base_shutdown = self.shutdown
        del self.shutdown

    def shutdown(self):
        for host in self._hostlist:
            if host.hostname != 'localhost':
                host.manager.shutdown()
        self._base_shutdown()

    def Process(self, group=None, target=None, name=None, args=(), kwargs={}):
        slot = self._slot_iterator.next()
        return slot.Process(
            group=group, target=target, name=name, args=args, kwargs=kwargs
            )

    def Pool(self, processes=None, initializer=None, initargs=()):
        return DistributedPool(self, processes, initializer, initargs)

    def __getitem__(self, i):
        return self._slotlist[i]

    def __len__(self):
        return len(self._slotlist)

    def __iter__(self):
        return iter(self._slotlist)

#
# Queue subclass used by distributed pool
#

class SettableQueue(Queue.Queue):
    def empty(self):
        return not self.queue
    def full(self):
        return self.maxsize > 0 and len(self.queue) == self.maxsize
    def set_contents(self, contents):
        # length of contents must be at least as large as the number of
        # threads which have potentially called get()
        self.not_empty.acquire()
        try:
            self.queue.clear()
            self.queue.extend(contents)
            self.not_empty.notifyAll()
        finally:
            self.not_empty.release()

Cluster.register('_SettableQueue', SettableQueue)

#
# Class representing a notional cpu in the cluster
#

class Slot(object):
    def __init__(self, host):
        self.host = host
        self.Process = host.Process

#
# Host
#

class Host(object):
    '''
    Represents a host to use as a node in a cluster.

    `hostname` gives the name of the host.  If hostname is not
    "localhost" then ssh is used to log in to the host.  To log in as
    a different user use a host name of the form
    "username@somewhere.org"

    `slots` is used to specify the number of slots for processes on
    the host.  This affects how often processes will be allocated to
    this host.  Normally this should be equal to the number of cpus on
    that host.
    '''
    def __init__(self, hostname, slots=None):
        self.hostname = hostname
        self.slots = slots

    def _start_manager(self, index, authkey, address, files):
        if self.hostname != 'localhost':
            tempdir = copy_to_remote_temporary_directory(self.hostname, files)
            debug('startup files copied to %s:%s', self.hostname, tempdir)
            p = subprocess.Popen(
                ['ssh', self.hostname, 'python', '-c',
                 '"import os; os.chdir(%r); '
                 'from distributing import main; main()"' % tempdir],
                stdin=subprocess.PIPE
                )
            data = dict(
                name='BoostrappingHost', index=index,
                dist_log_level=_logger.getEffectiveLevel(),
                dir=tempdir, authkey=str(authkey), parent_address=address
                )
            pickle.dump(data, p.stdin, pickle.HIGHEST_PROTOCOL)
            p.stdin.close()

#
# Copy files to remote directory, returning name of directory
#

unzip_code = '''"
import tempfile, os, sys, tarfile
tempdir = tempfile.mkdtemp(prefix='distrib-')
os.chdir(tempdir)
tf = tarfile.open(fileobj=sys.stdin, mode='r|gz')
for ti in tf:
    tf.extract(ti)
print tempdir
"'''

def copy_to_remote_temporary_directory(host, files):
    p = subprocess.Popen(
        ['ssh', host, 'python', '-c', unzip_code],
        stdout=subprocess.PIPE, stdin=subprocess.PIPE
        )
    tf = tarfile.open(fileobj=p.stdin, mode='w|gz')
    for name in files:
        tf.add(name, os.path.basename(name))
    tf.close()
    p.stdin.close()
    return p.stdout.read().rstrip()

#
# Code which runs a host manager
#

def main():
    # get data from parent over stdin
    data = pickle.load(sys.stdin)
    sys.stdin.close()

    # set some stuff
    _logger.setLevel(data['dist_log_level'])
    forking.prepare(data)

    # create server for a `HostManager` object
    server = managers.Server(HostManager._registry, ('', 0), data['authkey'])
    current_process()._server = server

    # report server address and number of cpus back to parent
    conn = connection.Client(data['parent_address'], authkey=data['authkey'])
    conn.send((data['index'], server.address, slot_count))
    conn.close()

    # set name etc
    current_process().set_name('Host-%s:%s' % server.address)
    util._run_after_forkers()

    # register a cleanup function
    def cleanup(directory):
        debug('removing directory %s', directory)
        shutil.rmtree(directory)
        debug('shutting down host manager')
    util.Finalize(None, cleanup, args=[data['dir']], exitpriority=0)

    # start host manager
    debug('remote host manager starting in %s', data['dir'])
    server.serve_forever()
