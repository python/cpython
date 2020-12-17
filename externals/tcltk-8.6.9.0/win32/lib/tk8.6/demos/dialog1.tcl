# dialog1.tcl --
#
# This demonstration script creates a dialog box with a local grab.

after idle {.dialog1.msg configure -wraplength 4i}
set i [tk_dialog .dialog1 "Dialog with local grab" {This is a modal dialog box.  It uses Tk's "grab" command to create a "local grab" on the dialog box.  The grab prevents any pointer-related events from getting to any other windows in the application until you have answered the dialog by invoking one of the buttons below.  However, you can still interact with other applications.} \
info 0 OK Cancel {Show Code}]

switch $i {
    0 {puts "You pressed OK"}
    1 {puts "You pressed Cancel"}
    2 {showCode .dialog1}
}
