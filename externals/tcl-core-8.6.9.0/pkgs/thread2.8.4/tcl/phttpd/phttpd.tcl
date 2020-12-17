#
# phttpd.tcl --
#
# Simple Sample httpd/1.0 server in 250 lines of Tcl.
# Stephen Uhler / Brent Welch (c) 1996 Sun Microsystems.
#
# Modified to use namespaces, direct url-to-procedure access
# and thread pool package. Grown little larger since ;)
#
# Usage:
#    phttpd::create port
#
#    port         Tcp port where the server listens
#
# Example:
#
#    # tclsh8.4
#    % source phttpd.tcl
#    % phttpd::create 5000
#    % vwait forever
#
#    Starts the server on the port 5000. Also, look at the Httpd array
#    definition in the "phttpd" namespace declaration to find out
#    about other options you may put on the command line.
#
#    You can use: http://localhost:5000/monitor URL to test the
#    server functionality.
#
# Copyright (c) 2002 by Zoran Vasiljevic.
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# -----------------------------------------------------------------------------

package require Tcl    8.4
package require Thread 2.5

#
# Modify the following in order to load the
# example Tcl implementation of threadpools.
# Per default, the C-level threadpool is used.
#

if {0} {
    eval [set TCL_TPOOL {source ../tpool/tpool.tcl}]
}

namespace eval phttpd {

    variable Httpd;           # Internal server state and config params
    variable MimeTypes;       # Cache of file-extension/mime-type
    variable HttpCodes;       # Portion of well-known http return codes
    variable ErrorPage;       # Format of error response page in html

    array set Httpd {
        -name  phttpd
        -vers  1.0
        -root  "."
        -index index.htm
    }
    array set HttpCodes {
        400  "Bad Request"
        401  "Not Authorized"
        404  "Not Found"
        500  "Server error"
    }
    array set MimeTypes {
        {}   "text/plain"
        .txt "text/plain"
        .htm "text/html"
        .htm "text/html"
        .gif "image/gif"
        .jpg "image/jpeg"
        .png "image/png"
    }
    set ErrorPage {
        <title>Error: %1$s %2$s</title>
        <h1>%3$s</h1>
        <p>Problem in accessing "%4$s" on this server.</p>
        <hr>
        <i>%5$s/%6$s Server at %7$s Port %8$s</i>
    }
}

#
# phttpd::create --
#
#	Start the server by listening for connections on the desired port.
#
# Arguments:
#   port
#   args
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::create {port args} {

    variable Httpd

    set arglen [llength $args]
    if {$arglen} {
        if {$arglen % 2} {
            error "wrong \# args, should be: key1 val1 key2 val2..."
        }
        set opts [array names Httpd]
        foreach {arg val} $args {
            if {[lsearch $opts $arg] == -1} {
                error "unknown option \"$arg\""
            }
            set Httpd($arg) $val
        }
    }

    #
    # Create thread pool with max 8 worker threads.
    #

    if {[info exists ::TCL_TPOOL] == 0} {
        #
        # Using the internal C-based thread pool
        #
        set initcmd "source ../phttpd/phttpd.tcl"
    } else {
        #
        # Using the Tcl-level hand-crafted thread pool
        #
        append initcmd "source ../phttpd/phttpd.tcl" \n $::TCL_TPOOL
    }

    set Httpd(tpid) [tpool::create -maxworkers 8 -initcmd $initcmd]

    #
    # Start the server on the given port. Note that we wrap
    # the actual accept with a helper after/idle callback.
    # This is a workaround for a well-known Tcl bug.
    #

    socket -server [namespace current]::_Accept $port
}

#
# phttpd::_Accept --
#
#	Helper procedure to solve Tcl shared-channel bug when responding
#   to incoming connection and transfering the channel to other thread(s).
#
# Arguments:
#   sock   incoming socket
#   ipaddr IP address of the remote peer
#   port   Tcp port used for this connection
#
# Side Effects:
#	None.
#
# Results:
#	None.
#

proc phttpd::_Accept {sock ipaddr port} {
    after idle [list [namespace current]::Accept $sock $ipaddr $port]
}

#
# phttpd::Accept --
#
#	Accept a new connection from the client.
#
# Arguments:
#   sock
#   ipaddr
#   port
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::Accept {sock ipaddr port} {

    variable Httpd

    #
    # Setup the socket for sane operation
    #

    fconfigure $sock -blocking 0 -translation {auto crlf}

    #
    # Detach the socket from current interpreter/tnread.
    # One of the worker threads will attach it again.
    #

    thread::detach $sock

    #
    # Send the work ticket to threadpool.
    #

    tpool::post -detached $Httpd(tpid) [list [namespace current]::Ticket $sock]
}

#
# phttpd::Ticket --
#
#	Job ticket to run in the thread pool thread.
#
# Arguments:
#   sock
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::Ticket {sock} {

    thread::attach $sock
    fileevent $sock readable [list [namespace current]::Read $sock]

    #
    # End of processing is signalized here.
    # This will release the worker thread.
    #

    vwait [namespace current]::done
}


#
# phttpd::Read --
#
#	Read data from client and parse incoming http request.
#
# Arguments:
#   sock
#
# Side Effects:
#	None.
#
# Results:
#	None.
#

proc phttpd::Read {sock} {

    variable Httpd
    variable data

    set data(sock) $sock

    while {1} {
        if {[catch {gets $data(sock) line} readCount] || [eof $data(sock)]} {
            return [Done]
        }
        if {![info exists data(state)]} {
            set pat {(POST|GET) ([^?]+)\??([^ ]*) HTTP/1\.[0-9]}
            if {[regexp $pat $line x data(proto) data(url) data(query)]} {
                set data(state) mime
                continue
            } else {
                Log error "bad request line: (%s)" $line
                Error 400
                return [Done]
            }
        }

        # string compare $readCount 0 maps -1 to -1, 0 to 0, and > 0 to 1

        set state [string compare $readCount 0],$data(state),$data(proto)
        switch -- $state {
            "0,mime,GET" - "0,query,POST" {
                Respond
                return [Done]
            }
            "0,mime,POST" {
                set data(state) query
                set data(query) ""
            }
            "1,mime,POST" - "1,mime,GET" {
                if [regexp {([^:]+):[   ]*(.*)}  $line dummy key value] {
                    set data(mime,[string tolower $key]) $value
                }
            }
            "1,query,POST" {
                append data(query) $line
                set clen $data(mime,content-length)
                if {($clen - [string length $data(query)]) <= 0} {
                    Respond
                    return [Done]
                }
            }
            default {
                if [eof $data(sock)] {
                    Log error "unexpected eof; client closed connection"
                    return [Done]
                } else {
                    Log error "bad http protocol state: %s" $state
                    Error 400
                    return [Done]
                }
            }
        }
    }
}

#
# phttpd::Done --
#
#	Close the connection socket
#
# Arguments:
#   s
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::Done {} {

    variable done
    variable data

    close $data(sock)

    if {[info exists data]} {
        unset data
    }

    set done 1 ; # Releases the request thread (See Ticket procedure)
}

#
# phttpd::Respond --
#
#	Respond to the query.
#
# Arguments:
#   s
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::Respond {} {

    variable data

    if {[info commands $data(url)] == $data(url)} {

        #
        # Service URL-procedure
        #

        if {[catch {
            puts $data(sock) "HTTP/1.0 200 OK"
            puts $data(sock) "Date: [Date]"
            puts $data(sock) "Last-Modified: [Date]"
        } err]} {
            Log error "client closed connection prematurely: %s" $err
            return
        }
        if {[catch {$data(url) data} err]} {
            Log error "%s: %s" $data(url) $err
        }

    } else {

        #
        # Service regular file path
        #

        set mypath [Url2File $data(url)]
        if {![catch {open $mypath} i]} {
            if {[catch {
                puts $data(sock) "HTTP/1.0 200 OK"
                puts $data(sock) "Date: [Date]"
                puts $data(sock) "Last-Modified: [Date [file mtime $mypath]]"
                puts $data(sock) "Content-Type: [ContentType $mypath]"
                puts $data(sock) "Content-Length: [file size $mypath]"
                puts $data(sock) ""
                fconfigure $data(sock) -translation binary -blocking 0
                fconfigure $i          -translation binary
                fcopy $i $data(sock)
                close $i
            } err]} {
                Log error "client closed connection prematurely: %s" $err
            }
        } else {
            Log error "%s: %s" $data(url) $i
            Error 404
        }
    }
}

#
# phttpd::ContentType --
#
#	Convert the file suffix into a mime type.
#
# Arguments:
#   path
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::ContentType {path} {

    # @c Convert the file suffix into a mime type.

    variable MimeTypes

    set type "text/plain"
    catch {set type $MimeTypes([file extension $path])}

    return $type
}

#
# phttpd::Error --
#
#	Emit error page
#
# Arguments:
#   s
#   code
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::Error {code} {

    variable Httpd
    variable HttpCodes
    variable ErrorPage
    variable data

    append data(url) ""
    set msg \
        [format $ErrorPage     \
             $code             \
             $HttpCodes($code) \
             $HttpCodes($code) \
             $data(url)        \
             $Httpd(-name)     \
             $Httpd(-vers)     \
             [info hostname]   \
             80                \
            ]
    if {[catch {
        puts $data(sock) "HTTP/1.0 $code $HttpCodes($code)"
        puts $data(sock) "Date: [Date]"
        puts $data(sock) "Content-Length: [string length $msg]"
        puts $data(sock) ""
        puts $data(sock) $msg
    } err]} {
        Log error "client closed connection prematurely: %s" $err
    }
}

#
# phttpd::Date --
#
#	Generate a date string in HTTP format.
#
# Arguments:
#   seconds
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::Date {{seconds 0}} {

    # @c Generate a date string in HTTP format.

    if {$seconds == 0} {
        set seconds [clock seconds]
    }
    clock format $seconds -format {%a, %d %b %Y %T %Z} -gmt 1
}

#
# phttpd::Log --
#
#	Log an httpd transaction.
#
# Arguments:
#   reason
#   format
#   args
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::Log {reason format args} {

    set messg [eval format [list $format] $args]
    set stamp [clock format [clock seconds] -format "%d/%b/%Y:%H:%M:%S"]

    puts stderr "\[$stamp\]\[-thread[thread::id]-\] $reason: $messg"
}

#
# phttpd::Url2File --
#
#	Convert a url into a pathname.
#
# Arguments:
#   url
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::Url2File {url} {

    variable Httpd

    lappend pathlist $Httpd(-root)
    set level 0

    foreach part [split $url /] {
        set part [CgiMap $part]
        if [regexp {[:/]} $part] {
            return ""
        }
        switch -- $part {
            "." { }
            ".." {incr level -1}
            default {incr level}
        }
        if {$level <= 0} {
            return ""
        }
        lappend pathlist $part
    }

    set file [eval file join $pathlist]

    if {[file isdirectory $file]} {
        return [file join $file $Httpd(-index)]
    } else {
        return $file
    }
}

#
# phttpd::CgiMap --
#
#	Decode url-encoded strings.
#
# Arguments:
#   data
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::CgiMap {data} {

    regsub -all {\+} $data { } data
    regsub -all {([][$\\])} $data {\\\1} data
    regsub -all {%([0-9a-fA-F][0-9a-fA-F])} $data {[format %c 0x\1]} data

    return [subst $data]
}

#
# phttpd::QueryMap --
#
#	Decode url-encoded query into key/value pairs.
#
# Arguments:
#   query
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc phttpd::QueryMap {query} {

    set res [list]

    regsub -all {[&=]} $query { }    query
    regsub -all {  }   $query { {} } query; # Othewise we lose empty values

    foreach {key val} $query {
        lappend res [CgiMap $key] [CgiMap $val]
    }
    return $res
}

#
# monitor --
#
#	Procedure used to test the phttpd server. It responds on the
#        http://<hostname>:<port>/monitor
#
# Arguments:
#   array
#
# Side Effects:
#	None..
#
# Results:
#	None.
#

proc /monitor {array} {

    upvar $array data ; # Holds the socket to remote client

    #
    # Emit headers
    #

    puts $data(sock) "HTTP/1.0 200 OK"
    puts $data(sock) "Date: [phttpd::Date]"
    puts $data(sock) "Content-Type: text/html"
    puts $data(sock) ""

    #
    # Emit body
    #

    puts $data(sock) [subst {
        <html>
        <body>
        <h3>[clock format [clock seconds]]</h3>
    }]

    after 1 ; # Simulate blocking call

    puts $data(sock) [subst {
        </body>
        </html>
    }]
}

# EOF $RCSfile: phttpd.tcl,v $
# Emacs Setup Variables
# Local Variables:
# mode: Tcl
# indent-tabs-mode: nil
# tcl-basic-offset: 4
# End:

