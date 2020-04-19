"""Utilities to support asyncio in the Python regression tests."""

import asyncio.events

def maybe_get_event_loop_policy():
    """Return the global event loop policy if one is set, else return None."""
    return asyncio.events._event_loop_policy
