#
# tpool.tcl --
#
# Tcl implementation of a threadpool paradigm in pure Tcl using
# the Tcl threading extension 2.5 (or higher).
#
# This file is for example purposes only. The efficient C-level
# threadpool implementation is already a part of the threading
# extension starting with 2.5 version. Both implementations have
# the same Tcl API so both can be used interchangeably. Goal of
# this implementation is to serve as an example of using the Tcl
# extension to implement some very common threading paradigms.
#
# Beware: with time, as improvements are made to the C-level
# implementation, this Tcl one might lag behind.
# Please consider this code as a working example only.
#
#
#
# Copyright (c) 2002 by Zoran Vasiljevic.
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# -----------------------------------------------------------------------------

package require Thread 2.5
set thisScript [info script]

namespace eval tpool {

    variable afterevent "" ; # Idle timer event for worker threads
    variable result        ; # Stores result from the worker thread
    variable waiter        ; # Waits for an idle worker thread
    variable jobsdone      ; # Accumulates results from worker threads

    #
    # Create shared array with a single element.
    # It is used for automatic pool handles creation.
    #

    set ns [namespace current]
    tsv::lock $ns {
        if {[tsv::exists $ns count] == 0} {
            tsv::set $ns count 0
        }
        tsv::set $ns count -1
    }
    variable thisScript [info script]
}

#
# tpool::create --
#
#   Creates instance of a thread pool.
#
# Arguments:
#   args Variable number of key/value arguments, as follows:
#
#        -minworkers  minimum # of worker threads (def:0)
#        -maxworkers  maximum # of worker threads (def:4)
#        -idletime    # of sec worker is idle before exiting (def:0 = never)
#        -initcmd     script used to initialize new worker thread
#        -exitcmd     script run at worker thread exit
#
# Side Effects:
#   Might create many new threads if "-minworkers" option is > 0.
#
# Results:
#   The id of the newly created thread pool. This id must be used
#   in all other tpool::* commands.
#

proc tpool::create {args} {

    variable thisScript

    #
    # Get next threadpool handle and create the pool array.
    #

    set usage "wrong \# args: should be \"[lindex [info level 1] 0]\
               ?-minworkers count? ?-maxworkers count?\
               ?-initcmd script? ?-exitcmd script?\
               ?-idletime seconds?\""

    set ns [namespace current]
    set tpid [namespace tail $ns][tsv::incr $ns count]

    tsv::lock $tpid {
        tsv::set $tpid name $tpid
    }

    #
    # Setup default pool data.
    #

    tsv::array set $tpid {
         thrworkers  ""
         thrwaiters  ""
         jobcounter  0
         refcounter  0
         numworkers  0
        -minworkers  0
        -maxworkers  4
        -idletime    0
        -initcmd    ""
        -exitcmd    ""
    }

    tsv::set $tpid -initcmd  "source $thisScript"

    #
    # Override with user-supplied data
    #

    if {[llength $args] % 2} {
        error $usage
    }

    foreach {arg val} $args {
        switch -- $arg {
            -minworkers -
            -maxworkers {tsv::set $tpid $arg $val}
            -idletime   {tsv::set $tpid $arg [expr {$val*1000}]}
            -initcmd    {tsv::append $tpid $arg \n $val}
            -exitcmd    {tsv::append $tpid $arg \n $val}
            default {
                error $usage
            }
        }
    }

    #
    # Start initial (minimum) number of worker threads.
    #

    for {set ii 0} {$ii < [tsv::set $tpid -minworkers]} {incr ii} {
        Worker $tpid
    }

    return $tpid
}

#
# tpool::names --
#
#   Returns list of currently created threadpools
#
# Arguments:
#   None.
#
# Side Effects:
#   None.
#
# Results
#   List of active threadpoool identifiers or empty if none found
#
#

proc tpool::names {} {
    tsv::names [namespace tail [namespace current]]*
}

#
# tpool::post --
#
#   Submits the new job to the thread pool. The caller might pass
#   the job in two modes: synchronous and asynchronous.
#   For the synchronous mode, the pool implementation will retain
#   the result of the passed script until the caller collects it
#   using the "thread::get" command.
#   For the asynchronous mode, the result of the script is ignored.
#
# Arguments:
#   args   Variable # of arguments with the following syntax:
#          tpool::post ?-detached? tpid script
#
#          -detached  flag to turn the async operation (ignore result)
#          tpid       the id of the thread pool
#          script     script to pass to the worker thread for execution
#
# Side Effects:
#   Depends on the passed script.
#
# Results:
#   The id of the posted job. This id is used later on to collect
#   result of the job and set local variables accordingly.
#   For asynchronously posted jobs, the return result is ignored
#   and this function returns empty result.
#

proc tpool::post {args} {

    #
    # Parse command arguments.
    #

    set ns [namespace current]
    set usage "wrong \# args: should be \"[lindex [info level 1] 0]\
               ?-detached? tpoolId script\""

    if {[llength $args] == 2} {
        set detached 0
        set tpid [lindex $args 0]
        set cmd  [lindex $args 1]
    } elseif {[llength $args] == 3} {
        if {[lindex $args 0] != "-detached"} {
            error $usage
        }
        set detached 1
        set tpid [lindex $args 1]
        set cmd  [lindex $args 2]
    } else {
        error $usage
    }

    #
    # Find idle (or create new) worker thread. This is relatively
    # a complex issue, since we must honour the limits about number
    # of allowed worker threads imposed to us by the caller.
    #

    set tid ""

    while {$tid == ""} {
        tsv::lock $tpid {
            set tid [tsv::lpop $tpid thrworkers]
            if {$tid == "" || [catch {thread::preserve $tid}]} {
                set tid ""
                tsv::lpush $tpid thrwaiters [thread::id] end
                if {[tsv::set $tpid numworkers]<[tsv::set $tpid -maxworkers]} {
                    Worker $tpid
                }
            }
        }
        if {$tid == ""} {
            vwait ${ns}::waiter
        }
    }

    #
    # Post the command to the worker thread
    #

    if {$detached} {
        set j ""
        thread::send -async $tid [list ${ns}::Run $tpid 0 $cmd]
    } else {
        set j [tsv::incr $tpid jobcounter]
        thread::send -async $tid [list ${ns}::Run $tpid $j $cmd] ${ns}::result
    }

    variable jobsdone
    set jobsdone($j) ""

    return $j
}

#
# tpool::wait --
#
#   Waits for jobs sent with "thread::post" to finish.
#
# Arguments:
#   tpid     Name of the pool shared array.
#   jobList  List of job id's done.
#   jobLeft  List of jobs still pending.
#
# Side Effects:
#   Might eventually enter the event loop while waiting
#   for the job result to arrive from the worker thread.
#   It ignores bogus job ids.
#
# Results:
#   Result of the job. If the job resulted in error, it sets
#   the global errorInfo and errorCode variables accordingly.
#

proc tpool::wait {tpid jobList {jobLeft ""}} {

    variable result
    variable jobsdone

    if {$jobLeft != ""} {
        upvar $jobLeft jobleft
    }

    set retlist ""
    set jobleft ""

    foreach j $jobList {
        if {[info exists jobsdone($j)] == 0} {
            continue ; # Ignore (skip) bogus job ids
        }
        if {$jobsdone($j) != ""} {
            lappend retlist $j
        } else {
            lappend jobleft $j
        }
    }
    if {[llength $retlist] == 0 && [llength $jobList]} {
        #
        # No jobs found; wait for the first one to get ready.
        #
        set jobleft $jobList
        while {1} {
            vwait [namespace current]::result
            set doneid [lindex $result 0]
            set jobsdone($doneid) $result
            if {[lsearch $jobList $doneid] >= 0} {
                lappend retlist $doneid
                set x [lsearch $jobleft $doneid]
                set jobleft [lreplace $jobleft $x $x]
                break
            }
        }
    }

    return $retlist
}

#
# tpool::get --
#
#   Waits for a job sent with "thread::post" to finish.
#
# Arguments:
#   tpid   Name of the pool shared array.
#   jobid  Id of the previously posted job.
#
# Side Effects:
#   None.
#
# Results:
#   Result of the job. If the job resulted in error, it sets
#   the global errorInfo and errorCode variables accordingly.
#

proc tpool::get {tpid jobid} {

    variable jobsdone

    if {[lindex $jobsdone($jobid) 1] != 0} {
        eval error [lrange $jobsdone($jobid) 2 end]
    }

    return [lindex $jobsdone($jobid) 2]
}

#
# tpool::preserve --
#
#   Increments the reference counter of the threadpool, reserving it
#   for the private usage..
#
# Arguments:
#   tpid   Name of the pool shared array.
#
# Side Effects:
#   None.
#
# Results:
#   Current number of threadpool reservations.
#

proc tpool::preserve {tpid} {
    tsv::incr $tpid refcounter
}

#
# tpool::release --
#
#   Decrements the reference counter of the threadpool, eventually
#   tearing the pool down if this was the last reservation.
#
# Arguments:
#   tpid   Name of the pool shared array.
#
# Side Effects:
#   If the number of reservations drops to zero or below
#   the threadpool is teared down.
#
# Results:
#   Current number of threadpool reservations.
#

proc tpool::release {tpid} {

    tsv::lock $tpid {
        if {[tsv::incr $tpid refcounter -1] <= 0} {
            # Release all workers threads
            foreach t [tsv::set $tpid thrworkers] {
                thread::release -wait $t
            }
            tsv::unset $tpid ; # This is not an error; it works!
        }
    }
}

#
# Private procedures, not a part of the threadpool API.
#

#
# tpool::Worker --
#
#   Creates new worker thread. This procedure must be executed
#   under the tsv lock.
#
# Arguments:
#   tpid  Name of the pool shared array.
#
# Side Effects:
#   Depends on the thread initialization script.
#
# Results:
#   None.
#

proc tpool::Worker {tpid} {

    #
    # Create new worker thread
    #

    set tid [thread::create]

    thread::send $tid [tsv::set $tpid -initcmd]
    thread::preserve $tid

    tsv::incr  $tpid numworkers
    tsv::lpush $tpid thrworkers $tid

    #
    # Signalize waiter threads if any
    #

    set waiter [tsv::lpop $tpid thrwaiters]
    if {$waiter != ""} {
        thread::send -async $waiter [subst {
            set [namespace current]::waiter 1
        }]
    }
}

#
# tpool::Timer --
#
#   This procedure should be executed within the worker thread only.
#   It registers the callback for terminating the idle thread.
#
# Arguments:
#   tpid  Name of the pool shared array.
#
# Side Effects:
#   Thread may eventually exit.
#
# Results:
#   None.
#

proc tpool::Timer {tpid} {

    tsv::lock $tpid {
        if {[tsv::set $tpid  numworkers] > [tsv::set $tpid -minworkers]} {

            #
            # We have more workers than needed, so kill this one.
            # We first splice ourselves from the list of active
            # workers, adjust the number of workers and release
            # this thread, which may exit eventually.
            #

            set x [tsv::lsearch $tpid thrworkers [thread::id]]
            if {$x >= 0} {
                tsv::lreplace $tpid thrworkers $x $x
                tsv::incr $tpid numworkers -1
                set exitcmd [tsv::set $tpid -exitcmd]
                if {$exitcmd != ""} {
                    catch {eval $exitcmd}
                }
                thread::release
            }
        }
    }
}

#
# tpool::Run --
#
#   This procedure should be executed within the worker thread only.
#   It performs the actual command execution in the worker thread.
#
# Arguments:
#   tpid  Name of the pool shared array.
#   jid   The job id
#   cmd   The command to execute
#
# Side Effects:
#   Many, depending of the passed command
#
# Results:
#   List for passing the evaluation result and status back.
#

proc tpool::Run {tpid jid cmd} {

    #
    # Cancel the idle timer callback, if any.
    #

    variable afterevent
    if {$afterevent != ""} {
        after cancel $afterevent
    }

    #
    # Evaluate passed command and build the result list.
    #

    set code [catch {uplevel \#0 $cmd} ret]
    if {$code == 0} {
        set res [list $jid 0 $ret]
    } else {
        set res [list $jid $code $ret $::errorInfo $::errorCode]
    }

    #
    # Check to see if any caller is waiting to be serviced.
    # If yes, kick it out of the waiting state.
    #

    set ns [namespace current]

    tsv::lock $tpid {
        tsv::lpush $tpid thrworkers [thread::id]
        set waiter [tsv::lpop $tpid thrwaiters]
        if {$waiter != ""} {
            thread::send -async $waiter [subst {
                set ${ns}::waiter 1
            }]
        }
    }

    #
    # Release the thread. If this turns out to be
    # the last refcount held, don't bother to do
    # any more work, since thread will soon exit.
    #

    if {[thread::release] <= 0} {
        return $res
    }

    #
    # Register the idle timer again.
    #

    if {[set idle [tsv::set $tpid -idletime]]} {
        set afterevent [after $idle [subst {
            ${ns}::Timer $tpid
        }]]
    }

    return $res
}

# EOF $RCSfile: tpool.tcl,v $

# Emacs Setup Variables
# Local Variables:
# mode: Tcl
# indent-tabs-mode: nil
# tcl-basic-offset: 4
# End:

