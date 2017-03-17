Issue #25304: Add asyncio.run_coroutine_threadsafe().  This lets you
submit a coroutine to a loop from another thread, returning a
concurrent.futures.Future.  By Vincent Michel.