"""Test: jinja2"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from jinja2 import Template
    t = Template("Hello {{ name }}!")
    assert t.render(name="Nanvix") == "Hello Nanvix!"
    print("jinja2: PASS")
except Exception as e:
    print(f"jinja2: FAIL: {e}")
    sys.exit(1)
