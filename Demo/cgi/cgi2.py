#!/usr/local/bin/python

"""CGI test 2 - basic use of cgi module."""

import cgitb; cgitb.enable()

import cgi

def main():
    form = cgi.FieldStorage()
    print "Content-type: text/html"
    print
    if not form:
        print "<h1>No Form Keys</h1>"
    else:
        print "<h1>Form Keys</h1>"
        for key in form.keys():
            value = form[key].value
            print "<p>", cgi.escape(key), ":", cgi.escape(value)

if __name__ == "__main__":
    main()
