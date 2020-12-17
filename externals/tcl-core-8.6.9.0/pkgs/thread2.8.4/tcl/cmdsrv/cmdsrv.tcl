#
# cmdsrv.tcl --
#
# Simple socket command server. Supports many simultaneous sessions.
# Works in thread mode with each new connection receiving a new thread.
#
# Usage:
#    cmdsrv::create port ?-idletime value? ?-initcmd cmd?
#
#    port         Tcp port where the server listens
#    -idletime    # of sec to idle before tearing down socket (def: 300 sec)
#    -initcmd     script to initialize new worker thread (def: empty)
#
# Example:
#
#    # tclsh8.4
#    % source cmdsrv.tcl
#    % cmdsrv::create 5000 -idletime 60
#    % vwait forever
#
#    Starts the server on the port 5000, sets idle timer to 1 minute.
#    You can now use "telnet" utility to connect.
#
# Copyright (c) 2002 by Zoran Vasiljevic.
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# -----------------------------------------------------------------------------

package require Tcl    8.4
package require Thread 2.5

namespace eval cmdsrv {
    variable data; # Stores global configuration options
}

#
# cmdsrv::create --
#
#	Start the server on the given Tcp port.
#
# Arguments:
#   port   Port where the server is listening
#   args   Variable number of arguments
#
# Side Effects:
#	None.
#
# Results:
#	None.
#

proc cmdsrv::create {port args} {

    variable data

    if {[llength $args] % 2} {
        error "wrong \# arguments, should be: key1 val1 key2 val2..."
    }

    #
    # Setup default pool data.
    #

    array set data {
        -idletime 300000
        -initcmd  {source cmdsrv.tcl}
    }

    #
    # Override with user-supplied data
    #

    foreach {arg val} $args {
        switch -- $arg {
            -idletime {set data($arg) [expr {$val*1000}]}
            -initcmd  {append data($arg) \n $val}
            default {
                error "unsupported pool option \"$arg\""
            }
        }
    }

    #
    # Start the server on the given port. Note that we wrap
    # the actual accept with a helper after/idle callback.
    # This is a workaround for a well-known Tcl bug.
    #

    socket -server [namespace current]::_Accept -myaddr 127.0.0.1 $port
}

#
# cmdsrv::_Accept --
#
#	Helper procedure to solve Tcl shared channel bug when responding
#   to incoming socket connection and transfering the channel to other
#   thread(s).
#
# Arguments:
#   s      incoming socket
#   ipaddr IP address of the remote peer
#   port   Tcp port used for this connection
#
# Side Effects:
#	None.
#
# Results:
#	None.
#

proc cmdsrv::_Accept {s ipaddr port} {
    after idle [list [namespace current]::Accept $s $ipaddr $port]
}

#
# cmdsrv::Accept --
#
#	Accepts the incoming socket connection, creates the worker thread.
#
# Arguments:
#   s      incoming socket
#   ipaddr IP address of the remote peer
#   port   Tcp port used for this connection
#
# Side Effects:
#	Creates new worker thread.
#
# Results:
#	None.
#

proc cmdsrv::Accept {s ipaddr port} {

    variable data

    #
    # Configure socket for sane operation
    #

    fconfigure $s -blocking 0 -buffering none -translation {auto crlf}

    #
    # Emit the prompt
    #

    puts -nonewline $s "% "

    #
    # Create worker thread and transfer socket ownership
    #

    set tid [thread::create [append data(-initcmd) \n thread::wait]]
    thread::transfer $tid $s ; # This flushes the socket as well

    #
    # Start event-loop processing in the remote thread
    #

    thread::send -async $tid [subst {
        array set [namespace current]::data {[array get data]}
        fileevent $s readable {[namespace current]::Read $s}
        proc exit args {[namespace current]::SockDone $s}
        [namespace current]::StartIdleTimer $s
    }]
}

#
# cmdsrv::Read --
#
#	Event loop procedure to read data from socket and collect the
#   command to execute. If the command read from socket is complete
#   it executes the command are prints the result back.
#
# Arguments:
#   s      incoming socket
#
# Side Effects:
#	None.
#
# Results:
#	None.
#

proc cmdsrv::Read {s} {

    variable data

    StopIdleTimer $s

    #
    # Cover client closing connection
    #

    if {[eof $s] || [catch {read $s} line]} {
        return [SockDone $s]
    }
    if {$line == "\n" || $line == ""} {
        if {[catch {puts -nonewline $s "% "}]} {
            return [SockDone $s]
        }
        return [StartIdleTimer $s]
    }

    #
    # Construct command line to eval
    #

    append data(cmd) $line
    if {[info complete $data(cmd)] == 0} {
        if {[catch {puts -nonewline $s "> "}]} {
            return [SockDone $s]
        }
        return [StartIdleTimer $s]
    }

    #
    # Run the command
    #

    catch {uplevel \#0 $data(cmd)} ret
    if {[catch {puts $s $ret}]} {
        return [SockDone $s]
    }
    set data(cmd) ""
    if {[catch {puts -nonewline $s "% "}]} {
        return [SockDone $s]
    }
    StartIdleTimer $s
}

#
# cmdsrv::SockDone --
#
#	Tears down the thread and closes the socket if the remote peer has
#   closed his side of the comm channel.
#
# Arguments:
#   s      incoming socket
#
# Side Effects:
#	Worker thread gets released.
#
# Results:
#	None.
#

proc cmdsrv::SockDone {s} {

    catch {close $s}
    thread::release
}

#
# cmdsrv::StopIdleTimer --
#
#	Cancel the connection idle timer.
#
# Arguments:
#   s      incoming socket
#
# Side Effects:
#	After event gets cancelled.
#
# Results:
#	None.
#

proc cmdsrv::StopIdleTimer {s} {

    variable data

    if {[info exists data(idleevent)]} {
        after cancel $data(idleevent)
        unset data(idleevent)
    }
}

#
# cmdsrv::StartIdleTimer --
#
#	Initiates the connection idle timer.
#
# Arguments:
#   s      incoming socket
#
# Side Effects:
#	After event gets posted.
#
# Results:
#	None.
#

proc cmdsrv::StartIdleTimer {s} {

    variable data

    set data(idleevent) \
        [after $data(-idletime) [list [namespace current]::SockDone $s]]
}

# EOF $RCSfile: cmdsrv.tcl,v $

# Emacs Setup Variables
# Local Variables:
# mode: Tcl
# indent-tabs-mode: nil
# tcl-basic-offset: 4
# End:

