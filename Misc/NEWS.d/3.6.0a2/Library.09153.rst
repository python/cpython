Issue #26373: subprocess.Popen.communicate now correctly ignores
BrokenPipeError when the child process dies before .communicate()
is called in more/all circumstances.