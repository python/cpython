from builtins import open as _orig_open

def open(file, mode='r', bufsize=-1):
    if 'w' not in mode:
        return _orig_open(file, mode, bufsize)
    import os
    backup = file + '~'
    try:
        os.unlink(backup)
    except OSError:
        pass
    try:
        os.rename(file, backup)
    except OSError:
        return _orig_open(file, mode, bufsize)
    f = _orig_open(file, mode, bufsize)
    _orig_close = f.close
    def close():
        _orig_close()
        import filecmp
        if filecmp.cmp(backup, file, shallow=False):
            import os
            os.unlink(file)
            os.rename(backup, file)
    f.close = close
    return f
