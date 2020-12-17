#
#	$Id: ListNBK.tcl,v 1.3 2001/12/09 05:31:07 idiscovery Exp $
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "widget": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This program demonstrates the ListBoteBook widget, which is very similar
# to a NoteBook widget but uses an HList instead of page tabs to list the
# pages.

proc RunSample {w} {
    set top [frame $w.f -bd 1 -relief raised]
    set box [tixButtonBox $w.b -bd 1 -relief raised]

    pack $box -side bottom -fill both
    pack $top -side top -fill both -expand yes

    #----------------------------------------------------------------------
    # Create the ListNoteBook with nice icons
    #----------------------------------------------------------------------
    tixListNoteBook $top.n -ipadx 6 -ipady 6

    set img0 [tix getimage harddisk]
    set img1 [tix getimage network]

    $top.n subwidget hlist add hard_disk -itemtype imagetext \
	-image $img0 -text "Hard Disk" -under 0
    $top.n subwidget hlist add network   -itemtype imagetext \
	-image $img1 -text "Network"   -under 0

    $top.n add hard_disk 
    $top.n add network
    
    #
    # Create the widgets inside the two pages
    
    # We use these options to set the sizes of the subwidgets inside the
    # notebook, so that they are well-aligned on the screen.
    #
    set name [tixOptionName $w]
    option add *$name*TixControl*entry.width 10
    option add *$name*TixControl*label.width 18
    option add *$name*TixControl*label.anchor e

    set f [$top.n subwidget hard_disk]

    tixControl $f.a -value 12   -label "Access Time: "
    tixControl $f.w -value 400  -label "Write Throughput: "
    tixControl $f.r -value 400  -label "Read Throughput: "
    tixControl $f.c -value 1021 -label "Capacity: "
    pack $f.a $f.w $f.r $f.c  -side top -padx 20 -pady 2

    set f [$top.n subwidget network]

    tixControl $f.a -value 12   -label "Access Time: "
    tixControl $f.w -value 400  -label "Write Throughput: "
    tixControl $f.r -value 400  -label "Read Throughput: "
    tixControl $f.c -value 1021 -label "Capacity: "
    tixControl $f.u -value 10   -label "Users: "

    pack $f.a $f.w $f.r $f.c $f.u -side top -padx 20 -pady 2

    pack $top.n -expand yes -fill both -padx 5 -pady 5

    # Create the buttons
    #
    $box add ok     -text Ok     -command "destroy $w" -width 6
    $box add cancel -text Cancel -command "destroy $w" -width 6
}

#----------------------------------------------------------------------
# Start-up code
#----------------------------------------------------------------------

if {![info exists tix_demo_running]} {
    wm withdraw .
    set w .demo
    toplevel $w; wm transient $w ""
    RunSample $w
    bind $w <Destroy> exit
}
