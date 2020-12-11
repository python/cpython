# httpTestScript.tcl
#
#	Test HTTP/1.1 concurrent requests including
#	queueing, pipelining and retries.
#
# Copyright (C) 2018 Keith Nash <kjnash@users.sourceforge.net>
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

# ------------------------------------------------------------------------------
# "Package" httpTestScript for executing test scripts written in a convenient
# shorthand.
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Documentation for "package" httpTestScript.
# ------------------------------------------------------------------------------
# To use the package:
# (a) define URLs as the values of elements in the array ::httpTestScript
# (b) define a script in terms of the commands
#         START STOP DELAY KEEPALIVE WAIT PIPELINE GET HEAD POST
#     referring to URLs by the name of the corresponding array element.  The
#     script can include any other Tcl commands, and evaluates in the
#     httpTestScript namespace.
# (c) Use the command httpTestScript::runHttpTestScript to evaluate the script.
# (d) For tcltest tests, wrap the runHttpTestScript call in a suitable "test"
#     command.
# ------------------------------------------------------------------------------
# START
# Must be the first command of the script.
#
# STOP
# Must be present in the script to avoid waiting for client timeout.
# Usually the last command, but can be elsewhere to end a script prematurely.
# Subsequent httpTestScript commands will have no effect.
#
# DELAY ms
# If there are no WAIT commands, this sets the delay in ms between subsequent
# calls to http::geturl.  Default 500ms.
#
# KEEPALIVE
# Set the value passed to http::geturl for the -keepalive option.  The command
# applies to subsequent requests in the script. Default 1.
#
# WAIT ms
# Pause for a time in ms before sending subsequent requests.
#
# PIPELINE boolean
# Set the value of -pipeline using http::config.  The last PIPELINE command
# in the script applies to every request. Default 1.
#
# POSTFRESH boolean
# Set the value of -postfresh using http::config.  The last POSTFRESH command
# in the script applies to every request. Default 0.
#
# REPOST boolean
# Set the value of -repost using http::config.  The last REPOST command
# in the script applies to every request. Default 1 for httpTestScript.
# (Default value in http is 0).
#
# GET uriCode ?arg ...?
# Send a HTTP request using the GET method.
# Arguments:
# uriCode - the code for the base URI - the value must be stored in
#           ::httpTestScript::URL($uriCode).
# args    - strings that will be joined by "&" and appended to the query
#           string with a preceding "&".
#
# HEAD uriCode ?arg ...?
# Send a HTTP request using the HEAD method.
# Arguments: as for GET
#
# POST uriCode ?arg ...?
# Send a HTTP request using the POST method.
# Arguments:
# uriCode - the code for the base URI - the value must be stored in
#           ::httpTestScript::URL($uriCode).
# args    - strings that will be joined by "&" and used as the request body.
# ------------------------------------------------------------------------------

namespace eval ::httpTestScript {
    namespace export runHttpTestScript cleanupHttpTestScript
}

# httpTestScript::START --
# Initialise, and create a long-stop timeout.

proc httpTestScript::START {} {
    variable CountRequestedSoFar
    variable RequestsWhenStopped
    variable KeepAlive
    variable Delay
    variable TimeOutCode
    variable TimeOutDone
    variable StartDone
    variable StopDone
    variable CountFinishedSoFar
    variable RequestList
    variable RequestsMade
    variable ExtraTime
    variable ActualKeepAlive

    if {[info exists StartDone] && ($StartDone == 1)} {
        set msg {START has been called twice without an intervening STOP}
        return -code error $msg
    }

    set StartDone 1
    set StopDone 0
    set TimeOutDone 0
    set CountFinishedSoFar 0
    set CountRequestedSoFar 0
    set RequestList {}
    set RequestsMade {}
    set ExtraTime 0
    set ActualKeepAlive 1

    # Undefined until a STOP command:
    unset -nocomplain RequestsWhenStopped

    # Default values:
    set KeepAlive 1
    set Delay 500

    # Default values for tests:
    KEEPALIVE 1
    PIPELINE  1
    POSTFRESH 0
    REPOST    1

    set TimeOutCode [after 30000 httpTestScript::TimeOutNow]
#    set TimeOutCode [after 4000 httpTestScript::TimeOutNow]
    return
}

# httpTestScript::STOP --
# Do not process any more commands.  The commands will be executed but will
# silently do nothing.

proc httpTestScript::STOP {} {
    variable CountRequestedSoFar
    variable CountFinishedSoFar
    variable RequestsWhenStopped
    variable TimeOutCode
    variable StartDone
    variable StopDone
    variable RequestsMade

    if {$StopDone} {
        # Don't do anything on a second call.
        return
    }

    if {![info exists StartDone]} {
        return -code error {initialise the script by calling command START}
    }

    set StopDone 1
    set StartDone 0
    set RequestsWhenStopped $CountRequestedSoFar
    unset -nocomplain StartDone

    if {$CountFinishedSoFar == $RequestsWhenStopped} {
        if {[info exists TimeOutCode]} {
            after cancel $TimeOutCode
        }
        set ::httpTestScript::FOREVER 0
    }
    return
}

# httpTestScript::DELAY --
# If there are no WAIT commands, this sets the delay in ms between subsequent
# calls to http::geturl.  Default 500ms.

proc httpTestScript::DELAY {t} {
    variable StartDone
    variable StopDone

    if {$StopDone} {
        return
    }

    if {![info exists StartDone]} {
        return -code error {initialise the script by calling command START}
    }

    variable Delay

    set Delay $t
    return
}

# httpTestScript::KEEPALIVE --
# Set the value passed to http::geturl for the -keepalive option.  Default 1.

proc httpTestScript::KEEPALIVE {b} {
    variable StartDone
    variable StopDone

    if {$StopDone} {
        return
    }

    if {![info exists StartDone]} {
        return -code error {initialise the script by calling command START}
    }

    variable KeepAlive
    set KeepAlive $b
    return
}

# httpTestScript::WAIT --
# Pause for a time in ms before processing any more commands.

proc httpTestScript::WAIT {t} {
    variable StartDone
    variable StopDone
    variable ExtraTime

    if {$StopDone} {
        return
    }

    if {![info exists StartDone]} {
        return -code error {initialise the script by calling command START}
    }

    if {(![string is integer -strict $t]) || $t < 0} {
        return -code error {argument to WAIT must be a non-negative integer}
    }

    incr ExtraTime $t

    return
}

# httpTestScript::PIPELINE --
# Pass a value to http::config -pipeline.

proc httpTestScript::PIPELINE {b} {
    variable StartDone
    variable StopDone

    if {$StopDone} {
        return
    }

    if {![info exists StartDone]} {
        return -code error {initialise the script by calling command START}
    }

    ::http::config -pipeline $b
    ##::http::Log http(-pipeline) is now [::http::config -pipeline]
    return
}

# httpTestScript::POSTFRESH --
# Pass a value to http::config -postfresh.

proc httpTestScript::POSTFRESH {b} {
    variable StartDone
    variable StopDone

    if {$StopDone} {
        return
    }

    if {![info exists StartDone]} {
        return -code error {initialise the script by calling command START}
    }

    ::http::config -postfresh $b
    ##::http::Log http(-postfresh) is now [::http::config -postfresh]
    return
}

# httpTestScript::REPOST --
# Pass a value to http::config -repost.

proc httpTestScript::REPOST {b} {
    variable StartDone
    variable StopDone

    if {$StopDone} {
        return
    }

    if {![info exists StartDone]} {
        return -code error {initialise the script by calling command START}
    }

    ::http::config -repost $b
    ##::http::Log http(-repost) is now [::http::config -repost]
    return
}

# httpTestScript::GET --
# Send a HTTP request using the GET method.
# Arguments:
# uriCode - the code for the base URI - the value must be stored in
#           ::httpTestScript::URL($uriCode).
# args    - strings that will each be preceded by "&" and appended to the query
#           string.

proc httpTestScript::GET {uriCode args} {
    variable RequestList
    lappend RequestList GET
    RequestAfter $uriCode 0 {} {*}$args
    return
}

# httpTestScript::HEAD --
# Send a HTTP request using the HEAD method.
# Arguments: as for GET

proc httpTestScript::HEAD {uriCode args} {
    variable RequestList
    lappend RequestList HEAD
    RequestAfter $uriCode 1 {} {*}$args
    return
}

# httpTestScript::POST --
# Send a HTTP request using the POST method.
# Arguments:
# uriCode - the code for the base URI - the value must be stored in
#           ::httpTestScript::URL($uriCode).
# args    - strings that will be joined by "&" and used as the request body.

proc httpTestScript::POST {uriCode args} {
    variable RequestList
    lappend RequestList POST
    RequestAfter $uriCode 0 {use} {*}$args
    return
}


proc httpTestScript::RequestAfter {uriCode validate query args} {
    variable CountRequestedSoFar
    variable Delay
    variable ExtraTime
    variable StartDone
    variable StopDone
    variable KeepAlive

    if {$StopDone} {
        return
    }

    if {![info exists StartDone]} {
        return -code error {initialise the script by calling command START}
    }

    incr CountRequestedSoFar
    set idelay [expr {($CountRequestedSoFar - 1) * $Delay + 10 + $ExtraTime}]

    # Could pass values of -pipeline, -postfresh, -repost if it were
    # useful to change these mid-script.
    after $idelay [list httpTestScript::Requester $uriCode $KeepAlive $validate $query {*}$args]
    return
}

proc httpTestScript::Requester {uriCode keepAlive validate query args} {
    variable URL

    ::http::config -accept {*/*}

    set absUrl $URL($uriCode)
    if {$query eq {}} {
	if {$args ne {}} {
	    append absUrl & [join $args &]
	}
	set queryArgs {}
    } elseif {$validate} {
        return -code error {cannot have both -validate (HEAD) and -query (POST)}
    } else {
	set queryArgs [list -query [join $args &]]
    }

    if {[catch {
        ::http::geturl     $absUrl        \
                -validate  $validate      \
                -timeout   10000          \
                {*}$queryArgs             \
                -keepalive $keepAlive     \
                -command   ::httpTestScript::WhenFinished
    } token]} {
        set msg $token
        catch {puts stdout "Error: $msg"}
        return
    } else {
        # Request will begin.
    }

    return

}

proc httpTestScript::TimeOutNow {} {
    variable TimeOutDone

    set TimeOutDone 1
    set ::httpTestScript::FOREVER 0
    return
}

proc httpTestScript::WhenFinished {hToken} {
    variable CountFinishedSoFar
    variable RequestsWhenStopped
    variable TimeOutCode
    variable StopDone
    variable RequestList
    variable RequestsMade
    variable ActualKeepAlive

    upvar #0 $hToken state

    if {[catch {
	if {    [info exists state(transfer)]
	     && ($state(transfer) eq "chunked")
	} {
	    set Trans chunked
	} else {
	    set Trans unchunked
	}

	if {    [info exists ::httpTest::testOptions(-verbose)]
	     && ($::httpTest::testOptions(-verbose) > 0)
	} {
	    puts "Token    $hToken
Response $state(http)
Status   $state(status)
Method   $state(method)
Transfer $Trans
Size     $state(currentsize)
URL      $state(url)
"
	}

	if {!$state(-keepalive)} {
	    set ActualKeepAlive 0
	}

	if {[info exists state(method)]} {
	    lappend RequestsMade $state(method)
	} else {
	    lappend RequestsMade UNKNOWN
	}
	set tk [namespace tail $hToken]

	if {    ($state(http) != {HTTP/1.1 200 OK})
	     || ($state(status) != {ok})
	     || (($state(currentsize) == 0) && ($state(method) ne "HEAD"))
	} {
	    ::http::Log ^X$tk unexpected result Response $state(http) Status $state(status) Size $state(currentsize) - token $hToken
	}
    } err]} {
	::http::Log ^X$tk httpTestScript::WhenFinished failed with error status: $err - token $hToken
    }

    incr CountFinishedSoFar
    if {$StopDone && ($CountFinishedSoFar == $RequestsWhenStopped)} {
        if {[info exists TimeOutCode]} {
            after cancel $TimeOutCode
        }
        if {$RequestsMade ne $RequestList && $ActualKeepAlive} {
	    ::http::Log ^X$tk unexpected result - Script asked for "{$RequestList}" but got "{$RequestsMade}" - token $hToken
        }
        set ::httpTestScript::FOREVER 0
    }

    return
}


proc httpTestScript::runHttpTestScript {scr} {
    variable TimeOutDone
    variable RequestsWhenStopped

    after idle [list namespace eval ::httpTestScript $scr]
    vwait ::httpTestScript::FOREVER
    # N.B. does not automatically execute in this namespace, unlike some other events.
    # Release when all requests have been served or have timed out.

    if {$TimeOutDone} {
        return -code error {test script timed out}
    }

    return $RequestsWhenStopped
}


proc httpTestScript::cleanupHttpTestScript {} {
    variable TimeOutDone
    variable RequestsWhenStopped

    if {![info exists RequestsWhenStopped]} {
	return -code error {Cleanup Failed: RequestsWhenStopped is undefined}
    }

    for {set i 1} {$i <= $RequestsWhenStopped} {incr i} {
        http::cleanup ::http::$i
    }

    return
}
