#import re

from ..info import KIND, ParsedItem, FileInfo


class TextInfo:

    def __init__(self, text, start=None, end=None):
        # immutable:
        if not start:
            start = 1
        self.start = start

        # mutable:
        lines = text.splitlines() or ['']
        self.text = text.strip()
        if not end:
            end = start + len(lines) - 1
        self.end = end
        self.line = lines[-1]

    def __repr__(self):
        args = (f'{a}={getattr(self, a)!r}'
                for a in ['text', 'start', 'end'])
        return f'{type(self).__name__}({", ".join(args)})'

    def add_line(self, line, lno=None):
        if lno is None:
            lno = self.end + 1
        else:
            if isinstance(lno, FileInfo):
                fileinfo = lno
                if fileinfo.filename != self.filename:
                    raise NotImplementedError((fileinfo, self.filename))
                lno = fileinfo.lno
            # XXX
            #if lno < self.end:
            #    raise NotImplementedError((lno, self.end))
        line = line.lstrip()
        self.text += ' ' + line
        self.line = line
        self.end = lno


class SourceInfo:

    _ready = False

    def __init__(self, filename, _current=None):
        # immutable:
        self.filename = filename
        # mutable:
        if isinstance(_current, str):
            _current = TextInfo(_current)
        self._current = _current
        start = -1
        self._start = _current.start if _current else -1
        self._nested = []
        self._set_ready()

    def __repr__(self):
        args = (f'{a}={getattr(self, a)!r}'
                for a in ['filename', '_current'])
        return f'{type(self).__name__}({", ".join(args)})'

    @property
    def start(self):
        if self._current is None:
            return self._start
        return self._current.start

    @property
    def end(self):
        if self._current is None:
            return self._start
        return self._current.end

    @property
    def text(self):
        if self._current is None:
            return ''
        return self._current.text

    def nest(self, text, before, start=None):
        if self._current is None:
            raise Exception('nesting requires active source text')
        current = self._current
        current.text = before
        self._nested.append(current)
        self._replace(text, start)

    def resume(self, remainder=None):
        if not self._nested:
            raise Exception('no nested text to resume')
        if self._current is None:
            raise Exception('un-nesting requires active source text')
        if remainder is None:
            remainder = self._current.text
        self._clear()
        self._current = self._nested.pop()
        self._current.text += ' ' + remainder
        self._set_ready()

    def advance(self, remainder, start=None):
        if self._current is None:
            raise Exception('advancing requires active source text')
        if remainder.strip():
            self._replace(remainder, start, fixnested=True)
        else:
            if self._nested:
                self._replace('', start, fixnested=True)
                #raise Exception('cannot advance while nesting')
            else:
                self._clear(start)

    def resolve(self, kind, data, name, parent=None):
        # "field" isn't a top-level kind, so we leave it as-is.
        if kind and kind != 'field':
            kind = KIND._from_raw(kind)
        fileinfo = FileInfo(self.filename, self._start)
        return ParsedItem(fileinfo, kind, parent, name, data)

    def done(self):
        self._set_ready()

    def too_much(self, maxtext, maxlines):
        if maxtext and len(self.text) > maxtext:
            pass
        elif maxlines and self.end - self.start > maxlines:
            pass
        else:
            return False

        #if re.fullmatch(r'[^;]+\[\][ ]*=[ ]*[{]([ ]*\d+,)*([ ]*\d+,?)\s*',
        #                self._current.text):
        #    return False
        return True

    def _set_ready(self):
        if self._current is None:
            self._ready = False
        else:
            self._ready = self._current.text.strip() != ''

    def _used(self):
        ready = self._ready
        self._ready = False
        return ready

    def _clear(self, start=None):
        old = self._current
        if self._current is not None:
            # XXX Fail if self._current wasn't used up?
            if start is None:
                start = self._current.end
            self._current = None
        if start is not None:
            self._start = start
        self._set_ready()
        return old

    def _replace(self, text, start=None, *, fixnested=False):
        end = self._current.end
        old = self._clear(start)
        self._current = TextInfo(text, self._start, end)
        if fixnested and self._nested and self._nested[-1] is old:
            self._nested[-1] = self._current
        self._set_ready()

    def _add_line(self, line, lno=None):
        if not line.strip():
            # We don't worry about multi-line string literals.
            return
        if self._current is None:
            self._start = lno
            self._current = TextInfo(line, lno)
        else:
            # XXX
            #if lno < self._current.end:
            #    # A circular include?
            #    raise NotImplementedError((lno, self))
            self._current.add_line(line, lno)
        self._ready = True
