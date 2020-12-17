if {([info commands ::tcl::pkgconfig] eq "")
	|| ([info sharedlibextension] ne ".dll")} return
if {[::tcl::pkgconfig get debug]} {
    package ifneeded registry 1.3.3 \
            [list load [file join $dir tclreg13g.dll] registry]
} else {
    package ifneeded registry 1.3.3 \
            [list load [file join $dir tclreg13.dll] registry]
}
