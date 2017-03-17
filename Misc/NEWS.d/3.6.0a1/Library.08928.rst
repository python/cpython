Issue #26117: The os.scandir() iterator now closes file descriptor not only
when the iteration is finished, but when it was failed with error.