"""High-perfomance logging profiler, mostly written in C."""

import _hotshot

from _hotshot import ProfilerError


class Profile:
    def __init__(self, logfn, lineevents=0, linetimings=1):
        self.lineevents = lineevents and 1 or 0
        self.linetimings = (linetimings and lineevents) and 1 or 0
        self._prof = p = _hotshot.profiler(
            logfn, self.lineevents, self.linetimings)

    def close(self):
        self._prof.close()

    def start(self):
        self._prof.start()

    def stop(self):
        self._prof.stop()

    # These methods offer the same interface as the profile.Profile class,
    # but delegate most of the work to the C implementation underneath.

    def run(self, cmd):
        import __main__
        dict = __main__.__dict__
        return self.runctx(cmd, dict, dict)

    def runctx(self, cmd, globals, locals):
        code = compile(cmd, "<string>", "exec")
        self._prof.runcode(code, globals, locals)
        return self

    def runcall(self, func, *args, **kw):
        return self._prof.runcall(func, args, kw)
