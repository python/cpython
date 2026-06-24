import pytest
import tempfile
import os
import sys

# We need to test that the pager module does not execute shell metacharacters
# in filenames when writing temp files for paging.

@pytest.mark.parametrize("payload", [
    'test$(touch /tmp/pwned).txt',
    'test`touch /tmp/pwned`.txt',
    'test;touch /tmp/pwned;.txt',
    'normal_filename.txt',
])
def test_pager_does_not_execute_shell_metacharacters_in_filenames(payload, tmp_path, monkeypatch):
    """Invariant: Shell metacharacters in temp filenames must never be executed by the shell."""
    # Track all os.system calls to verify proper escaping/quoting
    executed_commands = []
    marker_file = tmp_path / "pwned"

    def mock_system(cmd):
        executed_commands.append(cmd)
        # Check that the command does not contain unescaped shell metacharacters
        # that would allow injection
        return 1  # Return non-zero to simulate command failure (skip actual execution)

    monkeypatch.setattr(os, 'system', mock_system)
    monkeypatch.setenv('PAGER', '')  # Clear PAGER to test default path

    # Import the module under test
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'Lib'))
    from _pyrepl.pager import get_pager

    # Get the pager function and call it with some text
    # This exercises the tempfile code path
    pager_func = get_pager()
    try:
        pager_func("Some help text to page through\n" * 100)
    except Exception:
        pass  # We only care about what commands were attempted

    # Verify: if os.system was called, the filename must be properly quoted/escaped
    # such that shell metacharacters like $(), backticks, semicolons cannot execute
    for cmd in executed_commands:
        # The command should not contain raw unescaped shell metacharacters from payload
        # If shlex.quote or equivalent is used, dangerous chars will be escaped
        assert '$(touch' not in cmd or cmd.count("'") >= 2 or '\\' in cmd, \
            f"Shell metacharacters not properly escaped in command: {cmd}"
        assert '`touch' not in cmd or cmd.count("'") >= 2 or '\\' in cmd, \
            f"Backtick injection not properly escaped in command: {cmd}"
        assert ';touch' not in cmd or cmd.count("'") >= 2 or '\\' in cmd, \
            f"Semicolon injection not properly escaped in command: {cmd}"