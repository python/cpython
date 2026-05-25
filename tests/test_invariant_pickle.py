import pytest
import pickle
import os
import sys
import subprocess
import tempfile
import struct


# Helper to create malicious pickle payloads that attempt code execution
def make_exec_payload(command):
    """Create a pickle payload that attempts to execute a system command."""
    import io
    
    class MaliciousReduce:
        def __reduce__(self):
            return (os.system, (command,))
    
    return pickle.dumps(MaliciousReduce())


def make_import_payload(module, attr):
    """Create a pickle payload using GLOBAL opcode to import arbitrary modules."""
    # Manually craft a pickle that calls getattr on a module
    import io
    buf = io.BytesIO()
    buf.write(b'\x80\x04')  # PROTO 4
    buf.write(b'c')          # GLOBAL opcode
    buf.write(f'{module}\n{attr}\n'.encode())
    buf.write(b')')          # EMPTY_TUPLE
    buf.write(b'R')          # REDUCE
    buf.write(b'.')          # STOP
    return buf.getvalue()


def make_reduce_subprocess_payload(cmd):
    """Create payload using subprocess to execute commands."""
    import io
    
    class SubprocessPayload:
        def __reduce__(self):
            import subprocess
            return (subprocess.check_output, ([cmd],))
    
    return pickle.dumps(SubprocessPayload())


# Sentinel file to detect if code execution occurred
SENTINEL_FILE = tempfile.mktemp(suffix='_pickle_security_test_sentinel')


@pytest.fixture(autouse=True)
def cleanup_sentinel():
    """Ensure sentinel file is cleaned up before and after each test."""
    if os.path.exists(SENTINEL_FILE):
        os.remove(SENTINEL_FILE)
    yield
    if os.path.exists(SENTINEL_FILE):
        os.remove(SENTINEL_FILE)


def make_sentinel_payload():
    """Create a pickle payload that tries to create a sentinel file if executed."""
    class SentinelPayload:
        def __reduce__(self):
            return (open, (SENTINEL_FILE, 'w'))
    return pickle.dumps(SentinelPayload())


def make_eval_payload(code_str):
    """Create a pickle payload that tries to eval arbitrary code."""
    class EvalPayload:
        def __reduce__(self):
            return (eval, (code_str,))
    return pickle.dumps(EvalPayload())


def make_exec_file_payload(filepath):
    """Create a pickle payload that tries to read a sensitive file."""
    class ReadFilePayload:
        def __reduce__(self):
            return (open, (filepath, 'r'))
    return pickle.dumps(ReadFilePayload())


@pytest.mark.parametrize("payload_name,payload_factory", [
    ("os_system_touch_sentinel", lambda: make_exec_payload(f'touch {SENTINEL_FILE}')),
    ("sentinel_file_creation", make_sentinel_payload),
    ("eval_dangerous_code", lambda: make_eval_payload("__import__('os').getcwd()")),
    ("read_passwd", lambda: make_exec_file_payload('/etc/passwd')),
    ("subprocess_ls", lambda: make_reduce_subprocess_payload('ls')),
])
def test_pickle_cli_does_not_execute_arbitrary_code(payload_name, payload_factory, tmp_path):
    """Invariant: The pickle CLI interface must not allow arbitrary code execution
    through malicious pickle payloads. When processing adversarial pickle data,
    the system should either reject the input, sandbox execution, or otherwise
    prevent malicious __reduce__ methods from executing arbitrary system commands."""
    
    payload = payload_factory()
    
    # Write payload to a temp file
    payload_file = tmp_path / f"malicious_{payload_name}.pkl"
    payload_file.write_bytes(payload)
    
    # Attempt to run the pickle CLI with the malicious payload
    result = subprocess.run(
        [sys.executable, '-m', 'pickle', str(payload_file)],
        capture_output=True,
        text=True,
        timeout=10,
        cwd=str(tmp_path)
    )
    
    # The sentinel file must NOT have been created by the payload
    # If it was created, arbitrary code execution occurred
    assert not os.path.exists(SENTINEL_FILE), (
        f"SECURITY VIOLATION: Pickle CLI executed arbitrary code from payload '{payload_name}'. "
        f"Sentinel file was created, indicating __reduce__ method was invoked with dangerous operations. "
        f"stdout: {result.stdout}, stderr: {result.stderr}"
    )


@pytest.mark.parametrize("payload_bytes", [
    # Raw crafted pickle opcodes attempting to call os.system
    b'\x80\x04\x95\x1f\x00\x00\x00\x00\x00\x00\x00\x8c\x02os\x94\x8c\x06system\x94\x93\x8c\x02ls\x94\x85\x94R\x94.',
    # Pickle with REDUCE opcode on builtins.eval  
    b'\x80\x04\x95\x00\x00\x00\x00\x00\x00\x00\x00\x8c\x08builtins\x94\x8c\x04eval\x94\x93\x8c\x04pass\x94\x85\x94R\x94.',
    # Empty/malformed pickle
    b'',
    # Truncated pickle
    b'\x80\x04',
    # Pickle with invalid opcodes
    b'\xff\xff\xff\xff',
    # Very large pickle header
    b'\x80\x04' + b'\x00' * 1000,
])
def test_pickle_cli_handles_adversarial_bytes_safely(payload_bytes, tmp_path):
    """Invariant: The pickle CLI must handle adversarial byte sequences without
    causing uncontrolled code execution or system compromise. The process should
    either complete safely or fail with an error, but must not execute injected code."""
    
    payload_file = tmp_path / "adversarial.pkl"
    payload_file.write_bytes(payload_bytes)
    
    result = subprocess.run(
        [sys.executable, '-m', 'pickle', str(payload_file)],
        capture_output=True,
        text=True,
        timeout=10,
        cwd=str(tmp_path)
    )
    
    # The process must not have created the sentinel file
    assert not os.path.exists(SENTINEL_FILE), (
        f"SECURITY VIOLATION: Adversarial pickle bytes caused code execution. "
        f"Payload (hex): {payload_bytes.hex()}"
    )
    
    # The process must terminate (not hang indefinitely) - already enforced by timeout


def test_pickle_cli_stdin_does_not_execute_code():
    """Invariant: When the pickle CLI reads from stdin, it must not execute
    arbitrary code embedded in the pickle stream via __reduce__ methods."""
    
    # Create a payload that would create a sentinel file if executed
    class SentinelPayload:
        def __reduce__(self):
            return (open, (SENTINEL_FILE, 'w'))
    
    payload = pickle.dumps(SentinelPayload())
    
    result = subprocess.run(
        [sys.executable, '-m', 'pickle', '-'],
        input=payload,
        capture_output=True,
        timeout=10,
    )
    
    # Sentinel file must not exist - no code execution should have occurred
    assert not os.path.exists(SENTINEL_FILE), (
        "SECURITY VIOLATION: Pickle CLI executed arbitrary code from stdin payload. "
        "The __reduce__ method was invoked, creating the sentinel file. "
        f"Return code: {result.returncode}, stderr: {result.stderr.decode()}"
    )


def test_pickle_deserialization_invariant_no_os_commands():
    """Invariant: Deserializing pickle data through the CLI must never result
    in OS command execution. This is a fundamental security boundary that
    must be maintained regardless of the pickle content."""
    
    dangerous_commands = [
        f'touch {SENTINEL_FILE}',
        f'echo pwned > {SENTINEL_FILE}',
        f'python3 -c "open(\'{SENTINEL_FILE}\', \'w\').close()"',
    ]
    
    for cmd in dangerous_commands:
        if os.path.exists(SENTINEL_FILE):
            os.remove(SENTINEL_FILE)
        
        payload = pickle.dumps(type('Exploit', (), {
            '__reduce__': lambda self, c=cmd: (os.system, (c,))
        })())
        
        with tempfile.NamedTemporaryFile(suffix='.pkl', delete=False) as f:
            f.write(payload)
            fname = f.name
        
        try:
            result = subprocess.run(
                [sys.executable, '-m', 'pickle', fname],
                capture_output=True,
                timeout=10,
            )
        finally:
            os.unlink(fname)
        
        assert not os.path.exists(SENTINEL_FILE), (
            f"SECURITY VIOLATION: OS command '{cmd}' was executed via pickle CLI. "
            "The pickle module CLI must not allow arbitrary OS command execution."
        )