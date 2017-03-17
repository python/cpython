Issue #27736: Prevent segfault after interpreter re-initialization due
to ref count problem introduced in code for Issue #27038 in 3.6.0a3.
Patch by Xiang Zhang.