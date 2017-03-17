Issue #25764: In the subprocess module, preserve any exception caused by
fork() failure when preexec_fn is used.