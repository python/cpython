"""terminalcommand.py -- A minimal interface to Terminal.app.

To run a shell command in a new Terminal.app window:

    import terminalcommand
    terminalcommand.run("ls -l")

No result is returned; it is purely meant as a quick way to run a script
with a decent input/output window.
"""

#
# This module is a fairly straightforward translation of Jack Jansen's
# Mac/OSX/PythonLauncher/doscript.m.
#

import time
import os
from Carbon import AE
from Carbon.AppleEvents import *


TERMINAL_SIG = "trmx"
START_TERMINAL = "/usr/bin/open /Applications/Utilities/Terminal.app"
SEND_MODE = kAENoReply  # kAEWaitReply hangs when run from Terminal.app itself


def run(command):
    """Run a shell command in a new Terminal.app window."""
    termAddress = AE.AECreateDesc(typeApplSignature, TERMINAL_SIG)
    theEvent = AE.AECreateAppleEvent(kAECoreSuite, kAEDoScript, termAddress,
                                     kAutoGenerateReturnID, kAnyTransactionID)
    commandDesc = AE.AECreateDesc(typeChar, command)
    theEvent.AEPutParamDesc(kAECommandClass, commandDesc)

    try:
        theEvent.AESend(SEND_MODE, kAENormalPriority, kAEDefaultTimeout)
    except AE.Error, why:
        if why[0] != -600:  # Terminal.app not yet running
            raise
        os.system(START_TERMINAL)
        time.sleep(1)
        theEvent.AESend(SEND_MODE, kAENormalPriority, kAEDefaultTimeout)


if __name__ == "__main__":
    run("ls -l")
