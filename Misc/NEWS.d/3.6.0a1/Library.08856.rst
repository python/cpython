Issue #25510: fileinput.FileInput.readline() now returns b'' instead of ''
at the end if the FileInput was opened with binary mode.
Patch by Ryosuke Ito.