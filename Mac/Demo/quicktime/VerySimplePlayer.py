"""VerySimplePlayer converted to python

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

# XXXX maxbounds = (40, 40, 1000, 1000)

def main():
    print('hello world') # XXXX
    # skip the toolbox initializations, already done
    # XXXX Should use gestalt here to check for quicktime version
    Qt.EnterMovies()

    # Get the movie file
    fss = EasyDialogs.AskFileForOpen(wanted=File.FSSpec) # Was: QuickTime.MovieFileType
    if not fss:
        sys.exit(0)

    # Open the window
    bounds = (175, 75, 175+160, 75+120)
    theWindow = Win.NewCWindow(bounds, fss.as_tuple()[2], 0, 0, -1, 1, 0)
    # XXXX Needed? SetGWorld((CGrafPtr)theWindow, nil)
    Qd.SetPort(theWindow)

    # Get the movie
    theMovie = loadMovie(fss)

    # Relocate to (0, 0)
    bounds = theMovie.GetMovieBox()
    bounds = 0, 0, bounds[2]-bounds[0], bounds[3]-bounds[1]
    theMovie.SetMovieBox(bounds)

    # Create a controller
    theController = theMovie.NewMovieController(bounds, QuickTime.mcTopLeftMovie)

    # Get movie size and update window parameters
    rv, bounds = theController.MCGetControllerBoundsRect()
    theWindow.SizeWindow(bounds[2], bounds[3], 0)   # XXXX or [3] [2]?
    Qt.AlignWindow(theWindow, 0)
    theWindow.ShowWindow()

    # XXXX MCDoAction(theController, mcActionSetGrowBoxBounds, &maxBounds)
    theController.MCDoAction(QuickTime.mcActionSetKeysEnabled, '1')

    # XXXX MCSetActionFilterWithRefCon(theController, movieControllerEventFilter, (long)theWindow)

    done = 0
    while not done:
        gotone, evt = Evt.WaitNextEvent(0xffff, 0)
        (what, message, when, where, modifiers) = evt
##              print what, message, when, where, modifiers # XXXX

        if theController.MCIsPlayerEvent(evt):
            continue

        if what == Events.mouseDown:
            part, whichWindow = Win.FindWindow(where)
            if part == Windows.inGoAway:
                done = whichWindow.TrackGoAway(where)
            elif part == Windows.inDrag:
                Qt.DragAlignedWindow(whichWindow, where, (0, 0, 4000, 4000))
        elif what == Events.updateEvt:
            whichWindow = Win.WhichWindow(message)
            if not whichWindow:
                # Probably the console window. Print something, hope it helps.
                print('update')
            else:
                Qd.SetPort(whichWindow)
                whichWindow.BeginUpdate()
                Qd.EraseRect(whichWindow.GetWindowPort().GetPortBounds())
                whichWindow.EndUpdate()

def loadMovie(theFile):
    """Load a movie given an fsspec. Return the movie object"""
    movieResRef = Qt.OpenMovieFile(theFile, 1)
    movie, d1, d2 = Qt.NewMovieFromFile(movieResRef, 0, QuickTime.newMovieActive)
    return movie

if __name__ == '__main__':
    main()
