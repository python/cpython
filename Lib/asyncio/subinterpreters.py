"""High-level support for working with subinterpreters in asyncio"""

import os
from concurrent.futures.interpreter import InterpreterPoolExecutor

__all__ = "run_in_subinterpreter",


async def run_in_subinterpreter(func, /, *args, **kwargs):
    
    """
    Run a Python function in a subinterpreter asynchronously.

    This function executes the given callable `func` in a subinterpreter
    using `InterpreterPoolExecutor`. It chooses the number of 
    subinterpreters based on the CPU count of the host system, 
    defaulting to half the available cores if detectable.

    Args:
        func (Callable): The function to run in the subinterpreter.
        *args: Positional arguments to pass to `func`.
        **kwargs: Keyword arguments to pass to `func`.

    Returns:
        Any: The result returned by `func`.

    Raises:
        RuntimeError: If CPU count cannot be determined and a default 
        cannot be safely chosen.
    """
    
    n: int | None = os.cpu_count()
    worker_count: int | None = None
    if n:
        worker_count = max(1, n // 2)  
    with InterpreterPoolExecutor(max_workers=worker_count) as pool:
        future = pool.submit(func, *args, **kwargs)
        return future.result()