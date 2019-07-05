import subprocess 


def run_cmd(argv):
    proc = subprocess.run(
            argv,
            #capture_output=True,
            #stderr=subprocess.STDOUT,
            stdout=subprocess.PIPE,
            text=True,
            check=True,
            )
    return proc.stdout
