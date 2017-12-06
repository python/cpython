"""Start the emulator if not already running."""

import sys
import os
import argparse

from android_utils import run_subprocess, is_emulator_listening

def start_emulator(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--avd', required=True)
    parser.add_argument('--port', type=int, required=True)
    # Double the '-' prefix for the processing by argparse.
    args, unknown = parser.parse_known_args(
                        [('-' + arg if arg.startswith('-') else arg)
                         for arg in argv])

    if not is_emulator_listening(args.port):
        # Start the emulator.
        emulator = os.path.join(os.environ['ANDROID_SDK_ROOT'], 'emulator',
                                'emulator')
        run_subprocess(emulator, *argv)
    return 0

if __name__ == "__main__":
    sys.exit(start_emulator(sys.argv[1:]))
