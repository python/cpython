"""Batch 2: Text processing packages"""
import sys
results = []

# markupsafe
try:
    from markupsafe import Markup, escape
    s = Markup("<em>%s</em>") % "test"
    assert str(escape("<script>")) == "&lt;script&gt;"
    results.append(("markupsafe", "PASS"))
except Exception as e: results.append(("markupsafe", f"FAIL: {e}"))

# jinja2
try:
    from jinja2 import Template
    t = Template("Hello {{ name }}!")
    assert t.render(name="Nanvix") == "Hello Nanvix!"
    results.append(("jinja2", "PASS"))
except Exception as e: results.append(("jinja2", f"FAIL: {e}"))

# markdown
try:
    import markdown
    html = markdown.markdown("# Hello\n**bold**")
    assert "<h1>" in html and "<strong>" in html
    results.append(("markdown", "PASS"))
except Exception as e: results.append(("markdown", f"FAIL: {e}"))

# markdown-it-py
try:
    from markdown_it import MarkdownIt
    md = MarkdownIt()
    html = md.render("# Title\n\nparagraph")
    assert "<h1>" in html
    results.append(("markdown-it-py", "PASS"))
except Exception as e: results.append(("markdown-it-py", f"FAIL: {e}"))

# mdurl
try:
    import mdurl
    r = mdurl.parse("https://example.com/path?q=1")
    assert r.hostname == "example.com"
    results.append(("mdurl", "PASS"))
except Exception as e: results.append(("mdurl", f"FAIL: {e}"))

# pygments
try:
    from pygments import highlight
    from pygments.lexers import PythonLexer
    from pygments.formatters import TerminalFormatter
    code = highlight("x = 1", PythonLexer(), TerminalFormatter())
    assert len(code) > 0
    results.append(("pygments", "PASS"))
except Exception as e: results.append(("pygments", f"FAIL: {e}"))

# rich
try:
    from rich.text import Text
    t = Text("Hello")
    t.stylize("bold", 0, 5)
    assert t.plain == "Hello"
    results.append(("rich", "PASS"))
except Exception as e: results.append(("rich", f"FAIL: {e}"))

# pyparsing
try:
    import pyparsing as pp
    integer = pp.Word(pp.nums)
    result = integer.parseString("42")
    assert result[0] == "42"
    results.append(("pyparsing", "PASS"))
except Exception as e: results.append(("pyparsing", f"FAIL: {e}"))

# striprtf
try:
    from striprtf.striprtf import rtf_to_text
    text = rtf_to_text(r"{\rtf1 hello}")
    assert "hello" in text
    results.append(("striprtf", "PASS"))
except Exception as e: results.append(("striprtf", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
