"""Utilities for invoking LLVM tools."""

import asyncio
import functools
import os
import re
import shlex
import subprocess
import typing

import _targets


_LLVM_VERSION = "21"
_EXTERNALS_LLVM_TAG = "llvm-21.1.4.0"

_P = typing.ParamSpec("_P")
_R = typing.TypeVar("_R")
_C = typing.Callable[_P, typing.Awaitable[_R]]


def _async_cache(f: _C[_P, _R]) -> _C[_P, _R]:
    cache = {}
    lock = asyncio.Lock()

    @functools.wraps(f)
    async def wrapper(
        *args: _P.args, **kwargs: _P.kwargs  # pylint: disable = no-member
    ) -> _R:
        async with lock:
            if args not in cache:
                cache[args] = await f(*args, **kwargs)
            return cache[args]

    return wrapper


_CORES = asyncio.BoundedSemaphore(os.cpu_count() or 1)


async def _run(tool: str, args: typing.Iterable[str], echo: bool = False) -> str | None:
    command = [tool, *args]
    async with _CORES:
        if echo:
            print(shlex.join(command))
        try:
            process = await asyncio.create_subprocess_exec(
                *command, stdout=subprocess.PIPE
            )
        except FileNotFoundError:
            return None
        out, _ = await process.communicate()
    if process.returncode:
        raise RuntimeError(f"{tool} exited with return code {process.returncode}")
    return out.decode()


@_async_cache
async def _check_tool_version(
    name: str, llvm_version: str, *, echo: bool = False
) -> bool:
    output = await _run(name, ["--version"], echo=echo)
    _llvm_version_pattern = re.compile(rf"version\s+{llvm_version}\.\d+\.\d+\S*\s+")
    return bool(output and _llvm_version_pattern.search(output))


@_async_cache
async def _get_brew_llvm_prefix(llvm_version: str, *, echo: bool = False) -> str | None:
    output = await _run("brew", ["--prefix", f"llvm@{llvm_version}"], echo=echo)
    return output and output.removesuffix("\n")


@_async_cache
async def _find_tool(tool: str, llvm_version: str, *, echo: bool = False) -> str | None:
    # Unversioned executables:
    path = tool
    if await _check_tool_version(path, llvm_version, echo=echo):
        return path
    # Versioned executables:
    path = f"{tool}-{llvm_version}"
    if await _check_tool_version(path, llvm_version, echo=echo):
        return path
    # PCbuild externals:
    externals = os.environ.get("EXTERNALS_DIR", _targets.EXTERNALS)
    path = os.path.join(externals, _EXTERNALS_LLVM_TAG, "bin", tool)
    # On Windows, executables need .exe extension
    if os.name == "nt" and not path.endswith(".exe"):
        path_with_exe = path + ".exe"
        if os.path.exists(path_with_exe):
            path = path_with_exe
    if await _check_tool_version(path, llvm_version, echo=echo):
        return path
    # Homebrew-installed executables:
    prefix = await _get_brew_llvm_prefix(llvm_version, echo=echo)
    if prefix is not None:
        path = os.path.join(prefix, "bin", tool)
        if await _check_tool_version(path, llvm_version, echo=echo):
            return path
    # Nothing found:
    return None


async def maybe_run(
    tool: str,
    args: typing.Iterable[str],
    echo: bool = False,
    llvm_version: str = _LLVM_VERSION,
) -> str | None:
    """Run an LLVM tool if it can be found. Otherwise, return None."""

    path = await _find_tool(tool, llvm_version, echo=echo)
    return path and await _run(path, args, echo=echo)


async def run(
    tool: str,
    args: typing.Iterable[str],
    echo: bool = False,
    llvm_version: str = _LLVM_VERSION,
) -> str:
    """Run an LLVM tool if it can be found. Otherwise, raise RuntimeError."""

    output = await maybe_run(tool, args, echo=echo, llvm_version=llvm_version)
    if output is None:
        raise RuntimeError(f"Can't find {tool}-{llvm_version}!")
    return output
