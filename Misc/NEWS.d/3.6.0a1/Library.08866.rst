Issue #6973: When we know a subprocess.Popen process has died, do
not allow the send_signal(), terminate(), or kill() methods to do
anything as they could potentially signal a different process.