# Created on 4/15/99 by Loren Luke
#
# Color Prefs for idle

class ColorPrefs:
    CNormal     = "yellow", None
    CKeyword    = "medium sea green", None
    CComment    = "white", None
    CString     = "DarkOrange1", None
    CDefinition = "wheat1", None
    CHilite     = "#000068", "#006868"
    CSync       = "yellow", None
    CTodo       = "aquamarine4", None
    CBreak      = "white", None
    CHit        = "#000068", None
    CStdIn      = "yellow", None
    CStdOut     = "yellow", None
    CStdErr     = "firebrick3", None
    CConsole    = "yellow", None
    CError      = "white", "red"
    CCursor     = None, "yellow"
    
    def __init__(self, fg="yellow", bg="DodgerBlue4", ud=1):
        self.Default = fg, bg
        
        # Use Default Colors?
        if ud == 1:
            # Use Default fg, bg Colors
            # Define Screen Colors...
            # format: x = (fg, bg)
            self.CBreak      = "white", "#680000"
            self.CComment    = "white", self.Default[1]
            self.CConsole    = self.Default
            self.CCursor     = None, self.Default[0]
            self.CDefinition = "wheat1", self.Default[1]
            self.CError      = "white", "red"
            self.CHilite     = "#000068", "#006868"
            self.CHit        = "#000068", "#006868"
            self.CKeyword    = "medium sea green", self.Default[1]
            self.CNormal     = "yellow", self.Default[1]
            self.CStdErr     = "firebrick3", self.Default[1]
            self.CStdIn      = self.Default
            self.CStdOut     = self.Default
            self.CString     = "DarkOrange1", self.Default[1]
            self.CSync       = self.Default
            self.CTodo       = "aquamarine4", self.Default[1]
        else:
            #Define Screen Colors...
            # format: x = (fg, bg)
            self.CBreak      = "white", None
            self.CComment    = "white", None
            self.CConsole    = "yellow", None
            self.CCursor     = None, "yellow"
            self.CDefinition = "wheat1", None
            self.CError      = "white", "red"
            self.CHilite     = "#000068", "#006868"
            self.CHit        = "#000068", None
            self.CKeyword    = "medium sea green", None
            self.CNormal     = "yellow", None
            self.CStdErr     = "firebrick3", None
            self.CStdIn      = "yellow", None
            self.CStdOut     = "yellow", None
            self.CString     = "DarkOrange1", None
            self.CSync       = "yellow", None
            self.CTodo       = "aquamarine4", None
