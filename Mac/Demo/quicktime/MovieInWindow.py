"""MovieInWindow converted to python

Jack Jansen, CWI, December 1995
"""

from Carbon import Qt
from Carbon import QuickTime
from Carbon import Qd
from Carbon import QuickDraw
from Carbon import Evt
from Carbon import Events
from Carbon import Win
from Carbon import Windows
from Carbon import File
import EasyDialogs
import sys
import os


def main():
    # skip the toolbox initializations, already done
    # XXXX Should use gestalt here to check for quicktime version
    Qt.EnterMovies()

    # Get the movie file
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    else:
        filename = EasyDialogs.AskFileForOpen() # Was: QuickTime.MovieFileType
    if not filename:
        sys.exit(0)

    # Open the window
    bounds = (175, 75, 175+160, 75+120)
    theWindow = Win.NewCWindow(bounds, os.path.split(filename)[1], 1, 0, -1, 0, 0)
    Qd.SetPort(theWindow)
    # XXXX Needed? SetGWorld((CGrafPtr)theWindow, nil)

    playMovieInWindow(theWindow, filename, theWindow.GetWindowPort().GetPortBounds())

def playMovieInWindow(theWindow, theFile, movieBox):
    """Play a movie in a window"""
    # XXXX Needed?  SetGWorld((CGrafPtr)theWindow, nil);

    # Get the movie
    theMovie = loadMovie(theFile)

    # Set where we want it
    theMovie.SetMovieBox(movieBox)

    # Start at the beginning
    theMovie.GoToBeginningOfMovie()

    # Give a little time to preroll
    theMovie.MoviesTask(0)

    # Start playing
    theMovie.StartMovie()

    while not theMovie.IsMovieDone() and not Evt.Button():
        theMovie.MoviesTask(0)

def loadMovie(theFile):
    """Load a movie given an fsspec. Return the movie object"""
    movieResRef = Qt.OpenMovieFile(theFile, 1)
    movie, d1, d2 = Qt.NewMovieFromFile(movieResRef, 0, QuickTime.newMovieActive)
    return movie

if __name__ == '__main__':
    main()
