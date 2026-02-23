"""Test: python-dotenv"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from dotenv import dotenv_values
    import tempfile, os
    with tempfile.NamedTemporaryFile(mode="w", suffix=".env", delete=False) as f:
        f.write("KEY=value\n")
        f.flush()
        vals = dotenv_values(f.name)
    os.unlink(f.name)
    assert vals["KEY"] == "value"
    print("python-dotenv: PASS")
except Exception as e:
    print(f"python-dotenv: FAIL: {e}")
    sys.exit(1)
