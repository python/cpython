"""High-level support for working with subinterpreters in asyncio"""

import asyncio
from concurrent.futures.interpreter import InterpreterPoolExecutor

__all__ = "run_in_subinterpreter",


async def run_in_subinterpreter(func, /, *args, **kwargs):
    
    """
    Run a Python function in a subinterpreter asynchronously.

    This function executes the given callable `func` in a subinterpreter
    using `InterpreterPoolExecutor`.

    Args:
        func (Callable): The function to run in the subinterpreter.
        *args: Positional arguments to pass to `func`.
        **kwargs: Keyword arguments to pass to `func`.

    Returns:
        Any: The result returned by `func`.
    """
    
    with InterpreterPoolExecutor() as pool:
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(pool, func, *args, **kwargs)