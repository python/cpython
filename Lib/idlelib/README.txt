README.txt: an index to idlelib files and the IDLE menu.

IDLE is Python’s Integrated Development and Learning
Environment.  The user documentation is part of the Library Reference and
is available in IDLE by selecting Help => IDLE Help.  This README documents
idlelib for IDLE developers and curious users.

IDLELIB FILES lists files alphabetically by category,
with a short description of each.

IDLE MENU show the menu tree, annotated with the module
or module object that implements the corresponding function.

This file is descriptive, not prescriptive, and may have errors
and omissions and lag behind changes in idlelib.


IDLELIB FILES
Implemetation files not in IDLE MENU are marked (nim).
Deprecated files and objects are listed separately as the end.

Startup
-------
__init__.py  # import, does nothing
__main__.py  # -m, starts IDLE
idle.bat
idle.py
idle.pyw

Implementation
--------------
AutoComplete.py   # Complete attribute names or filenames.
AutoCompleteWindow.py  # Display completions.
AutoExpand.py     # Expand word with previous word in file.
Bindings.py       # Define most of IDLE menu.
CallTipWindow.py  # Display calltip.
CallTips.py       # Create calltip text.
ClassBrowser.py   # Create module browser window.
CodeContext.py    # Show compound statement headers otherwise not visible.
ColorDelegator.py # Colorize text (nim).
Debugger.py       # Debug code run from editor; show window.
Delegator.py      # Define base class for delegators (nim).
EditorWindow.py   # Define most of editor and utility functions.
FileList.py       # Open files and manage list of open windows (nim).
FormatParagraph.py# Re-wrap multiline strings and comments.
GrepDialog.py     # Find all occurrences of pattern in multiple files.
HyperParser.py    # Parse code around a given index.
IOBinding.py      # Open, read, and write files
IdleHistory.py    # Get previous or next user input in shell (nim)
MultiCall.py      # Wrap tk widget to allow multiple calls per event (nim).
MultiStatusBar.py # Define status bar for windows (nim).
ObjectBrowser.py  # Define class used in StackViewer (nim).
OutputWindow.py   # Create window for grep output.
ParenMatch.py     # Match fenceposts: (), [], and {}.
PathBrowser.py    # Create path browser window.
Percolator.py     # Manage delegator stack (nim).
PyParse.py        # Give information on code indentation
PyShell.py        # Start IDLE, manage shell, complete editor window
RemoteDebugger.py # Debug code run in remote process.
RemoteObjectBrowser.py # Communicate objects between processes with rpc (nim).
ReplaceDialog.py  # Search and replace pattern in text.
RstripExtension.py# Strip trailing whitespace
ScriptBinding.py  # Check and run user code.
ScrolledList.py   # Define ScrolledList widget for IDLE (nim).
SearchDialog.py   # Search for pattern in text.
SearchDialogBase.py  # Define base for search, replace, and grep dialogs.
SearchEngine.py   # Define engine for all 3 search dialogs.
StackViewer.py    # View stack after exception.
TreeWidget.py     # Define tree widger, used in browsers (nim).
UndoDelegator.py  # Manage undo stack.
WidgetRedirector.py # Intercept widget subcommands (for percolator) (nim).
WindowList.py     # Manage window list and define listed top level.
ZoomHeight.py     # Zoom window to full height of screen.
aboutDialog.py    # Display About IDLE dialog.
configDialog.py   # Display user configuration dialogs.
configHandler.py  # Load, fetch, and save configuration (nim).
configHelpSourceEdit.py  # Specify help source.
configSectionNameDialog.py  # Spefify user config section name
dynOptionMenuWidget.py  # define mutable OptionMenu widget (nim).
help.py           # Display IDLE's html doc.
keybindingDialog.py  # Change keybindings.
macosxSupport.py  # Help IDLE run on Macs (nim).
rpc.py            # Commuicate between idle and user processes (nim).
run.py            # Manage user code execution subprocess.
tabbedpages.py    # Define tabbed pages widget (nim).
textView.py       # Define read-only text widget (nim).

Configuration
-------------
config-extensions.def # Defaults for extensions
config-highlight.def  # Defaults for colorizing
config-keys.def       # Defaults for key bindings
config-main.def       # Defai;ts fpr font and geneal

Text
----
CREDITS.txt  # not maintained, displayed by About IDLE
HISTORY.txt  # NEWS up to July 2001
NEWS.txt     # commits, displayed by About IDLE
README.txt   # this file, displeyed by About IDLE
TODO.txt     # needs review
extend.txt   # about writing extensions
help.html    # copy of idle.html in docs, displayed by IDLE Help

Subdirectories
--------------
Icons  # small image files
idle_test  # files for human test and automated unit tests

Unused and Deprecated files and objects (nim)
---------------------------------------------
EditorWindow.py: Helpdialog and helpDialog
ToolTip.py: unused.
help.txt
idlever.py


IDLE MENUS
Top level items and most submenu items are defined in Bindings.
Extenstions add submenu items when active.  The names given are
found, quoted, in one of these modules, paired with a '<<pseudoevent>>'.
Each pseudoevent is bound to an event handler.  Some event handlers
call another function that does the actual work.  The annotations below
are intended to at least give the module where the actual work is done.

File  # IOBindig except as noted
  New File
  Open...  # IOBinding.open
  Open Module
  Recent Files
  Class Browser  # Class Browser
  Path Browser  # Path Browser
  ---
  Save  # IDBinding.save
  Save As...  # IOBinding.save_as
  Save Copy As...  # IOBindling.save_a_copy
  ---
  Print Window  # IOBinding.print_window
  ---
  Close
  Exit

Edit
  Undo  # undoDelegator
  Redo  # undoDelegator
  ---
  Cut
  Copy
  Paste
  Select All
  ---  # Next 5 items use SearchEngine; dialogs use SearchDialogBase
  Find  # Search Dialog
  Find Again
  Find Selection
  Find in Files...  # GrepDialog
  Replace...  # ReplaceDialog
  Go to Line
  Show Completions  # AutoComplete extension and AutoCompleteWidow (&HP)
  Expand Word  # AutoExpand extension
  Show call tip  # Calltips extension and CalltipWindow (& Hyperparser)
  Show surrounding parens  # ParenMatch (& Hyperparser)

Shell  # PyShell
  View Last Restart  # PyShell.?
  Restart Shell  # PyShell.?

Debug (Shell only)
  Go to File/Line
  Debugger  # Debugger, RemoteDebugger
  Stack Viewer  # StackViewer
  Auto-open Stack Viewer  # StackViewer

Format (Editor only)
  Indent Region
  Dedent Region
  Comment Out Region
  Uncomment Region
  Tabify Region
  Untabify Region
  Toggle Tabs
  New Indent Width
  Format Paragraph  # FormatParagraph extension
  ---
  Strip tailing whitespace  # RstripExtension extension

Run (Editor only)
  Python Shell  # PyShell
  ---
  Check Module  # ScriptBinding
  Run Module  # ScriptBinding

Options
  Configure IDLE  # configDialog
    (tabs in the dialog)
    Font tab  # onfig-main.def
    Highlight tab  # configSectionNameDialog, config-highlight.def
    Keys tab  # keybindingDialog, configSectionNameDialog, onfig-keus.def
    General tab  # configHelpSourceEdit, config-main.def
  Configure Extensions  # configDialog
    Xyz tab  # xyz.py, config-extensions.def
  ---
  Code Context (editor only)  # CodeContext extension

Window
  Zoomheight  # ZoomHeight extension
  ---
  <open windows>  # WindowList

Help
  About IDLE  # aboutDialog
  ---
  IDLE Help  # help
  Python Doc
  Turtle Demo
  ---
  <other help sources>

<Context Menu> (right click)
Defined in EditorWindow, PyShell, Output
   Cut
   Copy
   Paste
   ---
   Go to file/line (shell and output only)
   Set Breakpoint (editor only)
   Clear Breakpoint (editor only)
 Defined in Debugger
   Go to source line
   Show stack frame
