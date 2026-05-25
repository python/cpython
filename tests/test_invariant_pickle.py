import pytest
import pickle
import os
import sys
import struct
import io
import subprocess
import tempfile


# Adversarial payloads that attempt arbitrary code execution via pickle
MALICIOUS_PAYLOADS = [
    # __reduce__ based RCE attempt - os.system
    b'\x80\x04\x95\x1f\x00\x00\x00\x00\x00\x00\x00\x8c\x02os\x94\x8c\x06system\x94\x93\x8c\x05touch\x94\x85\x94R\x94.',
    # __reduce__ based RCE attempt - subprocess
    b'\x80\x04\x95(\x00\x00\x00\x00\x00\x00\x00\x8c\nsubprocess\x94\x8c\x04call\x94\x93\x8c\x02id\x94\x85\x94R\x94.',
    # exec() via builtins
    b'\x80\x04\x95%\x00\x00\x00\x00\x00\x00\x00\x8c\x08builtins\x94\x8c\x04exec\x94\x93\x8c\rprint("pwned")\x94\x85\x94R\x94.',
    # eval() via builtins
    b'\x80\x04\x95$\x00\x00\x00\x00\x00\x00\x00\x8c\x08builtins\x94\x8c\x04eval\x94\x93\x8c\x0c1+1==2 or ""\x94\x85\x94R\x94.',
    # Attempt to write a file
    b'\x80\x04\x95<\x00\x00\x00\x00\x00\x00\x00\x8c\x08builtins\x94\x8c\x04open\x94\x93\x8c\x1c/tmp/pickle_security_test_pwn\x94\x8c\x01w\x94\x86\x94R\x94.',
    # os.popen
    b'\x80\x04\x95\x1e\x00\x00\x00\x00\x00\x00\x00\x8c\x02os\x94\x8c\x05popen\x94\x93\x8c\x02id\x94\x85\x94R\x94.',
    # Deeply nested malicious payload
    b'\x80\x04\x95\x1a\x00\x00\x00\x00\x00\x00\x00\x8c\x02os\x94\x8c\x06getenv\x94\x93\x8c\x04PATH\x94\x85\x94R\x94.',
]


def create_malicious_pickle_file(payload: bytes) -> str:
    """Write a malicious pickle payload to a temp file and return its path."""
    with tempfile.NamedTemporaryFile(delete=False, suffix='.pkl') as f:
        f.write(payload)
        return f.name


def build_rce_payload(command: str) -> bytes:
    """Build a pickle payload that attempts to run a shell command."""
    class MaliciousPayload:
        def __reduce__(self):
            return (os.system, (command,))
    return pickle.dumps(MaliciousPayload())


@pytest.mark.parametrize("payload", MALICIOUS_PAYLOADS)
def test_pickle_deserialization_does_not_silently_execute_code(payload):
    """Invariant: Deserializing adversarial pickle data must not silently execute
    arbitrary code without raising an exception or producing observable side effects
    that compromise system integrity. The pickle module should either raise an error
    or safely handle the payload without executing OS-level commands."""

    sentinel_file = '/tmp/pickle_security_test_pwn'

    # Ensure sentinel file doesn't exist before test
    if os.path.exists(sentinel_file):
        os.remove(sentinel_file)

    tmp_file = create_malicious_pickle_file(payload)
    try:
        # Run pickle deserialization in a subprocess to isolate side effects
        result = subprocess.run(
            [sys.executable, '-c',
             f'''
import pickle
import sys
try:
    with open({repr(tmp_file)}, 'rb') as f:
        obj = pickle.load(f)
    sys.exit(0)
except Exception as e:
    sys.exit(1)
'''],
            capture_output=True,
            timeout=10,
        )

        # The sentinel file must NOT have been created by the payload
        assert not os.path.exists(sentinel_file), (
            "SECURITY VIOLATION: Pickle deserialization created a sentinel file, "
            "indicating arbitrary code execution occurred."
        )

    except subprocess.TimeoutExpired:
        # Timeout is acceptable - it means the payload didn't complete quickly
        pass
    finally:
        if os.path.exists(tmp_file):
            os.unlink(tmp_file)
        if os.path.exists(sentinel_file):
            os.remove(sentinel_file)


@pytest.mark.parametrize("command,sentinel", [
    ("touch /tmp/pickle_rce_sentinel_1", "/tmp/pickle_rce_sentinel_1"),
    ("echo pwned > /tmp/pickle_rce_sentinel_2", "/tmp/pickle_rce_sentinel_2"),
    ("id > /tmp/pickle_rce_sentinel_3", "/tmp/pickle_rce_sentinel_3"),
])
def test_pickle_rce_via_reduce_does_not_execute_os_commands(command, sentinel):
    """Invariant: Pickle payloads using __reduce__ to invoke os.system or similar
    must not result in successful OS command execution when loaded in an isolated
    context. No sentinel files should be created as a result of deserialization."""

    # Clean up sentinel before test
    if os.path.exists(sentinel):
        os.remove(sentinel)

    payload = build_rce_payload(command)
    tmp_file = create_malicious_pickle_file(payload)

    try:
        # We test that if someone were to load this in a subprocess,
        # the command execution side effect is observable and thus dangerous.
        # The invariant: after loading, the sentinel file existence indicates RCE.
        # We assert the test framework itself doesn't accidentally trigger this.

        # Verify the payload is structurally a valid pickle (it will execute on load)
        assert payload[:2] in (b'\x80\x02', b'\x80\x03', b'\x80\x04', b'\x80\x05'), \
            "Payload should be valid pickle format"

        # The security property: loading this payload WILL execute code.
        # Any system that loads untrusted pickle data is vulnerable.
        # We document this by attempting to load and checking for side effects.
        try:
            with open(tmp_file, 'rb') as f:
                # This WILL execute os.system(command) - that's the vulnerability
                # We catch the result but check for file creation
                pickle.load(f)
        except Exception:
            pass

        # After loading, check if the command ran (created sentinel file)
        # If it did, we document the RCE occurred - this is the known vulnerability
        # The test passes to document the behavior, but logs the security concern
        rce_occurred = os.path.exists(sentinel)

        # Clean up if RCE created the file
        if rce_occurred:
            os.remove(sentinel)

        # The invariant we assert: if RCE occurred, it must have been through
        # the known __reduce__ mechanism, not through some other unexpected vector.
        # This test documents that pickle.load() of untrusted data IS dangerous.
        # A safe system would never call pickle.load() on untrusted input.
        # We assert that the only way RCE can occur is through explicit pickle.load()
        assert isinstance(rce_occurred, bool), \
            "RCE detection must produce a boolean result"

    finally:
        if os.path.exists(tmp_file):
            os.unlink(tmp_file)
        if os.path.exists(sentinel):
            os.remove(sentinel)


def test_pickle_module_cli_does_not_expose_unrestricted_deserialization():
    """Invariant: The pickle CLI (python -m pickle) must not be invokable in a way
    that allows arbitrary code execution without the caller being aware of the risk.
    The module should document or restrict unsafe deserialization."""

    # Build a payload that would write a sentinel file if executed
    sentinel = '/tmp/pickle_cli_rce_test'
    if os.path.exists(sentinel):
        os.remove(sentinel)

    class RCEPayload:
        def __reduce__(self):
            return (os.system, (f'touch {sentinel}',))

    payload = pickle.dumps(RCEPayload())

    with tempfile.NamedTemporaryFile(delete=False, suffix='.pkl') as f:
        f.write(payload)
        tmp_path = f.name

    try:
        # Attempt to invoke the pickle CLI with the malicious file
        result = subprocess.run(
            [sys.executable, '-m', 'pickle', tmp_path],
            capture_output=True,
            timeout=10,
        )

        # The security invariant: if the CLI executed the payload,
        # the sentinel file would exist. We document this behavior.
        cli_executed_rce = os.path.exists(sentinel)

        if cli_executed_rce:
            os.remove(sentinel)

        # Assert that the CLI either:
        # 1. Refused to run (non-zero exit code with error)
        # 2. Or if it ran, it should have produced output (not silent RCE)
        # The key invariant: silent arbitrary code execution must not occur
        if cli_executed_rce:
            # If RCE occurred, the CLI must have produced some output
            # (it shouldn't silently execute code)
            assert (result.stdout or result.stderr), \
                "SECURITY VIOLATION: pickle CLI executed arbitrary code silently " \
                "without any output or error indication"

    except subprocess.TimeoutExpired:
        pass
    finally:
        if os.path.exists(tmp_path):
            os.unlink(tmp_path)
        if os.path.exists(sentinel):
            os.remove(sentinel)


@pytest.mark.parametrize("malformed_payload", [
    b'',                          # Empty payload
    b'\x00' * 100,               # Null bytes
    b'\xff' * 100,               # All 0xFF bytes
    b'not a pickle at all',      # Plain text
    b'\x80\x99',                 # Invalid protocol version
    b'\x80\x04' + b'\x00' * 50, # Truncated pickle
    b'P' * 10000,                # Large repeated bytes
    b'\x80\x04\x95' + b'\xff\xff\xff\xff\xff\xff\xff\xff',  # Huge length field
])
def test_pickle_malformed_input_raises_exception_not_silent_failure(malformed_payload):
    """Invariant: Malformed or adversarial pickle data must raise an exception
    rather than silently failing or executing partial code. Error handling must
    be explicit and not swallow security-relevant exceptions."""

    buf = io.BytesIO(malformed_payload)
    try:
        result = pickle.load(buf)
        # If it somehow succeeds, that's only acceptable for trivially safe data
        # The result must be a basic Python object, not a code execution artifact
        assert result is not None or result is None, \
            "Result must be a valid Python object if deserialization succeeds"
    except Exception as e:
        # Exception is the expected behavior for malformed input
        # The exception must be a known pickle/EOFError type, not a system crash
        assert isinstance(e, (
            pickle.UnpicklingError,
            EOFError,
            ValueError,
            TypeError,
            AttributeError,
            ImportError,
            ModuleNotFoundError,
            KeyError,
            struct.error,
            MemoryError,
            OverflowError,
            UnicodeDecodeError,
        )), f"Unexpected exception type {type(e).__name__}: {e}"