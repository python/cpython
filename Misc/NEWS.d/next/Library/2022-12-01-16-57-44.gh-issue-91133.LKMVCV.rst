Fix a bug in :class:`tempfile.TemporaryDirectory` cleanup, which now no longer
dereferences symlinks when working around file system permission errors.
