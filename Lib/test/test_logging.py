#!/usr/bin/env python
"""
Test 0
======

Some preliminaries:
>>> import sys
>>> import logging
>>> def nextmessage():
...    global msgcount
...    rv = "Message %d" % msgcount
...    msgcount = msgcount + 1
...    return rv

Set a few variables, then go through the logger autoconfig and set the default threshold.
>>> msgcount = 0
>>> FINISH_UP = "Finish up, it's closing time. Messages should bear numbers 0 through 24."
>>> logging.basicConfig(stream=sys.stdout)
>>> rootLogger = logging.getLogger("")
>>> rootLogger.setLevel(logging.DEBUG)

Now, create a bunch of loggers, and set their thresholds.
>>> ERR = logging.getLogger("ERR0")
>>> ERR.setLevel(logging.ERROR)
>>> INF = logging.getLogger("INFO0")
>>> INF.setLevel(logging.INFO)
>>> INF_ERR  = logging.getLogger("INFO0.ERR")
>>> INF_ERR.setLevel(logging.ERROR)
>>> DEB = logging.getLogger("DEB0")
>>> DEB.setLevel(logging.DEBUG)
>>> INF_UNDEF = logging.getLogger("INFO0.UNDEF")
>>> INF_ERR_UNDEF = logging.getLogger("INFO0.ERR.UNDEF")
>>> UNDEF = logging.getLogger("UNDEF0")
>>> GRANDCHILD = logging.getLogger("INFO0.BADPARENT.UNDEF")
>>> CHILD = logging.getLogger("INFO0.BADPARENT")


And finally, run all the tests.

>>> ERR.log(logging.FATAL, nextmessage())
CRITICAL:ERR0:Message 0

>>> ERR.error(nextmessage())
ERROR:ERR0:Message 1

>>> INF.log(logging.FATAL, nextmessage())
CRITICAL:INFO0:Message 2

>>> INF.error(nextmessage())
ERROR:INFO0:Message 3

>>> INF.warn(nextmessage())
WARNING:INFO0:Message 4

>>> INF.info(nextmessage())
INFO:INFO0:Message 5

>>> INF_UNDEF.log(logging.FATAL, nextmessage())
CRITICAL:INFO0.UNDEF:Message 6

>>> INF_UNDEF.error(nextmessage())
ERROR:INFO0.UNDEF:Message 7

>>> INF_UNDEF.warn (nextmessage())
WARNING:INFO0.UNDEF:Message 8

>>> INF_UNDEF.info (nextmessage())
INFO:INFO0.UNDEF:Message 9

>>> INF_ERR.log(logging.FATAL, nextmessage())
CRITICAL:INFO0.ERR:Message 10

>>> INF_ERR.error(nextmessage())
ERROR:INFO0.ERR:Message 11

>>> INF_ERR_UNDEF.log(logging.FATAL, nextmessage())
CRITICAL:INFO0.ERR.UNDEF:Message 12

>>> INF_ERR_UNDEF.error(nextmessage())
ERROR:INFO0.ERR.UNDEF:Message 13

>>> DEB.log(logging.FATAL, nextmessage())
CRITICAL:DEB0:Message 14

>>> DEB.error(nextmessage())
ERROR:DEB0:Message 15

>>> DEB.warn (nextmessage())
WARNING:DEB0:Message 16

>>> DEB.info (nextmessage())
INFO:DEB0:Message 17

>>> DEB.debug(nextmessage())
DEBUG:DEB0:Message 18

>>> UNDEF.log(logging.FATAL, nextmessage())
CRITICAL:UNDEF0:Message 19

>>> UNDEF.error(nextmessage())
ERROR:UNDEF0:Message 20

>>> UNDEF.warn (nextmessage())
WARNING:UNDEF0:Message 21

>>> UNDEF.info (nextmessage())
INFO:UNDEF0:Message 22

>>> GRANDCHILD.log(logging.FATAL, nextmessage())
CRITICAL:INFO0.BADPARENT.UNDEF:Message 23

>>> CHILD.log(logging.FATAL, nextmessage())
CRITICAL:INFO0.BADPARENT:Message 24

These should not log:

>>> ERR.warn(nextmessage())

>>> ERR.info(nextmessage())

>>> ERR.debug(nextmessage())

>>> INF.debug(nextmessage())

>>> INF_UNDEF.debug(nextmessage())

>>> INF_ERR.warn(nextmessage())

>>> INF_ERR.info(nextmessage())

>>> INF_ERR.debug(nextmessage())

>>> INF_ERR_UNDEF.warn(nextmessage())

>>> INF_ERR_UNDEF.info(nextmessage())

>>> INF_ERR_UNDEF.debug(nextmessage())

>>> INF.info(FINISH_UP)
INFO:INFO0:Finish up, it's closing time. Messages should bear numbers 0 through 24.

Test 1
======

>>> import sys, logging
>>> logging.basicConfig(stream=sys.stdout)

First, we define our levels. There can be as many as you want - the only
limitations are that they should be integers, the lowest should be > 0 and
larger values mean less information being logged. If you need specific
level values which do not fit into these limitations, you can use a
mapping dictionary to convert between your application levels and the
logging system.

>>> SILENT      = 10
>>> TACITURN    = 9
>>> TERSE       = 8
>>> EFFUSIVE    = 7
>>> SOCIABLE    = 6
>>> VERBOSE     = 5
>>> TALKATIVE   = 4
>>> GARRULOUS   = 3
>>> CHATTERBOX  = 2
>>> BORING      = 1
>>> LEVEL_RANGE = range(BORING, SILENT + 1)


Next, we define names for our levels. You don't need to do this - in which
  case the system will use "Level n" to denote the text for the level.
'


>>> my_logging_levels = {
...     SILENT      : 'SILENT',
...     TACITURN    : 'TACITURN',
...     TERSE       : 'TERSE',
...     EFFUSIVE    : 'EFFUSIVE',
...     SOCIABLE    : 'SOCIABLE',
...     VERBOSE     : 'VERBOSE',
...     TALKATIVE   : 'TALKATIVE',
...     GARRULOUS   : 'GARRULOUS',
...     CHATTERBOX  : 'CHATTERBOX',
...     BORING      : 'BORING',
...     }


Now, to demonstrate filtering: suppose for some perverse reason we only
want to print out all except GARRULOUS messages. We create a filter for
this purpose...

>>> class SpecificLevelFilter(logging.Filter):
...     def __init__(self, lvl):
...         self.level = lvl
...
...     def filter(self, record):
...         return self.level != record.levelno

>>> class GarrulousFilter(SpecificLevelFilter):
...     def __init__(self):
...         SpecificLevelFilter.__init__(self, GARRULOUS)


Now, demonstrate filtering at the logger. This time, use a filter
which excludes SOCIABLE and TACITURN messages. Note that GARRULOUS events
are still excluded.


>>> class VerySpecificFilter(logging.Filter):
...     def filter(self, record):
...         return record.levelno not in [SOCIABLE, TACITURN]

>>> SHOULD1 = "This should only be seen at the '%s' logging level (or lower)"

Configure the logger, and tell the logging system to associate names with our levels.
>>> logging.basicConfig(stream=sys.stdout)
>>> rootLogger = logging.getLogger("")
>>> rootLogger.setLevel(logging.DEBUG)
>>> for lvl in my_logging_levels.keys():
...    logging.addLevelName(lvl, my_logging_levels[lvl])
>>> log = logging.getLogger("")
>>> hdlr = log.handlers[0]
>>> from test_logging import message

Set the logging level to each different value and call the utility
function to log events. In the output, you should see that each time
round the loop, the number of logging events which are actually output
decreases.

>>> log.setLevel(1)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)
BORING:root:This should only be seen at the '1' logging level (or lower)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)
CHATTERBOX:root:This should only be seen at the '2' logging level (or lower)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)
GARRULOUS:root:This should only be seen at the '3' logging level (or lower)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(2)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)
CHATTERBOX:root:This should only be seen at the '2' logging level (or lower)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)
GARRULOUS:root:This should only be seen at the '3' logging level (or lower)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(3)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)
GARRULOUS:root:This should only be seen at the '3' logging level (or lower)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(4)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(5)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)


>>> log.setLevel(6)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(7)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(8)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(9)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(10)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> hdlr.setLevel(SOCIABLE)

>>> log.setLevel(1)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(2)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(3)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(4)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(5)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(6)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(7)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(8)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(9)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(10)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>>

>>> hdlr.setLevel(0)

>>> garr = GarrulousFilter()

>>> hdlr.addFilter(garr)

>>> log.setLevel(1)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)
BORING:root:This should only be seen at the '1' logging level (or lower)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)
CHATTERBOX:root:This should only be seen at the '2' logging level (or lower)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(2)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)
CHATTERBOX:root:This should only be seen at the '2' logging level (or lower)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(3)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(4)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(5)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.setLevel(6)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)
SOCIABLE:root:This should only be seen at the '6' logging level (or lower)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(7)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(8)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(9)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)
TACITURN:root:This should only be seen at the '9' logging level (or lower)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(10)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> spec = VerySpecificFilter()

>>> log.addFilter(spec)

>>> log.setLevel(1)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)
BORING:root:This should only be seen at the '1' logging level (or lower)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)
CHATTERBOX:root:This should only be seen at the '2' logging level (or lower)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(2)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)
CHATTERBOX:root:This should only be seen at the '2' logging level (or lower)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(3)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(4)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)
TALKATIVE:root:This should only be seen at the '4' logging level (or lower)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(5)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)
VERBOSE:root:This should only be seen at the '5' logging level (or lower)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(6)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(7)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)
EFFUSIVE:root:This should only be seen at the '7' logging level (or lower)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(8)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)
TERSE:root:This should only be seen at the '8' logging level (or lower)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(9)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(10)

>>> log.log(1, "This should only be seen at the '%s' logging level (or lower)", BORING)

>>> log.log(2, "This should only be seen at the '%s' logging level (or lower)", CHATTERBOX)

>>> log.log(3, "This should only be seen at the '%s' logging level (or lower)", GARRULOUS)

>>> log.log(4, "This should only be seen at the '%s' logging level (or lower)", TALKATIVE)

>>> log.log(5, "This should only be seen at the '%s' logging level (or lower)", VERBOSE)

>>> log.log(6, "This should only be seen at the '%s' logging level (or lower)", SOCIABLE)

>>> log.log(7, "This should only be seen at the '%s' logging level (or lower)", EFFUSIVE)

>>> log.log(8, "This should only be seen at the '%s' logging level (or lower)", TERSE)

>>> log.log(9, "This should only be seen at the '%s' logging level (or lower)", TACITURN)

>>> log.log(10, "This should only be seen at the '%s' logging level (or lower)", SILENT)
SILENT:root:This should only be seen at the '10' logging level (or lower)

>>> log.setLevel(0)

>>> log.removeFilter(spec)

>>> hdlr.removeFilter(garr)

>>> logging.addLevelName(logging.DEBUG, "DEBUG")


Test 2
======
Test memory handlers. These are basically buffers for log messages: they take so many messages, and then print them all.

>>> import logging.handlers

>>> sys.stderr = sys.stdout
>>> logger = logging.getLogger("")
>>> sh = logger.handlers[0]
>>> sh.close()
>>> logger.removeHandler(sh)
>>> mh = logging.handlers.MemoryHandler(10,logging.WARNING, sh)
>>> logger.setLevel(logging.DEBUG)
>>> logger.addHandler(mh)

>>> logger.debug("Debug message")

-- logging at INFO, nothing should be seen yet --

>>> logger.info("Info message")

-- logging at WARNING, 3 messages should be seen --

>>> logger.warn("Warn message")
DEBUG:root:Debug message
INFO:root:Info message
WARNING:root:Warn message

>>> logger.info("Info index = 0")

>>> logger.info("Info index = 1")

>>> logger.info("Info index = 2")

>>> logger.info("Info index = 3")

>>> logger.info("Info index = 4")

>>> logger.info("Info index = 5")

>>> logger.info("Info index = 6")

>>> logger.info("Info index = 7")

>>> logger.info("Info index = 8")

>>> logger.info("Info index = 9")
INFO:root:Info index = 0
INFO:root:Info index = 1
INFO:root:Info index = 2
INFO:root:Info index = 3
INFO:root:Info index = 4
INFO:root:Info index = 5
INFO:root:Info index = 6
INFO:root:Info index = 7
INFO:root:Info index = 8
INFO:root:Info index = 9

>>> logger.info("Info index = 10")

>>> logger.info("Info index = 11")

>>> logger.info("Info index = 12")

>>> logger.info("Info index = 13")

>>> logger.info("Info index = 14")

>>> logger.info("Info index = 15")

>>> logger.info("Info index = 16")

>>> logger.info("Info index = 17")

>>> logger.info("Info index = 18")

>>> logger.info("Info index = 19")
INFO:root:Info index = 10
INFO:root:Info index = 11
INFO:root:Info index = 12
INFO:root:Info index = 13
INFO:root:Info index = 14
INFO:root:Info index = 15
INFO:root:Info index = 16
INFO:root:Info index = 17
INFO:root:Info index = 18
INFO:root:Info index = 19

>>> logger.info("Info index = 20")

>>> logger.info("Info index = 21")

>>> logger.info("Info index = 22")

>>> logger.info("Info index = 23")

>>> logger.info("Info index = 24")

>>> logger.info("Info index = 25")

>>> logger.info("Info index = 26")

>>> logger.info("Info index = 27")

>>> logger.info("Info index = 28")

>>> logger.info("Info index = 29")
INFO:root:Info index = 20
INFO:root:Info index = 21
INFO:root:Info index = 22
INFO:root:Info index = 23
INFO:root:Info index = 24
INFO:root:Info index = 25
INFO:root:Info index = 26
INFO:root:Info index = 27
INFO:root:Info index = 28
INFO:root:Info index = 29

>>> logger.info("Info index = 30")

>>> logger.info("Info index = 31")

>>> logger.info("Info index = 32")

>>> logger.info("Info index = 33")

>>> logger.info("Info index = 34")

>>> logger.info("Info index = 35")

>>> logger.info("Info index = 36")

>>> logger.info("Info index = 37")

>>> logger.info("Info index = 38")

>>> logger.info("Info index = 39")
INFO:root:Info index = 30
INFO:root:Info index = 31
INFO:root:Info index = 32
INFO:root:Info index = 33
INFO:root:Info index = 34
INFO:root:Info index = 35
INFO:root:Info index = 36
INFO:root:Info index = 37
INFO:root:Info index = 38
INFO:root:Info index = 39

>>> logger.info("Info index = 40")

>>> logger.info("Info index = 41")

>>> logger.info("Info index = 42")

>>> logger.info("Info index = 43")

>>> logger.info("Info index = 44")

>>> logger.info("Info index = 45")

>>> logger.info("Info index = 46")

>>> logger.info("Info index = 47")

>>> logger.info("Info index = 48")

>>> logger.info("Info index = 49")
INFO:root:Info index = 40
INFO:root:Info index = 41
INFO:root:Info index = 42
INFO:root:Info index = 43
INFO:root:Info index = 44
INFO:root:Info index = 45
INFO:root:Info index = 46
INFO:root:Info index = 47
INFO:root:Info index = 48
INFO:root:Info index = 49

>>> logger.info("Info index = 50")

>>> logger.info("Info index = 51")

>>> logger.info("Info index = 52")

>>> logger.info("Info index = 53")

>>> logger.info("Info index = 54")

>>> logger.info("Info index = 55")

>>> logger.info("Info index = 56")

>>> logger.info("Info index = 57")

>>> logger.info("Info index = 58")

>>> logger.info("Info index = 59")
INFO:root:Info index = 50
INFO:root:Info index = 51
INFO:root:Info index = 52
INFO:root:Info index = 53
INFO:root:Info index = 54
INFO:root:Info index = 55
INFO:root:Info index = 56
INFO:root:Info index = 57
INFO:root:Info index = 58
INFO:root:Info index = 59

>>> logger.info("Info index = 60")

>>> logger.info("Info index = 61")

>>> logger.info("Info index = 62")

>>> logger.info("Info index = 63")

>>> logger.info("Info index = 64")

>>> logger.info("Info index = 65")

>>> logger.info("Info index = 66")

>>> logger.info("Info index = 67")

>>> logger.info("Info index = 68")

>>> logger.info("Info index = 69")
INFO:root:Info index = 60
INFO:root:Info index = 61
INFO:root:Info index = 62
INFO:root:Info index = 63
INFO:root:Info index = 64
INFO:root:Info index = 65
INFO:root:Info index = 66
INFO:root:Info index = 67
INFO:root:Info index = 68
INFO:root:Info index = 69

>>> logger.info("Info index = 70")

>>> logger.info("Info index = 71")

>>> logger.info("Info index = 72")

>>> logger.info("Info index = 73")

>>> logger.info("Info index = 74")

>>> logger.info("Info index = 75")

>>> logger.info("Info index = 76")

>>> logger.info("Info index = 77")

>>> logger.info("Info index = 78")

>>> logger.info("Info index = 79")
INFO:root:Info index = 70
INFO:root:Info index = 71
INFO:root:Info index = 72
INFO:root:Info index = 73
INFO:root:Info index = 74
INFO:root:Info index = 75
INFO:root:Info index = 76
INFO:root:Info index = 77
INFO:root:Info index = 78
INFO:root:Info index = 79

>>> logger.info("Info index = 80")

>>> logger.info("Info index = 81")

>>> logger.info("Info index = 82")

>>> logger.info("Info index = 83")

>>> logger.info("Info index = 84")

>>> logger.info("Info index = 85")

>>> logger.info("Info index = 86")

>>> logger.info("Info index = 87")

>>> logger.info("Info index = 88")

>>> logger.info("Info index = 89")
INFO:root:Info index = 80
INFO:root:Info index = 81
INFO:root:Info index = 82
INFO:root:Info index = 83
INFO:root:Info index = 84
INFO:root:Info index = 85
INFO:root:Info index = 86
INFO:root:Info index = 87
INFO:root:Info index = 88
INFO:root:Info index = 89

>>> logger.info("Info index = 90")

>>> logger.info("Info index = 91")

>>> logger.info("Info index = 92")

>>> logger.info("Info index = 93")

>>> logger.info("Info index = 94")

>>> logger.info("Info index = 95")

>>> logger.info("Info index = 96")

>>> logger.info("Info index = 97")

>>> logger.info("Info index = 98")

>>> logger.info("Info index = 99")
INFO:root:Info index = 90
INFO:root:Info index = 91
INFO:root:Info index = 92
INFO:root:Info index = 93
INFO:root:Info index = 94
INFO:root:Info index = 95
INFO:root:Info index = 96
INFO:root:Info index = 97
INFO:root:Info index = 98
INFO:root:Info index = 99

>>> logger.info("Info index = 100")

>>> logger.info("Info index = 101")

>>> mh.close()
INFO:root:Info index = 100
INFO:root:Info index = 101

>>> logger.removeHandler(mh)
>>> logger.addHandler(sh)



Test 3
======

>>> import sys, logging
>>> sys.stderr = sys
>>> logging.basicConfig()
>>> FILTER = "a.b"
>>> root = logging.getLogger()
>>> root.setLevel(logging.DEBUG)
>>> hand = root.handlers[0]

>>> logging.getLogger("a").info("Info 1")
INFO:a:Info 1

>>> logging.getLogger("a.b").info("Info 2")
INFO:a.b:Info 2

>>> logging.getLogger("a.c").info("Info 3")
INFO:a.c:Info 3

>>> logging.getLogger("a.b.c").info("Info 4")
INFO:a.b.c:Info 4

>>> logging.getLogger("a.b.c.d").info("Info 5")
INFO:a.b.c.d:Info 5

>>> logging.getLogger("a.bb.c").info("Info 6")
INFO:a.bb.c:Info 6

>>> logging.getLogger("b").info("Info 7")
INFO:b:Info 7

>>> logging.getLogger("b.a").info("Info 8")
INFO:b.a:Info 8

>>> logging.getLogger("c.a.b").info("Info 9")
INFO:c.a.b:Info 9

>>> logging.getLogger("a.bb").info("Info 10")
INFO:a.bb:Info 10

Filtered with 'a.b'...

>>> filt = logging.Filter(FILTER)

>>> hand.addFilter(filt)

>>> logging.getLogger("a").info("Info 1")

>>> logging.getLogger("a.b").info("Info 2")
INFO:a.b:Info 2

>>> logging.getLogger("a.c").info("Info 3")

>>> logging.getLogger("a.b.c").info("Info 4")
INFO:a.b.c:Info 4

>>> logging.getLogger("a.b.c.d").info("Info 5")
INFO:a.b.c.d:Info 5

>>> logging.getLogger("a.bb.c").info("Info 6")

>>> logging.getLogger("b").info("Info 7")

>>> logging.getLogger("b.a").info("Info 8")

>>> logging.getLogger("c.a.b").info("Info 9")

>>> logging.getLogger("a.bb").info("Info 10")

>>> hand.removeFilter(filt)


Test 4
======
>>> import sys, logging, logging.handlers, string
>>> import tempfile, logging.config, os, test.test_support
>>> sys.stderr = sys.stdout

>>> from test_logging import config0, config1

config2 has a subtle configuration error that should be reported
>>> config2 = string.replace(config1, "sys.stdout", "sys.stbout")

config3 has a less subtle configuration error
>>> config3 = string.replace(config1, "formatter=form1", "formatter=misspelled_name")

>>> def test4(conf):
...     loggerDict = logging.getLogger().manager.loggerDict
...     logging._acquireLock()
...     try:
...         saved_handlers = logging._handlers.copy()
...         saved_handler_list = logging._handlerList[:]
...         saved_loggers = loggerDict.copy()
...     finally:
...         logging._releaseLock()
...     try:
...         fn = test.test_support.TESTFN
...         f = open(fn, "w")
...         f.write(conf)
...         f.close()
...         try:
...             logging.config.fileConfig(fn)
...             #call again to make sure cleanup is correct
...             logging.config.fileConfig(fn)
...         except:
...             t = sys.exc_info()[0]
...             message(str(t))
...         else:
...             message('ok.')
...             os.remove(fn)
...     finally:
...         logging._acquireLock()
...         try:
...             logging._handlers.clear()
...             logging._handlers.update(saved_handlers)
...             logging._handlerList[:] = saved_handler_list
...             loggerDict = logging.getLogger().manager.loggerDict
...             loggerDict.clear()
...             loggerDict.update(saved_loggers)
...         finally:
...             logging._releaseLock()

>>> test4(config0)
ok.

>>> test4(config1)
ok.

>>> test4(config2)
<type 'exceptions.AttributeError'>

>>> test4(config3)
<type 'exceptions.KeyError'>

>>> import test_logging
>>> test_logging.test5()
ERROR:root:just testing
<type 'exceptions.KeyError'>... Don't panic!


Test Main
=========
>>> import select
>>> import os, sys, string, struct, types, cPickle, cStringIO
>>> import socket, tempfile, threading, time
>>> import logging, logging.handlers, logging.config
>>> import test_logging

>>> test_logging.test_main_inner()
ERR -> CRITICAL: Message 0 (via logrecv.tcp.ERR)
ERR -> ERROR: Message 1 (via logrecv.tcp.ERR)
INF -> CRITICAL: Message 2 (via logrecv.tcp.INF)
INF -> ERROR: Message 3 (via logrecv.tcp.INF)
INF -> WARNING: Message 4 (via logrecv.tcp.INF)
INF -> INFO: Message 5 (via logrecv.tcp.INF)
INF.UNDEF -> CRITICAL: Message 6 (via logrecv.tcp.INF.UNDEF)
INF.UNDEF -> ERROR: Message 7 (via logrecv.tcp.INF.UNDEF)
INF.UNDEF -> WARNING: Message 8 (via logrecv.tcp.INF.UNDEF)
INF.UNDEF -> INFO: Message 9 (via logrecv.tcp.INF.UNDEF)
INF.ERR -> CRITICAL: Message 10 (via logrecv.tcp.INF.ERR)
INF.ERR -> ERROR: Message 11 (via logrecv.tcp.INF.ERR)
INF.ERR.UNDEF -> CRITICAL: Message 12 (via logrecv.tcp.INF.ERR.UNDEF)
INF.ERR.UNDEF -> ERROR: Message 13 (via logrecv.tcp.INF.ERR.UNDEF)
DEB -> CRITICAL: Message 14 (via logrecv.tcp.DEB)
DEB -> ERROR: Message 15 (via logrecv.tcp.DEB)
DEB -> WARNING: Message 16 (via logrecv.tcp.DEB)
DEB -> INFO: Message 17 (via logrecv.tcp.DEB)
DEB -> DEBUG: Message 18 (via logrecv.tcp.DEB)
UNDEF -> CRITICAL: Message 19 (via logrecv.tcp.UNDEF)
UNDEF -> ERROR: Message 20 (via logrecv.tcp.UNDEF)
UNDEF -> WARNING: Message 21 (via logrecv.tcp.UNDEF)
UNDEF -> INFO: Message 22 (via logrecv.tcp.UNDEF)
INF.BADPARENT.UNDEF -> CRITICAL: Message 23 (via logrecv.tcp.INF.BADPARENT.UNDEF)
INF.BADPARENT -> CRITICAL: Message 24 (via logrecv.tcp.INF.BADPARENT)
INF -> INFO: Finish up, it's closing time. Messages should bear numbers 0 through 24. (via logrecv.tcp.INF)
<BLANKLINE>
"""
import select
import os, sys, string, struct, cPickle, cStringIO
import socket, threading
import logging, logging.handlers, logging.config, test.test_support


BANNER = "-- %-10s %-6s ---------------------------------------------------\n"

FINISH_UP = "Finish up, it's closing time. Messages should bear numbers 0 through 24."
#----------------------------------------------------------------------------
# Test 0
#----------------------------------------------------------------------------

msgcount = 0

def nextmessage():
    global msgcount
    rv = "Message %d" % msgcount
    msgcount = msgcount + 1
    return rv

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
        logger = logging.getLogger("logrecv")
        logger.handle(record)

# The server sets socketDataProcessed when it's done.
socketDataProcessed = threading.Event()
#----------------------------------------------------------------------------
# Test 5
#----------------------------------------------------------------------------

test5_config = """
[loggers]
keys=root

[handlers]
keys=hand1

[formatters]
keys=form1

[logger_root]
level=NOTSET
handlers=hand1

[handler_hand1]
class=StreamHandler
level=NOTSET
formatter=form1
args=(sys.stdout,)

[formatter_form1]
class=test.test_logging.FriendlyFormatter
format=%(levelname)s:%(name)s:%(message)s
datefmt=
"""

class FriendlyFormatter (logging.Formatter):
    def formatException(self, ei):
        return "%s... Don't panic!" % str(ei[0])


def test5():
    loggerDict = logging.getLogger().manager.loggerDict
    logging._acquireLock()
    try:
        saved_handlers = logging._handlers.copy()
        saved_handler_list = logging._handlerList[:]
        saved_loggers = loggerDict.copy()
    finally:
        logging._releaseLock()
    try:
        fn = test.test_support.TESTFN
        f = open(fn, "w")
        f.write(test5_config)
        f.close()
        logging.config.fileConfig(fn)
        try:
            raise KeyError
        except KeyError:
            logging.exception("just testing")
        os.remove(fn)
        hdlr = logging.getLogger().handlers[0]
        logging.getLogger().handlers.remove(hdlr)
    finally:
        logging._acquireLock()
        try:
            logging._handlers.clear()
            logging._handlers.update(saved_handlers)
            logging._handlerList[:] = saved_handler_list
            loggerDict = logging.getLogger().manager.loggerDict
            loggerDict.clear()
            loggerDict.update(saved_loggers)
        finally:
            logging._releaseLock()


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
        self.abort = False
        self.timeout = 1

    def serve_until_stopped(self):
        while not self.abort:
            rd, wr, ex = select.select([self.socket.fileno()], [], [],
                                       self.timeout)
            if rd:
                self.handle_request()
            socketDataProcessed.set()
        # close the listen socket
        self.server_close()

    def process_request(self, request, client_address):
        t = threading.Thread(target = self.finish_request,
                             args = (request, client_address))
        t.start()

def runTCP(tcpserver):
    tcpserver.serve_until_stopped()

def banner(nm, typ):
    sep = BANNER % (nm, typ)
    sys.stdout.write(sep)
    sys.stdout.flush()

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

def test_main_inner():
    rootLogger = logging.getLogger("")
    rootLogger.setLevel(logging.DEBUG)

    tcpserver = LogRecordSocketReceiver(port=0)
    port = tcpserver.socket.getsockname()[1]

    # Set up a handler such that all events are sent via a socket to the log
    # receiver (logrecv).
    # The handler will only be added to the rootLogger for some of the tests
    shdlr = logging.handlers.SocketHandler('localhost', port)
    rootLogger.addHandler(shdlr)

    # Configure the logger for logrecv so events do not propagate beyond it.
    # The sockLogger output is buffered in memory until the end of the test,
    # and printed at the end.
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
    #sys.stdout.write("About to start TCP server...\n")
    threads.append(threading.Thread(target=runTCP, args=(tcpserver,)))

    for thread in threads:
        thread.start()
    try:
        test0()

        # XXX(nnorwitz): Try to fix timing related test failures.
        # This sleep gives us some extra time to read messages.
        # The test generally only fails on Solaris without this sleep.
        #time.sleep(2.0)
        shdlr.close()
        rootLogger.removeHandler(shdlr)

    finally:
        #wait for TCP receiver to terminate
        socketDataProcessed.wait()
        # ensure the server dies
        tcpserver.abort = True
        for thread in threads:
            thread.join(2.0)
        print(sockOut.getvalue())
        sockOut.close()
        sockLogger.removeHandler(sockhdlr)
        sockhdlr.close()
        sys.stdout.flush()

# config0 is a standard configuration.
config0 = """
[loggers]
keys=root

[handlers]
keys=hand1

[formatters]
keys=form1

[logger_root]
level=NOTSET
handlers=hand1

[handler_hand1]
class=StreamHandler
level=NOTSET
formatter=form1
args=(sys.stdout,)

[formatter_form1]
format=%(levelname)s:%(name)s:%(message)s
datefmt=
"""

# config1 adds a little to the standard configuration.
config1 = """
[loggers]
keys=root,parser

[handlers]
keys=hand1

[formatters]
keys=form1

[logger_root]
level=NOTSET
handlers=hand1

[logger_parser]
level=DEBUG
handlers=hand1
propagate=1
qualname=compiler.parser

[handler_hand1]
class=StreamHandler
level=NOTSET
formatter=form1
args=(sys.stdout,)

[formatter_form1]
format=%(levelname)s:%(name)s:%(message)s
datefmt=
"""

def message(s):
    sys.stdout.write("%s\n" % s)

# config2 has a subtle configuration error that should be reported
config2 = string.replace(config1, "sys.stdout", "sys.stbout")

# config3 has a less subtle configuration error
config3 = string.replace(
    config1, "formatter=form1", "formatter=misspelled_name")

def test_main():
    from test import test_support, test_logging
    test_support.run_doctest(test_logging)

if __name__=="__main__":
    test_main()
