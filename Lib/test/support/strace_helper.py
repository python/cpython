import re
import sys
import textwrap
import unittest
from dataclasses import dataclass
from test import support
from test.support.script_helper import run_python_until_end
from typing import Dict, List

_strace_binary = "/usr/bin/strace"
_syscall_regex = re.compile(
    r"(?P<syscall>[^(]*)\((?P<args>[^)]*)\)\s*[=]\s*(?P<returncode>.+)")
_returncode_regex = re.compile(
    r"\+\+\+ exited with (?P<returncode>\d+) \+\+\+")

# Cached value of whether or not there is a compatible strace binary
_strace_working: bool | None = None


@dataclass
class StraceEvent:
    syscall: str
    args: List[str]
    returncode: str


@dataclass
class StraceResult:
    strace_returncode: int
    python_returncode: int
    _raw_events: str
    stdout: str
    stderr: str

    def events(self) -> List[StraceEvent]:
        """Extract the call list information from _raw_events"""
        matches = [
            _syscall_regex.match(event)
            for event in self._raw_events.splitlines()
        ]
        return [
            StraceEvent(match["syscall"],
                        [arg.strip() for arg in (match["args"].split(","))],
                        match["returncode"]) for match in matches if match
        ]

    def sections(self) -> Dict[str:List[StraceEvent]]:
        """Find all "MARK <X>" writes and use them to make groups of events.

        This is useful to avoid variable / overhead strace events, like that
        at interpreter startup, so a test can just check does the small case
        under study work."""
        current_section = "__startup"
        sections = {current_section: []}
        for event in self.events():
            if event.syscall == 'write' and len(
                    event.args) > 2 and event.args[1].startswith("\"MARK "):
                # Found a new section, don't include the write in the section
                # but all events until next mark should be in that section
                current_section = event.args[1].split(
                    " ", 1)[1].removesuffix('\\n"')
                if current_section not in sections:
                    sections[current_section] = list()
            else:
                sections[current_section].append(event)

        return sections


@support.requires_subprocess()
def strace_python(code: str,
                  strace_flags: List[str],
                  check: bool = True) -> StraceResult:
    """Run strace and return the trace.

    Sets strace_returncode and python_returncode to `-1` on error
    """
    res = None

    def _make_error(reason, details):
        return StraceResult(
            strace_returncode=-1,
            python_returncode=-1,
            _raw_events=f"error({reason},details={details}) = -1",
            stdout=res.out if res else "",
            stderr=res.err if res else "")

    # Run strace, and get out the raw text
    try:
        res, cmd_line = run_python_until_end(
            "-c",
            textwrap.dedent(code),
            __run_using_command=[_strace_binary] + strace_flags)
    except OSError as err:
        return _make_error("Caught OSError", err)

    # Get out program returncode
    decoded = res.err.decode().strip()

    output = decoded.rsplit("\n", 1)
    if len(output) != 2:
        return _make_error("Expected strace events and exit code line",
                           decoded[-50:])

    returncode_match = _returncode_regex.match(output[1])
    if not returncode_match:
        return _make_error("Expected to find returncode in last line.",
                           output[1][:50])

    python_returncode = int(returncode_match["returncode"])
    if check and (res.rc or python_returncode):
        res.fail(cmd_line)

    return StraceResult(strace_returncode=res.rc,
                        python_returncode=python_returncode,
                        _raw_events=output[0],
                        stdout=res.out,
                        stderr=res.err)


def _get_events(code: str, strace_flags: List[str], prelude: str,
                cleanup: str) -> List[StraceEvent]:
    # NOTE: The flush is currently required to prevent the prints from getting
    # buffered and done all at once at exit
    prelude = textwrap.dedent(prelude)
    code = textwrap.dedent(code)
    cleanup = textwrap.dedent(cleanup)
    to_run = f"""
print("MARK prelude", flush=True)
{prelude}
print("MARK code", flush=True)
{code}
print("MARK cleanup", flush=True)
{cleanup}
print("MARK __shutdown", flush=True)
    """
    trace = strace_python(to_run, strace_flags)
    all_sections = trace.sections()
    return all_sections['code']


def get_syscalls(code: str,
                 strace_flags: List[str],
                 prelude: str = "",
                 cleanup: str = "") -> List[str]:
    """Get the syscalls which a given chunk of python code generates"""
    events = _get_events(code, strace_flags, prelude=prelude, cleanup=cleanup)
    return [ev.syscall for ev in events]


def _can_strace():
    res = strace_python("import sys; sys.exit(0)", [], check=False)
    assert res.events(), "Should have parsed multiple calls"

    global _strace_working
    _strace_working = res.strace_returncode == 0 and res.python_returncode == 0


def requires_strace():
    if sys.platform != "linux":
        return unittest.skip("Linux only, requires strace.")
    # Moderately expensive (spawns a subprocess), so share results when possible.
    if _strace_working is None:
        _can_strace()

    assert _strace_working is not None, "Should have been set by _can_strace"
    return unittest.skipUnless(_strace_working, "Requires working strace")


__all__ = ["requires_strace", "strace_python", "StraceResult", "StraceEvent"]
