import math
import sys
import subprocess
import re
from shutil import copyfile
import random
KEY_ERROR_REGEX = re.compile(r"KeyError: HeapRef\([0-9]+\)$")

def execute3():
    cmd1 = subprocess.run(["python3", "tests/test2.py"], 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE)
    if cmd1.returncode == 0:
        return False
    cmd1stderr = cmd1.stderr.decode("utf-8")
    errlines = cmd1stderr.split("\n")
    match1 = errlines[-2] == 'SyntaxError: invalid syntax'
    match2 = errlines[-4] == '    log_file.flush()'
    return bool(match1 and match2)

def execute2():
    cmd1 = subprocess.run(["python3", "tests/test2.py"], 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE)
    if cmd1.returncode == 0:
        return False
    cmd1stdout = cmd1.stdout.decode("utf-8")
    if cmd1stdout:
        print(cmd1stdout)
    cmd1stderr = cmd1.stderr.decode("utf-8")
    if cmd1stderr:
        print(cmd1stderr)
    errlines = cmd1stderr.split("\n")
    match1 = errlines[-4] == '    not_covered &= ~member_value'
    match2 = errlines[-2] == 'SyntaxError: invalid syntax'
    return bool(match1 and match2)

def execute():
    print("Command 1")
    cmd1 = subprocess.run(["./python.exe", "tests/test2.py"], 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE)
    cmd1stdout = cmd1.stdout.decode("utf-8")
    if cmd1stdout:
        print(cmd1stdout)
    cmd1stderr = cmd1.stderr.decode("utf-8")
    if cmd1stderr:
        print(cmd1stderr)
    if cmd1.returncode != 0:
        return False
    
    print("Command 2")
    cmd2 = subprocess.run(
        ["python3", "recreate.py", "tests/test2.rewind"], 
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    cmd2stdout = cmd2.stdout.decode("utf-8")
    if cmd2stdout:
        print(cmd2stdout)
    cmd2stderr = cmd2.stderr.decode("utf-8")
    if cmd2stderr:
        print(cmd2stderr)

    if cmd2.returncode == 0:
        return False
    
    stderr = cmd2.stderr.decode("utf-8")
    errlines = stderr.split("\n")
    return errlines[-2] == 'TypeError: list indices must be integers or slices, not HeapRef'
    
def main():
    # print(execute3())
    simplify_test_case(execute3, "tests/test2.py")
    
def simplify_test_case(execute, input_file):
    copyfile(input_file, input_file + ".original")
    print("Backed up " + input_file + " to " + input_file + ".original")
    f = open(input_file)
    file_lines = open(input_file).readlines()
    prev_file_lines = file_lines
    chunk_idx = 0
    divisor = 2
    f.close()
    no_tries = 0
    result = execute()
    assert result, "should have problem initially"
    while True:
        prev_file_lines = file_lines
        file_lines = file_lines.copy()
        chunk_size = len(file_lines) / divisor
        if chunk_size < 1:
            break
        low = math.floor(chunk_idx * chunk_size)
        del file_lines[low:low + math.floor(chunk_size)]
        print("Try no", no_tries)
        print("Divisor", divisor)
        print("Chunk size", chunk_size)
        print("Removing chuck", chunk_idx)
        print("Reduced lines from", len(prev_file_lines), "to", len(file_lines))
        new_file_content = "".join(file_lines)
        f = open(input_file, "w")
        f.write(new_file_content)
        f.close()
        result = execute()
        no_tries += 1
        if result:
            print("Problem persists. Keeping change.")
            # keep this, reset divisor to 2
            f = open(input_file + ".last_good", "w")
            f.write(new_file_content)
            f.close()
            print("Saved to " + input_file + ".last_good")
            divisor = 2
            chunk_idx = 0
            no_tries = 0
        else:
            # undo
            print("Output changed. Undoing.")
            chunk_idx += 1
            if chunk_idx >= divisor - 1:
                print("Increase divisor to", divisor)
                divisor *= 2
                chunk_idx = 0
                print("Reset part index to", chunk_idx)
            else:
                print("Increased part index to", chunk_idx)
            file_lines = prev_file_lines
            prev_file_content = "".join(file_lines)
            f = open(input_file, "w")
            f.write(prev_file_content)
            f.close()
    print("Complete")

if __name__ == "__main__":
    main()