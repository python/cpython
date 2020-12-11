#!/usr/local/bin/tclsh
set mydir [file dirname [info script]]
lappend auto_path /usr/local/lib
package req doctools
doctools::new dt
set wd [pwd]
cd $mydir
file rename html htm
set code [catch {
    set f [open man.macros]
    set m [read $f]
    close $f
    foreach file [glob -nocomplain *.man] {
        set xx [file root $file]
        set f [open $xx.man]
        set t [read $f]
        close $f
        foreach {fmt ext dir} {nroff n man html html htm} {
            dt configure -format $fmt
            set o [dt format $t]
            set f [open $dir/$xx.$ext w]
            if {$fmt == "nroff"} {
                set o [string map [list {.so man.macros} $m] $o]
            }
            puts $f $o
            close $f
        }
    }
} err]
file rename htm html
cd $wd
if {$code} {
    error $err
}
exit 0
