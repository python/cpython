import os
from Carbon import Qd
from Carbon import Win
from Carbon import Qt, QuickTime
import W
from Carbon import File
from Carbon import Evt, Events

_moviesinitialized = 0

def EnterMovies():
    global _moviesinitialized
    if not _moviesinitialized:
        Qt.EnterMovies()
        _moviesinitialized = 1

class Movie(W.Widget):

    def __init__(self, possize):
        EnterMovies()
        self.movie = None
        self.running = 0
        W.Widget.__init__(self, possize)

    def adjust(self, oldbounds):
        self.SetPort()
        self.GetWindow().InvalWindowRect(oldbounds)
        self.GetWindow().InvalWindowRect(self._bounds)
        self.calcmoviebox()

    def set(self, path_or_fss, start = 0):
        self.SetPort()
        if self.movie:
            #self.GetWindow().InvalWindowRect(self.movie.GetMovieBox())
            Qd.PaintRect(self.movie.GetMovieBox())
        path = File.pathname(path)
        self.movietitle = os.path.basename(path)
        movieResRef = Qt.OpenMovieFile(path_or_fss, 1)
        self.movie, dummy, dummy = Qt.NewMovieFromFile(movieResRef, 0, QuickTime.newMovieActive)
        self.moviebox = self.movie.GetMovieBox()
        self.calcmoviebox()
        Qd.ObscureCursor()      # XXX does this work at all?
        self.movie.GoToBeginningOfMovie()
        if start:
            self.movie.StartMovie()
            self.running = 1
        else:
            self.running = 0
            self.movie.MoviesTask(0)

    def get(self):
        return self.movie

    def getmovietitle(self):
        return self.movietitle

    def start(self):
        if self.movie:
            Qd.ObscureCursor()
            self.movie.StartMovie()
            self.running = 1

    def stop(self):
        if self.movie:
            self.movie.StopMovie()
            self.running = 0

    def rewind(self):
        if self.movie:
            self.movie.GoToBeginningOfMovie()

    def calcmoviebox(self):
        if not self.movie:
            return
        ml, mt, mr, mb = self.moviebox
        wl, wt, wr, wb = widgetbox = self._bounds
        mheight = mb - mt
        mwidth = mr - ml
        wheight = wb - wt
        wwidth = wr - wl
        if (mheight * 2 < wheight) and (mwidth * 2 < wwidth):
            scale = 2
        elif mheight > wheight or mwidth > wwidth:
            scale = min(float(wheight) / mheight, float(wwidth) / mwidth)
        else:
            scale = 1
        mwidth, mheight = mwidth * scale, mheight * scale
        ml, mt = wl + (wwidth - mwidth) / 2, wt + (wheight - mheight) / 2
        mr, mb = ml + mwidth, mt + mheight
        self.movie.SetMovieBox((ml, mt, mr, mb))

    def idle(self, *args):
        if self.movie:
            if not self.movie.IsMovieDone() and self.running:
                Qd.ObscureCursor()
                while 1:
                    self.movie.MoviesTask(0)
                    gotone, event = Evt.EventAvail(Events.everyEvent)
                    if gotone or self.movie.IsMovieDone():
                        break
            elif self.running:
                box = self.movie.GetMovieBox()
                self.SetPort()
                self.GetWindow().InvalWindowRect(box)
                self.movie = None
                self.running = 0

    def draw(self, visRgn = None):
        if self._visible:
            Qd.PaintRect(self._bounds)
            if self.movie:
                self.movie.UpdateMovie()
                self.movie.MoviesTask(0)
