========================================================================
              Copyright (c) 1999 Bill Bedford				
========================================================================
Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that the copyright notice and warranty disclaimer appear in
supporting documentation.

Bill Bedford disclaims all warranties with regard to this software,
including all implied warranties of merchantability and fitness.  In
no event shall Bill Bedford be liable for any special, indirect or
consequential damages or any damages whatsoever resulting from loss of
use, data or profits, whether in an action of contract, negligence or
other tortuous action, arising out of or in connection with the use or
performance of this software.
========================================================================


PythonDetectors is a set of Apple Data Detectors that will open an entry in 
the Python Library from a keyword in contextual menus.  It looks up either 
module name and open the relevant page or looks up a built in function and 
opens at the entry in the Builtin functions page. 

If anyone would like more functionality please contact me.

To use it you must have Apple Data Detectors 1.0.2 installed This can be 
obtained from http://www.apple.com/applescript/data_detectors/ 

Two action files are provided "OpenPythonLib" will open the library file 
with whatever application is defined as the Internet Config 'file' helper.  
"OpenPythonLib with NS" opens the library file with Netscape.  

Instructions
============

1/ Open the two apple script files with the Script Editor and change the 
first compiled line to point to the location of your Python Library 
folder.

2/ Open the Apple Data Detectors Control Panel and choose Install Detector 
File...  from the File menu.

3/  Pick a Detector file from the dialog.

4/  Let the Data Detectors Control Panel optimize the detector.

5/ Choose Install Action File...  from the File menu.

6/ Pick an action file from the dialog.

7/ The Data Detectors Control Panel will automatically place the new action 
with its detector.  You can click on the action in the control panel window 
to view information about its use.


Gotchas
=======

Unfortunately ADD only works with the US keyboard installed.
