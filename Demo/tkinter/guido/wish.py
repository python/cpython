# This is about all it requires to write a wish shell in Python!

import _tkinter
import os
import sys

tk = _tkinter.create(os.environ['DISPLAY'], 'wish', 'Tk', 1)
tk.call('update')

cmd = ''

while 1:
    if cmd: prompt = ''
    else: prompt = '% '
    try:
        sys.stdout.write(prompt)
        sys.stdout.flush()
        line = sys.stdin.readline()
    except EOFError:
        break
    cmd = cmd + (line + '\n')
    if tk.getboolean(tk.call('info', 'complete', cmd)):
        tk.record(line)
        try:
            result = tk.call('eval', cmd)
        except _tkinter.TclError as msg:
            print('TclError:', msg)
        else:
            if result: print(result)
        cmd = ''
