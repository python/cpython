"""Tests for the Perfetto sampling collector."""

import unittest

from profiling.sampling.perfetto_collector import PerfettoCollector


def _read_varint(data, offset):
    value = 0
    shift = 0
    while True:
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7f) << shift
        if not byte & 0x80:
            return value, offset
        shift += 7


def _decode_fields(data):
    """Return protobuf fields as (field number, wire type, value) tuples."""
    fields = []
    offset = 0
    while offset < len(data):
        tag, offset = _read_varint(data, offset)
        field = tag >> 3
        wire_type = tag & 7
        if wire_type == 0:
            value, offset = _read_varint(data, offset)
        elif wire_type == 2:
            size, offset = _read_varint(data, offset)
            value = data[offset:offset + size]
            offset += size
        else:
            raise AssertionError(f"unsupported wire type {wire_type}")
        fields.append((field, wire_type, value))
    return fields


def _field_values(data, field):
    return [value for number, _, value in _decode_fields(data)
            if number == field]


class TestPerfettoCollector(unittest.TestCase):
    def test_frame_source_locations_are_encoded_separately(self):
        collector = PerfettoCollector(sample_interval_usec=1000)
        filename = "/src/package/example.py"
        frames = [
            (filename, (12, 12, 4, 10), "work", None),
            (filename, (3, 3, 0, 8), "work", None),
        ]

        collector._callstack_id(frames)
        interned = collector._pending_interned

        # The function name and source path are each interned once even though
        # two distinct source lines produce two frames.
        function_names = _field_values(interned, 5)
        self.assertEqual(len(function_names), 1)
        self.assertEqual(_field_values(function_names[0], 2), [b"work"])

        source_paths = _field_values(interned, 18)
        self.assertEqual(len(source_paths), 1)
        self.assertEqual(_field_values(source_paths[0], 2),
                         [filename.encode()])

        encoded_frames = _field_values(interned, 6)
        self.assertEqual(len(encoded_frames), 2)
        self.assertEqual([_field_values(frame, 2) for frame in encoded_frames],
                         [[1], [1]])
        self.assertEqual([_field_values(frame, 5) for frame in encoded_frames],
                         [[1], [1]])
        self.assertEqual([_field_values(frame, 6) for frame in encoded_frames],
                         [[3], [12]])
        self.assertEqual([_field_values(frame, 7) for frame in encoded_frames],
                         [[3], [3]])

        # Symbolized Python frames have no invented mapping or program counter.
        for frame in encoded_frames:
            self.assertEqual(_field_values(frame, 3), [])
            self.assertEqual(_field_values(frame, 4), [])

    def test_unknown_line_number_is_omitted(self):
        collector = PerfettoCollector(sample_interval_usec=1000)
        collector._frame_id(
            ("/src/package/example.py", (-1, -1, -1, -1), "work", None))

        encoded_frame, = _field_values(collector._pending_interned, 6)
        self.assertEqual(_field_values(encoded_frame, 5), [1])
        self.assertEqual(_field_values(encoded_frame, 6), [])

    def test_native_and_gc_markers_have_no_source_or_mapping(self):
        collector = PerfettoCollector(sample_interval_usec=1000)
        collector._callstack_id([
            ("~", None, "<native>", None),
            ("~", None, "<GC>", None),
        ])

        encoded_frames = _field_values(collector._pending_interned, 6)
        self.assertEqual(len(encoded_frames), 2)
        # The callstack is reversed while interning, so GC is encoded first.
        self.assertEqual([_field_values(frame, 7) for frame in encoded_frames],
                         [[5], [1]])
        for frame in encoded_frames:
            self.assertEqual(_field_values(frame, 3), [])
            self.assertEqual(_field_values(frame, 4), [])
            self.assertEqual(_field_values(frame, 5), [])
            self.assertEqual(_field_values(frame, 6), [])
        self.assertEqual(_field_values(collector._pending_interned, 18), [])

    def test_same_named_files_do_not_share_frames(self):
        collector = PerfettoCollector(sample_interval_usec=1000)

        first = collector._frame_id(
            ("/first/example.py", 10, "work", None))
        second = collector._frame_id(
            ("/second/example.py", 10, "work", None))

        self.assertNotEqual(first, second)
        self.assertEqual(len(collector._func_ids), 1)
        self.assertEqual(len(collector._source_path_ids), 2)


if __name__ == "__main__":
    unittest.main()
