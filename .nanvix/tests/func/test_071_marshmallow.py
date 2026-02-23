"""Test: marshmallow"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from marshmallow import Schema, fields
    class UserSchema(Schema):
        name = fields.Str(required=True)
        age = fields.Int()
    schema = UserSchema()
    result = schema.load({"name": "test", "age": 25})
    assert result["name"] == "test"
    print("marshmallow: PASS")
except Exception as e:
    print(f"marshmallow: FAIL: {e}")
    sys.exit(1)
