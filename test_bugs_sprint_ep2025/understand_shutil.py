import subprocess
from subprocess import Popen
import shlex

print("""\ncalling 'shlex.quote("for")'""")
subprocess.call(shlex.quote("for"), shell=True)

print("""\ncalling 'shlex.quote("'for'")'""")
subprocess.call(shlex.quote("'for'"), shell=True)

print("""\ncalling "'for'" """)
subprocess.call("'for'", shell=True, env={'PATH': '.'})

print("""\ncalling "for" """)
subprocess.call("for", shell=True, env={'PATH': '.'})

# import os, shlex, shutil, subprocess
# open("do", "w").write("#!/bin/sh\necho Something is being done...")

# os.chmod("do", 0o700)

# subprocess.call(shlex.quote("'./my_command'"), shell=True)
# subprocess.call("'my_command'", shell=True, env={'PATH': '.'})
# subprocess.run(shlex.quote("do"), shell=True, env={'PATH': '.'})

# print(shlex.quote("my_command"))

2
# p = Popen(shlex.split("mycommand"), shell=False, executable="/bin/bash")
# print(p)

# test = shlex.quote("done")
# print(test)

# class MyError(Exception):
#     def __init__(self):
#         print("Hello")
    

# class SomeProcessError(MyError):
#     def __init__(self, returncode):
#         self.returncode = returncode

#     def __str__(self):
#         return f"Died with returncode: {self.returncode}"

# raise SomeProcessError(3)


