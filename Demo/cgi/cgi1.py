#!/usr/local/bin/python

"""CGI test 1 - check server setup."""

# Until you get this to work, your web server isn't set up right or
# your Python isn't set up right.

# If cgi0.sh works but cgi1.py doesn't, check the #! line and the file
# permissions.  The docs for the cgi.py module have debugging tips.

print "Content-type: text/html"
print
print "<h1>Hello world</h1>"
print "<p>This is cgi1.py"
