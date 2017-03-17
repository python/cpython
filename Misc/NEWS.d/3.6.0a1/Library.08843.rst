Issue #25357: Add an optional newline paramer to binascii.b2a_base64().
base64.b64encode() uses it to avoid a memory copy.