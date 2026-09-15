# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021 Taneli Hukkinen
# Licensed to PSF under a Contributor Agreement.

"""Utilities for tests that are in the "burntsushi" format."""

import datetime
from typing import Any


def convert(obj):
    if isinstance(obj, str):
        return {"type": "string", "value": obj}
    elif isinstance(obj, bool):
        return {"type": "bool", "value": str(obj).lower()}
    elif isinstance(obj, int):
        return {"type": "integer", "value": str(obj)}
    elif isinstance(obj, float):
        return {"type": "float", "value": _normalize_float_str(str(obj))}
    elif isinstance(obj, datetime.datetime):
        val = _normalize_datetime_str(obj.isoformat())
        if obj.tzinfo:
            return {"type": "datetime", "value": val}
        return {"type": "datetime-local", "value": val}
    elif isinstance(obj, datetime.time):
        return {
            "type": "time-local",
            "value": _normalize_localtime_str(str(obj)),
        }
    elif isinstance(obj, datetime.date):
        return {
            "type": "date-local",
            "value": str(obj),
        }
    elif isinstance(obj, list):
        return [convert(i) for i in obj]
    elif isinstance(obj, dict):
        return {k: convert(v) for k, v in obj.items()}
    raise Exception("unsupported type")


def normalize(obj: Any) -> Any:
    """Normalize test objects.

    This normalizes primitive values (e.g. floats)."""
    if isinstance(obj, list):
        return [normalize(item) for item in obj]
    if isinstance(obj, dict):
        if "type" in obj and "value" in obj:
            type_ = obj["type"]
            value = obj["value"]
            if type_ == "float":
                norm_value = _normalize_float_str(value)
            elif type_ in {"datetime", "datetime-local"}:
                norm_value = _normalize_datetime_str(value)
            elif type_ == "time-local":
                norm_value = _normalize_localtime_str(value)
            else:
                norm_value = value

            if type_ == "array":
                return [normalize(item) for item in value]
            return {"type": type_, "value": norm_value}
        return {k: normalize(v) for k, v in obj.items()}
    raise AssertionError("Burntsushi fixtures should be dicts/lists only")


def _normalize_datetime_str(dt_str: str) -> str:
    if dt_str[-1].lower() == "z":
        dt_str = dt_str[:-1] + "+00:00"

    date = dt_str[:10]
    rest = dt_str[11:]

    if "+" in rest:
        sign = "+"
    elif "-" in rest:
        sign = "-"
    else:
        sign = ""

    if sign:
        time, _, offset = rest.partition(sign)
    else:
        time = rest
        offset = ""

    time = time.rstrip("0") if "." in time else time
    return date + "T" + time + sign + offset


def _normalize_localtime_str(lt_str: str) -> str:
    return lt_str.rstrip("0") if "." in lt_str else lt_str


def _normalize_float_str(float_str: str) -> str:
    as_float = float(float_str)

    # Normalize "-0.0" and "+0.0"
    if as_float == 0:
        return "0"

    return str(as_float)
