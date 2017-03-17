Issue #27243: Update the __aiter__ protocol: instead of returning
an awaitable that resolves to an asynchronous iterator, the asynchronous
iterator should be returned directly.  Doing the former will trigger a
PendingDeprecationWarning.