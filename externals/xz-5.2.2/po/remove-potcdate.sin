# Sed script that remove the POT-Creation-Date line in the header entry
# from a POT file.
#
# The distinction between the first and the following occurrences of the
# pattern is achieved by looking at the hold space.
/^"POT-Creation-Date: .*"$/{
x
# Test if the hold space is empty.
s/P/P/
ta
# Yes it was empty. First occurrence. Remove the line.
g
d
bb
:a
# The hold space was nonempty. Following occurrences. Do nothing.
x
:b
}
