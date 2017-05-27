"""Kill the emulator."""

import sys
import os
import telnetlib

def main():
    try:
        with telnetlib.Telnet('localhost', 5554) as tn:
            idx, _, bytes_read = tn.expect([b'Android Console'], timeout=5)
            if idx != 0:
                if bytes_read:
                    print(bytes_read, file=sys.stderr)
                return 'Timed out, not an Android Console'
            with open(os.path.expanduser('~/.emulator_console_auth_token'),
                                         'rb') as f:
                token = f.read()
            tn.write(b'auth ' + token + b'\n')
            tn.write(b'kill\n')
            print(tn.read_all().decode('ascii'), file=sys.stderr)
    except OSError as e:
        return e

if __name__ == "__main__":
    err = main()
    if err is not None:
        print('Error: Cannot telnet to the Android Console: %s.' % err)
        sys.exit(1)
