.. bpo: 30119
.. date: 2017-07-26-15-11-17
.. nonce: DZ6C_S
.. release date: 2017-08-07
.. section: Library

ftplib.FTP.putline() now throws ValueError on commands that contains CR or
LF. Patch by Dong-hee Na.
