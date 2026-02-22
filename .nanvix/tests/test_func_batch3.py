"""Batch 3: Parsing, encoding, data structures"""
import sys
results = []

# click
try:
    import click
    @click.command()
    @click.option("--name", default="World")
    def hello(name): pass
    assert hello is not None
    results.append(("click", "PASS"))
except Exception as e: results.append(("click", f"FAIL: {e}"))

# certifi
try:
    import certifi
    ca = certifi.where()
    assert ca.endswith(".pem")
    results.append(("certifi", "PASS"))
except Exception as e: results.append(("certifi", f"FAIL: {e}"))

# chardet
try:
    import chardet
    r = chardet.detect(b"Hello World")
    assert r["encoding"] is not None
    results.append(("chardet", "PASS"))
except Exception as e: results.append(("chardet", f"FAIL: {e}"))

# charset-normalizer
try:
    from charset_normalizer import from_bytes
    r = from_bytes(b"Hello World")
    assert r is not None
    results.append(("charset-normalizer", "PASS"))
except Exception as e: results.append(("charset-normalizer", f"FAIL: {e}"))

# idna
try:
    import idna
    encoded = idna.encode("münchen.de")
    assert b"xn--" in encoded
    results.append(("idna", "PASS"))
except Exception as e: results.append(("idna", f"FAIL: {e}"))

# pycparser
try:
    import pycparser
    p = pycparser.CParser()
    assert p is not None
    results.append(("pycparser", "PASS"))
except Exception as e: results.append(("pycparser", f"FAIL: {e}"))

# ply
try:
    import ply.lex
    assert ply.lex is not None
    results.append(("ply", "PASS"))
except Exception as e: results.append(("ply", f"FAIL: {e}"))

# ordered-set
try:
    from ordered_set import OrderedSet
    s = OrderedSet([3, 1, 4, 1, 5])
    assert list(s) == [3, 1, 4, 5]
    results.append(("ordered-set", "PASS"))
except Exception as e: results.append(("ordered-set", f"FAIL: {e}"))

# immutabledict
try:
    from immutabledict import immutabledict
    d = immutabledict({"a": 1, "b": 2})
    assert d["a"] == 1
    results.append(("immutabledict", "PASS"))
except Exception as e: results.append(("immutabledict", f"FAIL: {e}"))

# et-xmlfile
try:
    from xml.etree.ElementTree import Element
    import io
    from et_xmlfile import xmlfile
    f = io.BytesIO()
    with xmlfile(f) as xf:
        with xf.element("root"):
            xf.write(Element("child"))
    assert b"<root>" in f.getvalue()
    results.append(("et-xmlfile", "PASS"))
except Exception as e: results.append(("et-xmlfile", f"FAIL: {e}"))

# flatbuffers
try:
    import flatbuffers
    b = flatbuffers.Builder(256)
    assert b is not None
    results.append(("flatbuffers", "PASS"))
except Exception as e: results.append(("flatbuffers", f"FAIL: {e}"))

# gast
try:
    import gast
    node = gast.parse("x = 1")
    assert isinstance(node, gast.Module)
    results.append(("gast", "PASS"))
except Exception as e: results.append(("gast", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
