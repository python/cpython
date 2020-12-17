# -*- mode: TCL; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: CaseData.tcl,v 1.4 2004/03/28 02:44:57 hobbs Exp $
#
# CaseData.tcl --
#
#	Contains data for test cases
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

# GetHomeDirs --
#
#	Returns a list of user names (prefixed with tilde) and their
#	home directories
#
proc GetHomeDirs {} {
    set tryList {root ftp admin operator man john ioi}
    if [catch {
	lappend tryList [exec whoami]
    }] {
	catch {
	    lappend tryList [exec logname]
	}
    }
	    

    set list {}
    foreach user $tryList {
	if [info exists done($user)] {
	    continue
	}
	set expanded [tixFile tilde ~$user]
	if ![tixStrEq $expanded ~$user] {
	    lappend list [list ~$user $expanded]
	}
	set done($user) 1
    }
    return $list
}

# GetCases_FsNormDir --
#
#	Returns a set of test cases for verifying whether a non-normalized
#	directory is properly notmalized
#
proc GetCases_FsNormDir {} {

    if [tixStrEq [tix platform] unix] {
	#   PATHNAME to TEST		expected result   Causes error for
	#						    file normalize?
	#----------------------------------------------------------------
	set list {
	    {.					""		1}
	    {foo				""		1}
	    {~nosuchuser			""		1}
	    {~nosuchuser/../			""		1}
	    {/					/		0}
	    {///				/		0}
	    {/./				/		0}
	    {/./.				/		0}
	    {/./.				/		0}
	    {/././.././../			/		0}
	    {/etc				/etc		0}
	    {/etc/../etc			/etc		0}
	    {/etc/../etc/./			/etc		0}
	    {/etc/../etc/./			/etc		0}
	    {/etc/../usr/./lib			/usr/lib	0}
	}
	foreach userInfo [GetHomeDirs] {
	    lappend list [list [lindex $userInfo 0] [lindex $userInfo 1] 0]
	}
    } else {
	set list [list \
	    [list .				""			1] \
	    [list foo				""			1] \
	    [list ..				""			1] \
	    [list ..\\foo			""			1] \
	    [list ..\\dat\\.			""			1] \
	    [list C:				""			1] \
	    [list C:\\				C:			0] \
	    [list c:\\				C:			0] \
	    [list C:\\\\			C:			0] \
	    [list C:\\				C:			0] \
	    [list C:\\.				C:			0] \
	    [list C:\\Windows			C:\\Windows		0] \
	    [list C:\\Windows\\System		C:\\Windows\\System	0] \
	    [list C:\\Windows\\..		C:			0] \
	]
    }

    return $list
}

# GetCases_FSNorm --
#
#	Returns a set of test cases for testing the tixFSNorm command.
#
proc GetCases_FSNorm {} {
    global tixPriv

    if [tixStrEq [tix platform] unix] {
	#   PATHNAME to TEST		context    <----------  Expected Result ----------------------------------->
	#					       path	       vpath(todo)     files(todo)   patterns(todo)
	#----------------------------------------------------------------
	set list {
	    {.				/		/		}
	    {./				/		/		}
	    {./////./			/ 		/		}
	    {..				/		/		}
	    {../			/		/		}
	    {../..			/		/		}
	    {../../../			/		/		}
	    {/etc			/		/etc		}
	    {/etc///../etc///		/		/etc		}
	    {/etc///../etc///..		/		/		}
	    {/etc///../etc///../	/		/		}
	    {/etc/.			/		/etc		}
	    {/./etc/.			/		/etc		}
	    {/./././etc/.		/		/etc		}
	    {/usr/./././local/./lib////	/		/usr/local/lib	}
	    {./././././etc/		/		/etc		}
	    {/etc/../etc		/		/etc		}
	    {/etc/../etc/../etc		/		/etc		}
	    {/etc/../etc/../		/		/		}
	    {~foobar/foo		/		/~foobar	}
	    {~foobar/foo/		/		/~foobar/foo	}
	}
    } else {
	set p $tixPriv(WinPrefix)

	set list [list \
	    [list .			$p\\C:		$p\\C:			] \
	    [list .\\.			$p\\C:		$p\\C:			] \
	    [list .\\Windows		$p\\C:		$p\\C:\\Windows		] \
	    [list .\\Windows\\..\\	$p\\C:		$p\\C:			] \
	    [list tmp\\			$p\\C:		$p\\C:\\tmp		] \
	    [list "no such file"	$p\\C:		$p\\C:			] \
	    [list "autoexec.bat"	$p\\C:		$p\\C:			] \
	    [list "ignore/slash\\dd"	$p\\C:		$p\\C:\\ignore/slash	] \
	    [list "has space\\"		$p\\C:		"$p\\C:\\has space"	] \
	    [list "has space"		$p\\C:		"$p\\C:"		] \
        ]
	# ToDo:
	#	(1) xx\xx\C: + .. should be xx\xx
	#	(2) xx\xx\C: + D: should be xx\xx\D:
    }
    return $list
}
