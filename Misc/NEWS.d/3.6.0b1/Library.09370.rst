Remove support for passing a file descriptor to os.access. It never worked but
previously didn't raise.