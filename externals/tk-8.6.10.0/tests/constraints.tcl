if {[namespace exists tk::test]} {
    deleteWindows
    wm geometry . {}
    raise .
    return
}

package require Tk 8.4
tk appname tktest
wm title . tktest
# If the main window isn't already mapped (e.g. because the tests are
# being run automatically) , specify a precise size for it so that the
# user won't have to position it manually.

if {![winfo ismapped .]} {
    wm geometry . +0+0
    update
}

package require tcltest 2.1

namespace eval tk {
    namespace eval test {

	namespace export loadTkCommand
	proc loadTkCommand {} {
	    set tklib {}
	    foreach pair [info loaded {}] {
		foreach {lib pfx} $pair break
		if {$pfx eq "Tk"} {
		    set tklib $lib
		    break
		}
	    }
	    return [list load $tklib Tk]
	}

	namespace eval bg {
	    # Manage a background process.
	    # Replace with slave interp or thread?
	    namespace import ::tcltest::interpreter
	    namespace import ::tk::test::loadTkCommand
	    namespace export setup cleanup do

	    proc cleanup {} {
		variable fd
		# catch in case the background process has closed $fd
		catch {puts $fd exit}
		catch {close $fd}
		set fd ""
	    }
	    proc setup args {
		variable fd
		if {[info exists fd] && [string length $fd]} {
		    cleanup
		}
		set fd [open "|[list [interpreter] \
			-geometry +0+0 -name tktest] $args" r+]
		puts $fd "puts foo; flush stdout"
		flush $fd
		if {[gets $fd data] < 0} {
		    error "unexpected EOF from \"[interpreter]\""
		}
		if {$data ne "foo"} {
		    error "unexpected output from\
			    background process: \"$data\""
		}
		puts $fd [loadTkCommand]
		flush $fd
		fileevent $fd readable [namespace code Ready]
	    }
	    proc Ready {} {
		variable fd
		variable Data
		variable Done
		set x [gets $fd]
		if {[eof $fd]} {
		    fileevent $fd readable {}
		    set Done 1
		} elseif {$x eq "**DONE**"} {
		    set Done 1
		} else {
		    append Data $x
		}
	    }
	    proc do {cmd {block 0}} {
		variable fd
		variable Data
		variable Done
		if {$block} {
		    fileevent $fd readable {}
		}
		puts $fd "[list catch $cmd msg]; update; puts \$msg;\
			puts **DONE**; flush stdout"
		flush $fd
		set Data {}
		if {$block} {
		    while {![eof $fd]} {
			set line [gets $fd]
			if {$line eq "**DONE**"} {
			    break
			}
			append Data $line
		    }
		} else {
		    set Done 0
		    vwait [namespace which -variable Done]
		}
		return $Data
	    }
	}

	proc Export {internal as external} {
	    uplevel 1 [list namespace import $internal]
	    uplevel 1 [list rename [namespace tail $internal] $external]
	    uplevel 1 [list namespace export $external]
	}
	Export bg::setup as setupbg
	Export bg::cleanup as cleanupbg
	Export bg::do as dobg

	namespace export deleteWindows
	proc deleteWindows {} {
	    eval destroy [winfo children .]
	}

	namespace export fixfocus
	proc fixfocus {} {
            catch {destroy .focus}
            toplevel .focus
            wm geometry .focus +0+0
            entry .focus.e
            .focus.e insert 0 "fixfocus"
            pack .focus.e
            update
            focus -force .focus.e
            destroy .focus
	}


        namespace export imageInit imageFinish imageCleanup imageNames
        variable ImageNames
        proc imageInit {} {
            variable ImageNames
            if {![info exists ImageNames]} {
                set ImageNames [lsort [image names]]
            }
            imageCleanup
            if {[lsort [image names]] ne $ImageNames} {
                return -code error "IMAGE NAMES mismatch: [image names] != $ImageNames"
            }
        }
        proc imageFinish {} {
            variable ImageNames
            if {[lsort [image names]] ne $ImageNames} {
                return -code error "images remaining: [image names] != $ImageNames"
            }
            imageCleanup
        }
        proc imageCleanup {} {
            variable ImageNames
            foreach img [image names] {
                if {$img ni $ImageNames} {image delete $img}
            }
        }
        proc imageNames {} {
            variable ImageNames
            set r {}
            foreach img [image names] {
                if {$img ni $ImageNames} {lappend r $img}
            }
            return $r
        }

    }
}

namespace import -force tk::test::*

namespace import -force tcltest::testConstraint
testConstraint notAqua [expr {[tk windowingsystem] ne "aqua"}]
testConstraint aqua [expr {[tk windowingsystem] eq "aqua"}]
testConstraint x11 [expr {[tk windowingsystem] eq "x11"}]
testConstraint nonwin [expr {[tk windowingsystem] ne "win32"}]
testConstraint aquaOrWin32 [expr {
    ([tk windowingsystem] eq "win32") || [testConstraint aqua]
}]
testConstraint userInteraction 0
testConstraint nonUnixUserInteraction [expr {
    [testConstraint userInteraction] ||
    ([testConstraint unix] && [testConstraint notAqua])
}]
testConstraint haveDISPLAY [expr {[info exists env(DISPLAY)] && [testConstraint x11]}]
testConstraint altDisplay  [info exists env(TK_ALT_DISPLAY)]
testConstraint noExceed [expr {
    ![testConstraint unix] || [catch {font actual "\{xyz"}]
}]

# constraints for testing facilities defined in the tktest executable...
testConstraint testImageType [expr {[lsearch [image types] test] >= 0}]
testConstraint testOldImageType [expr {[lsearch [image types] oldtest] >= 0}]
testConstraint testbitmap    [llength [info commands testbitmap]]
testConstraint testborder    [llength [info commands testborder]]
testConstraint testcbind     [llength [info commands testcbind]]
testConstraint testclipboard [llength [info commands testclipboard]]
testConstraint testcolor     [llength [info commands testcolor]]
testConstraint testcursor    [llength [info commands testcursor]]
testConstraint testembed     [llength [info commands testembed]]
testConstraint testfont      [llength [info commands testfont]]
testConstraint testmakeexist [llength [info commands testmakeexist]]
testConstraint testmenubar   [llength [info commands testmenubar]]
testConstraint testmetrics   [llength [info commands testmetrics]]
testConstraint testobjconfig [llength [info commands testobjconfig]]
testConstraint testsend      [llength [info commands testsend]]
testConstraint testtext      [llength [info commands testtext]]
testConstraint testwinevent  [llength [info commands testwinevent]]
testConstraint testwrapper   [llength [info commands testwrapper]]

# constraint to see what sort of fonts are available
testConstraint fonts 1
destroy .e
entry .e -width 0 -font {Helvetica -12} -bd 1 -highlightthickness 1
.e insert end a.bcd
if {([winfo reqwidth .e] != 37) || ([winfo reqheight .e] != 20)} {
    testConstraint fonts 0
}
destroy .e
destroy .t
text .t -width 80 -height 20 -font {Times -14} -bd 1
pack .t
.t insert end "This is\na dot."
update
set x [list [.t bbox 1.3] [.t bbox 2.5]]
destroy .t
if {![string match {{22 3 6 15} {31 18 [34] 15}} $x]} {
    testConstraint fonts 0
}
testConstraint textfonts [expr {
    [testConstraint fonts] || [tk windowingsystem] eq "win32"
}]

# constraints for the visuals available..
testConstraint pseudocolor8 [expr {
    ([catch {
	toplevel .t -visual {pseudocolor 8} -colormap new
    }] == 0) && ([winfo depth .t] == 8)
}]
destroy .t
testConstraint haveTruecolor24 [expr {
    [lsearch -exact [winfo visualsavailable .] {truecolor 24}] >= 0
}]
testConstraint haveGrayscale8 [expr {
    [lsearch -exact [winfo visualsavailable .] {grayscale 8}] >= 0
}]
testConstraint defaultPseudocolor8 [expr {
    ([winfo visual .] eq "pseudocolor") && ([winfo depth .] == 8)
}]

# constraint based on whether our display is secure
setupbg
set app [dobg {tk appname}]
testConstraint secureserver 0
if {[llength [info commands send]]} {
    testConstraint secureserver 1
    if {[catch {send $app set a 0} msg] == 1} {
        if {[string match "X server insecure *" $msg]} {
            testConstraint secureserver 0
	}
    }
}
cleanupbg

eval tcltest::configure $argv
namespace import -force tcltest::test
namespace import -force tcltest::makeFile
namespace import -force tcltest::removeFile
namespace import -force tcltest::makeDirectory
namespace import -force tcltest::removeDirectory
namespace import -force tcltest::interpreter
namespace import -force tcltest::testsDirectory
namespace import -force tcltest::cleanupTests

deleteWindows
wm geometry . {}
raise .

