"""Utilities for locating LLVM tools."""

import functools
import re
import shlex
import subprocess

LLVM_VERSION = 16


def _check_tool_version(name: str, *, echo: bool = False) -> bool:
    """Check if an LLVM tool matches LLVM_VERSION."""
    args = [name, "--version"]
    if echo:
        print(shlex.join(args))
    try:
        process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
    except FileNotFoundError:
        return False
    pattern = rf"version\s+{LLVM_VERSION}\.\d+\.\d+\s+"
    return re.search(pattern, process.stdout.decode()) is not None


@functools.cache
def find_tool(tool: str, *, echo: bool = False) -> str | None:
    """Find an LLVM tool with LLVM_VERSION. Otherwise, return None."""
    # Unversioned executables:
    if _check_tool_version(path := tool, echo=echo):
        return path
    # Versioned executables:
    if _check_tool_version(path := f"{tool}-{LLVM_VERSION}", echo=echo):
        return path
    # My homebrew homies:
    args = ["brew", "--prefix", f"llvm@{LLVM_VERSION}"]
    if echo:
        print(shlex.join(args))
    try:
        process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None
    prefix = process.stdout.decode().removesuffix("\n")
    if _check_tool_version(path := f"{prefix}/bin/{tool}", echo=echo):
        return path
    return None


def require_tool(tool: str, *, echo: bool = False) -> str:
    """Find an LLVM tool with LLVM_VERSION. Otherwise, raise RuntimeError."""
    if path := find_tool(tool, echo=echo):
        return path
    raise RuntimeError(f"Can't find {tool}-{LLVM_VERSION}!")
