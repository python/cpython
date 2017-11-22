from sys import maxsize

def _last_version(libnames, sep):
    def _num_version(libname):
        # "libxyz.so.MAJOR.MINOR" => [MAJOR, MINOR]
        parts = libname.split(sep)
        nums = []
        try:
            while parts:
                nums.insert(0, int(parts.pop()))
        except ValueError:
            pass
        return nums or [maxsize]

    return max(reversed(libnames), key=_num_version)
