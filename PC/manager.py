# This is embedded into the py.exe launcher to provide management routines.
# The public API is entirely functions. Everything else is internal.
#
# * add_alias(tag, executable_path)
# * remove_alias(tag)
# * refresh_alias()
#
# * install(tag)
# * list_installs()

# These variables are filled in as part of initialization
PY_EXE = None
LOCAL_APPDATA = None
DRYRUN = False
VERBOSE = False
QUIET = False

def init(py_exe, local_appdata, dryrun, verbose, quiet):
    global PY_EXE, LOCAL_APPDATA, DRYRUN, VERBOSE, QUIET
    PY_EXE = py_exe
    LOCAL_APPDATA = local_appdata
    DRYRUN = bool(dryrun)
    VERBOSE = bool(verbose)
    QUIET = bool(quiet)


import os
import subprocess
import sys

def debug(message):
    if VERBOSE:
        print(message, file=sys.stderr)


STORE_SEARCH = [
    # 3.12 by default
    ( "3", "9NCVDN91XZQP" ),
    ( "3.13", "9PNRBTZXMB4Z" ),
    ( "3.12", "9NCVDN91XZQP" ),
    ( "3.11", "9NRWMJP3717K" ),
    ( "3.10", "9PJPW5LDXLZ5" ),
    ( "3.9", "9P7QFQMJRFP7" ),
    ( "3.8", "9MSSZTT1N39L" ),
]

def _get_store_id_for_tag(tag):
    matches = [sid for t, sid in STORE_SEARCH if t == tag]
    if not matches:
        raise RuntimeError()
    return matches[0]

WINGET_COMMAND = "Microsoft\\WindowsApps\\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\\winget.exe"
WINGET_ARGUMENTS = ["install", "-q", "$STORE_ID$", "--exact", "--accept-package-agreements", "--source", "msstore"]


def _winget_install(tag):
    cmd = os.path.join(LOCAL_APPDATA, WINGET_COMMAND)
    store_id = _get_store_id_for_tag(tag)
    args = [store_id if a == "$STORE_ID$" else a for a in WINGET_ARGUMENTS]
    if QUIET:
        args.append("--silent")
    else:
        print("Launching winget to install Python. The following output is from the install process")
        print("***********************************************************************")
    debug(f"# Executing {cmd} {' '.join(args)}")
    success = False
    try:
        if DRYRUN:
            debug(f"# Skipping due to PYLAUNCHER_DRYRUN")
            return True
        subprocess.check_call([cmd, *args], executable=cmd)
        success = True
    finally:
        print("***********************************************************************")
        if success:
            print("Install appears to have succeeded. Searching for new matching installs.")
        else:
            print("Please check the install status and run your command again.")
    return True


MSSTORE_COMMAND = "ms-windows-store://pdp/?productid="

def _msstore_open(tag):
    if QUIET:
        return False
    store_id = _get_store_id_for_tag(tag)
    print("Opening the Microsoft Store to install Python. After installation, please run your command again.")
    if DRYRUN:
        debug(f"# Opening {MSSTORE_COMMAND}{store_id}")
    else:
        os.startfile(MSSTORE_COMMAND + store_id)
    return True


def install(tag):
    if _winget_install(tag):
        return
    if _msstore_open(tag):
        return
    raise RuntimeError()
