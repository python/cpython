#!/usr/bin/env python
#
# Copyright 2001-2002 by Vinay Sajip. All Rights Reserved.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appear in all copies and that
# both that copyright notice and this permission notice appear in
# supporting documentation, and that the name of Vinay Sajip
# not be used in advertising or publicity pertaining to distribution
# of the software without specific, written prior permission.
# VINAY SAJIP DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
# ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
# VINAY SAJIP BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
# ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
# IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
# OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# This file is part of the Python logging distribution. See
# http://www.red-dove.com/python_logging.html
#
"""Test harness for the logging module. Run all tests.

Copyright (C) 2001-2002 Vinay Sajip. All Rights Reserved.
"""

import select
import os, sys, string, struct, types, cPickle, cStringIO
import socket, threading, time
import logging, logging.handlers, logging.config

BANNER = "-- %-10s %-6s ---------------------------------------------------\n"

FINISH_UP = "Finish up, it's closing time. Messages should bear numbers 0 through 24."

#----------------------------------------------------------------------------
# Log receiver
#----------------------------------------------------------------------------

TIMEOUT         = 10

from SocketServer import ThreadingTCPServer, StreamRequestHandler

class LogRecordStreamHandler(StreamRequestHandler):
    """
    Handler for a streaming logging request. It basically logs the record
    using whatever logging policy is configured locally.
    """

    def handle(self):
        """
        Handle multiple requests - each expected to be a 4-byte length,
        followed by the LogRecord in pickle format. Logs the record
        according to whatever policy is configured locally.
        """
        while 1:
            try:
                chunk = self.connection.recv(4)
                if len(chunk) < 4:
                    break
                slen = struct.unpack(">L", chunk)[0]
                chunk = self.connection.recv(slen)
                while len(chunk) < slen:
                    chunk = chunk + self.connection.recv(slen - len(chunk))
                obj = self.unPickle(chunk)
                record = logging.makeLogRecord(obj)
                self.handleLogRecord(record)
            except:
                raise

    def unPickle(self, data):
        return cPickle.loads(data)

    def handleLogRecord(self, record):
        logname = "logrecv.tcp." + record.name
        #If the end-of-messages sentinel is seen, tell the server to terminate
        if record.msg == FINISH_UP:
            self.server.abort = 1
        record.msg = record.msg + " (via " + logname + ")"
        logger = logging.getLogger(logname)
        logger.handle(record)

# The server sets socketDataProcessed when it's done.
socketDataProcessed = threading.Event()

class LogRecordSocketReceiver(ThreadingTCPServer):
    """
    A simple-minded TCP socket-based logging receiver suitable for test
    purposes.
    """

    allow_reuse_address = 1

    def __init__(self, host='localhost',
                             port=logging.handlers.DEFAULT_TCP_LOGGING_PORT,
                     handler=LogRecordStreamHandler):
        ThreadingTCPServer.__init__(self, (host, port), handler)
        self.abort = 0
        self.timeout = 1

    def serve_until_stopped(self):
        abort = 0
        while not abort:
            rd, wr, ex = select.select([self.socket.fileno()],
                                       [], [],
                                       self.timeout)
            if rd:
                self.handle_request()
            abort = self.abort
        #notify the main thread that we're about to exit
        socketDataProcessed.set()

    def process_request(self, request, client_address):
        #import threading
        t = threading.Thread(target = self.finish_request,
                             args = (request, client_address))
        t.start()

def runTCP(tcpserver):
    tcpserver.serve_until_stopped()

#----------------------------------------------------------------------------
# Test 0
#----------------------------------------------------------------------------

msgcount = 0

def nextmessage():
    global msgcount
    rv = "Message %d" % msgcount
    msgcount = msgcount + 1
    return rv

def test0():
    ERR = logging.getLogger("ERR")
    ERR.setLevel(logging.ERROR)
    INF = logging.getLogger("INF")
    INF.setLevel(logging.INFO)
    INF_ERR  = logging.getLogger("INF.ERR")
    INF_ERR.setLevel(logging.ERROR)
    DEB = logging.getLogger("DEB")
    DEB.setLevel(logging.DEBUG)

    INF_UNDEF = logging.getLogger("INF.UNDEF")
    INF_ERR_UNDEF = logging.getLogger("INF.ERR.UNDEF")
    UNDEF = logging.getLogger("UNDEF")

    GRANDCHILD = logging.getLogger("INF.BADPARENT.UNDEF")
    CHILD = logging.getLogger("INF.BADPARENT")

    #These should log
    ERR.log(logging.FATAL, nextmessage())
    ERR.error(nextmessage())

    INF.log(logging.FATAL, nextmessage())
    INF.error(nextmessage())
    INF.warn(nextmessage())
    INF.info(nextmessage())

    INF_UNDEF.log(logging.FATAL, nextmessage())
    INF_UNDEF.error(nextmessage())
    INF_UNDEF.warn (nextmessage())
    INF_UNDEF.info (nextmessage())

    INF_ERR.log(logging.FATAL, nextmessage())
    INF_ERR.error(nextmessage())

    INF_ERR_UNDEF.log(logging.FATAL, nextmessage())
    INF_ERR_UNDEF.error(nextmessage())

    DEB.log(logging.FATAL, nextmessage())
    DEB.error(nextmessage())
    DEB.warn (nextmessage())
    DEB.info (nextmessage())
    DEB.debug(nextmessage())

    UNDEF.log(logging.FATAL, nextmessage())
    UNDEF.error(nextmessage())
    UNDEF.warn (nextmessage())
    UNDEF.info (nextmessage())

    GRANDCHILD.log(logging.FATAL, nextmessage())
    CHILD.log(logging.FATAL, nextmessage())

    #These should not log
    ERR.warn(nextmessage())
    ERR.info(nextmessage())
    ERR.debug(nextmessage())

    INF.debug(nextmessage())
    INF_UNDEF.debug(nextmessage())

    INF_ERR.warn(nextmessage())
    INF_ERR.info(nextmessage())
    INF_ERR.debug(nextmessage())
    INF_ERR_UNDEF.warn(nextmessage())
    INF_ERR_UNDEF.info(nextmessage())
    INF_ERR_UNDEF.debug(nextmessage())

    INF.info(FINISH_UP)

#----------------------------------------------------------------------------
# Test 1
#----------------------------------------------------------------------------

#
#   First, we define our levels. There can be as many as you want - the only
#     limitations are that they should be integers, the lowest should be > 0 and
#   larger values mean less information being logged. If you need specific
#   level values which do not fit into these limitations, you can use a
#   mapping dictionary to convert between your application levels and the
#   logging system.
#
SILENT      = 10
TACITURN    = 9
TERSE       = 8
EFFUSIVE    = 7
SOCIABLE    = 6
VERBOSE     = 5
TALKATIVE   = 4
GARRULOUS   = 3
CHATTERBOX  = 2
BORING      = 1

LEVEL_RANGE = range(BORING, SILENT + 1)

#
#   Next, we define names for our levels. You don't need to do this - in which
#   case the system will use "Level n" to denote the text for the level.
#
my_logging_levels = {
    SILENT      : 'Silent',
    TACITURN    : 'Taciturn',
    TERSE       : 'Terse',
    EFFUSIVE    : 'Effusive',
    SOCIABLE    : 'Sociable',
    VERBOSE     : 'Verbose',
    TALKATIVE   : 'Talkative',
    GARRULOUS   : 'Garrulous',
    CHATTERBOX  : 'Chatterbox',
    BORING      : 'Boring',
}

#
#   Now, to demonstrate filtering: suppose for some perverse reason we only
#   want to print out all except GARRULOUS messages. Let's create a filter for
#   this purpose...
#
class SpecificLevelFilter(logging.Filter):
    def __init__(self, lvl):
        self.level = lvl

    def filter(self, record):
        return self.level != record.levelno

class GarrulousFilter(SpecificLevelFilter):
    def __init__(self):
        SpecificLevelFilter.__init__(self, GARRULOUS)

#
#   Now, let's demonstrate filtering at the logger. This time, use a filter
#   which excludes SOCIABLE and TACITURN messages. Note that GARRULOUS events
#   are still excluded.
#
class VerySpecificFilter(logging.Filter):
    def filter(self, record):
        return record.levelno not in [SOCIABLE, TACITURN]

def message(s):
    sys.stdout.write("%s\n" % s)

SHOULD1 = "This should only be seen at the '%s' logging level (or lower)"

def test1():
#
#   Now, tell the logging system to associate names with our levels.
#
    for lvl in my_logging_levels.keys():
        logging.addLevelName(lvl, my_logging_levels[lvl])

#
#   Now, define a test function which logs an event at each of our levels.
#

    def doLog(log):
        for lvl in LEVEL_RANGE:
            log.log(lvl, SHOULD1, logging.getLevelName(lvl))

    log = logging.getLogger("")
    hdlr = log.handlers[0]
#
#   Set the logging level to each different value and call the utility
#   function to log events.
#   In the output, you should see that each time round the loop, the number of
#   logging events which are actually output decreases.
#
    for lvl in LEVEL_RANGE:
        message("-- setting logging level to '%s' -----" %
                        logging.getLevelName(lvl))
        log.setLevel(lvl)
        doLog(log)
  #
  #   Now, we demonstrate level filtering at the handler level. Tell the
  #   handler defined above to filter at level 'SOCIABLE', and repeat the
  #   above loop. Compare the output from the two runs.
  #
    hdlr.setLevel(SOCIABLE)
    message("-- Filtering at handler level to SOCIABLE --")
    for lvl in LEVEL_RANGE:
        message("-- setting logging level to '%s' -----" %
                      logging.getLevelName(lvl))
        log.setLevel(lvl)
        doLog(log)

    hdlr.setLevel(0)    #turn off level filtering at the handler

    garr = GarrulousFilter()
    hdlr.addFilter(garr)
    message("-- Filtering using GARRULOUS filter --")
    for lvl in LEVEL_RANGE:
        message("-- setting logging level to '%s' -----" %
                        logging.getLevelName(lvl))
        log.setLevel(lvl)
        doLog(log)
    spec = VerySpecificFilter()
    log.addFilter(spec)
    message("-- Filtering using specific filter for SOCIABLE, TACITURN --")
    for lvl in LEVEL_RANGE:
        message("-- setting logging level to '%s' -----" %
                      logging.getLevelName(lvl))
        log.setLevel(lvl)
        doLog(log)

    log.removeFilter(spec)
    hdlr.removeFilter(garr)
    #Undo the one level which clashes...for regression tests
    logging.addLevelName(logging.DEBUG, "DEBUG")

#----------------------------------------------------------------------------
# Test 2
#----------------------------------------------------------------------------

MSG = "-- logging %d at INFO, messages should be seen every 10 events --"
def test2():
    logger = logging.getLogger("")
    sh = logger.handlers[0]
    sh.close()
    logger.removeHandler(sh)
    mh = logging.handlers.MemoryHandler(10,logging.WARNING, sh)
    logger.setLevel(logging.DEBUG)
    logger.addHandler(mh)
    message("-- logging at DEBUG, nothing should be seen yet --")
    logger.debug("Debug message")
    message("-- logging at INFO, nothing should be seen yet --")
    logger.info("Info message")
    message("-- logging at WARNING, 3 messages should be seen --")
    logger.warn("Warn message")
    for i in xrange(102):
        message(MSG % i)
        logger.info("Info index = %d", i)
    mh.close()
    logger.removeHandler(mh)
    logger.addHandler(sh)

#----------------------------------------------------------------------------
# Test 3
#----------------------------------------------------------------------------

FILTER = "a.b"

def doLog3():
    logging.getLogger("a").info("Info 1")
    logging.getLogger("a.b").info("Info 2")
    logging.getLogger("a.c").info("Info 3")
    logging.getLogger("a.b.c").info("Info 4")
    logging.getLogger("a.b.c.d").info("Info 5")
    logging.getLogger("a.bb.c").info("Info 6")
    logging.getLogger("b").info("Info 7")
    logging.getLogger("b.a").info("Info 8")
    logging.getLogger("c.a.b").info("Info 9")
    logging.getLogger("a.bb").info("Info 10")

def test3():
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)
    hand = root.handlers[0]
    message("Unfiltered...")
    doLog3()
    message("Filtered with '%s'..." % FILTER)
    filt = logging.Filter(FILTER)
    hand.addFilter(filt)
    doLog3()
    hand.removeFilter(filt)

#----------------------------------------------------------------------------
# Test Harness
#----------------------------------------------------------------------------
def banner(nm, typ):
    sep = BANNER % (nm, typ)
    sys.stdout.write(sep)
    sys.stdout.flush()

def test_main_inner():
    rootLogger = logging.getLogger("")
    rootLogger.setLevel(logging.DEBUG)
    hdlr = logging.StreamHandler(sys.stdout)
    fmt = logging.Formatter(logging.BASIC_FORMAT)
    hdlr.setFormatter(fmt)
    rootLogger.addHandler(hdlr)

    #Set up a handler such that all events are sent via a socket to the log
    #receiver (logrecv).
    #The handler will only be added to the rootLogger for some of the tests
    hdlr = logging.handlers.SocketHandler('localhost',
                                   logging.handlers.DEFAULT_TCP_LOGGING_PORT)

    #Configure the logger for logrecv so events do not propagate beyond it.
    #The sockLogger output is buffered in memory until the end of the test,
    #and printed at the end.
    sockOut = cStringIO.StringIO()
    sockLogger = logging.getLogger("logrecv")
    sockLogger.setLevel(logging.DEBUG)
    sockhdlr = logging.StreamHandler(sockOut)
    sockhdlr.setFormatter(logging.Formatter(
                                   "%(name)s -> %(levelname)s: %(message)s"))
    sockLogger.addHandler(sockhdlr)
    sockLogger.propagate = 0

    #Set up servers
    threads = []
    tcpserver = LogRecordSocketReceiver()
    #sys.stdout.write("About to start TCP server...\n")
    threads.append(threading.Thread(target=runTCP, args=(tcpserver,)))

    for thread in threads:
        thread.start()
    try:
        banner("log_test0", "begin")

        rootLogger.addHandler(hdlr)
        test0()
        hdlr.close()
        rootLogger.removeHandler(hdlr)

        banner("log_test0", "end")

        banner("log_test1", "begin")
        test1()
        banner("log_test1", "end")

        banner("log_test2", "begin")
        test2()
        banner("log_test2", "end")

        banner("log_test3", "begin")
        test3()
        banner("log_test3", "end")

    finally:
        #wait for TCP receiver to terminate
        socketDataProcessed.wait()
        for thread in threads:
            thread.join()
        banner("logrecv output", "begin")
        sys.stdout.write(sockOut.getvalue())
        sockOut.close()
        banner("logrecv output", "end")
        sys.stdout.flush()

def test_main():
    import locale
    # Set the locale to the platform-dependent default.  I have no idea
    # why the test does this, but in any case we save the current locale
    # first so we can restore it at the end.
    try:
        original_locale = locale.setlocale(locale.LC_ALL)
        locale.setlocale(locale.LC_ALL, '')
    except (ValueError, locale.Error):
        # this happens on a Solaris box which only supports "C" locale
        # or a Mac OS X box which supports very little locale stuff at all
        original_locale = None

    try:
        test_main_inner()
    finally:
        if original_locale is not None:
            locale.setlocale(locale.LC_ALL, original_locale)

if __name__ == "__main__":
    sys.stdout.write("test_logging\n")
    test_main()
