import functools
import re
import shlex
import subprocess

LLVM_VERSION = 16


def get_tool_version(name: str, *, echo: bool = False) -> int | None:
    try:
        args = [name, "--version"]
        if echo:
            print(shlex.join(args))
        process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
    except FileNotFoundError:
        return None
    match = re.search(rb"version\s+(\d+)\.\d+\.\d+\s+", process.stdout)
    return int(match.group(1)) if match else None


@functools.cache
def find_tool(tool: str, *, echo: bool = False) -> str | None:
    # Unversioned executables:
    path = tool
    if get_tool_version(path, echo=echo) == LLVM_VERSION:
        return path
    # Versioned executables:
    path = f"{tool}-{LLVM_VERSION}"
    if get_tool_version(path, echo=echo) == LLVM_VERSION:
        return path
    # My homebrew homies:
    try:
        args = ["brew", "--prefix", f"llvm@{LLVM_VERSION}"]
        if echo:
            print(shlex.join(args))
        process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None
    prefix = process.stdout.decode().removesuffix("\n")
    path = f"{prefix}/bin/{tool}"
    if get_tool_version(path, echo=echo) == LLVM_VERSION:
        return path
    return None


def require_tool(tool: str, *, echo: bool = False) -> str:
    path = find_tool(tool, echo=echo)
    if path is not None:
        return path
    raise RuntimeError(f"Can't find {tool}-{LLVM_VERSION}!")
