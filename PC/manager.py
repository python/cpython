# This is embedded into the py.exe launcher to provide management routines.
# The public API is entirely functions. Everything else is internal.
#
# * add_alias(tag, executable_path)
# * remove_alias(tag)
# * refresh_alias()
#
# * install(tag)
# * list_installs()

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

def warn(message):
    if not QUIET:
        print(message, file=sys.stderr)


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

# ==============================================================================
# ALIAS MANAGEMENT
# ==============================================================================

def _read_aliases_from_file():
    pre = []
    section = None
    post = None
    with open(os.path.join(PY_EXE, "..", "py.ini"), "r", encoding="utf-8", errors="surrogateescape") as f:
        for line in f:
            if post is not None:
                post.append(line.rstrip())
            elif section is not None:
                if line.lstrip().startswith("["):
                    post = [line.rstrip()]
                else:
                    section.append(line.rstrip())
            else:
                pre.append(line.rstrip())
                if pre[-1] == "[alias]":
                    section = []
    while section and not section[-1]:
        post.insert(0, section.pop())
    section_dict = {
        k: v
        for k, sep, v in (line.partition("=") for line in section)
        if sep == "="
    }
    return pre, section_dict, post


def _write_aliases_to_file(pre, section, post):
    with open(os.path.join(PY_EXE, "..", "py.ini"), "w", encoding="utf-8", errors="surrogateescape") as f:
        for line in pre:
            print(line, file=f)
        for key, value in section.items():
            print(key, value, sep="=", file=f)
        for line in post:
            print(line, file=f)


def _get_aliases(tag):
    if not PY_EXE:
        raise RuntimeError("Cannot create alias without py.exe path")
    src = PY_EXE
    company, _, tag = tag.rpartition("/")
    if company.lower() in ("", "pythoncore"):
        company = "python"
    #if PY_EXE.lower().endswith("_d.exe"):
    #    srcw = PY_EXE.rpartition("_d.")[0] + "w_d.exe"
    #else:
    #    srcw = PY_EXE.rpartition(".")[0] + "w.exe"
    yield f"{company}.exe", src
    #yield f"{company}w.exe", srcw
    yield f"{company}{tag}.exe", src
    #yield f"{company}w{tag}.exe", srcw
    for i, c in enumerate(tag):
        if not c.isalnum():
            yield f"{company}{tag[:i]}.exe", src
            #yield f"{company}w{tag[:i]}.exe", srcw
            break


def _manage_alias(tag, create=True):
    pre, section, post = _read_aliases_from_file()

    if tag is None:
        aliases = [(k, None) for k in section]
    else:
        aliases = _get_aliases(tag)

    for alias_name, exe_path in aliases:
        alias_exe = os.path.join(os.path.dirname(PY_EXE), alias_name)
        if create:
            debug(f"# Creating alias at '{alias_exe}'")
        else:
            debug(f"# Removing alias at '{alias_exe}'")
        if not DRYRUN:
            try:
                os.unlink(alias_exe)
            except FileNotFoundError:
                pass
            except PermissionError as ex:
                debug(f"# Failed to remove existing alias '{alias_exe}': {ex}")
            except Exception as ex:
                warn(f"Failed to remove alias at '{alias_exe}': {ex}")
            if create:
                try:
                    os.link(exe_path, alias_exe, follow_symlinks=False)
                except OSError:
                    shutil.copy2(exe_path, alias_exe, follow_symlinks=False)
                section[alias_name] = launch_command
            else:
                section.pop(alias_name, None)
            _write_aliases_to_file(pre, section, post)


def create_alias(tag):
    _manage_alias(tag, create=True)


def delete_alias(tag):
    _manage_alias(tag, create=False)


def delete_all_aliases():
    _manage_alias(None, create=False)
