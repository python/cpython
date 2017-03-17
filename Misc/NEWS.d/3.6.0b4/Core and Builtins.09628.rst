Issue #27243: Change PendingDeprecationWarning -> DeprecationWarning.
As it was agreed in the issue, __aiter__ returning an awaitable
should result in PendingDeprecationWarning in 3.5 and in
DeprecationWarning in 3.6.