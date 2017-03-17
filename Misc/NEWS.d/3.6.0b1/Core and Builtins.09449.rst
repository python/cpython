Issue #27812: Properly clear out a generator's frame's backreference to the
generator to prevent crashes in frame.clear().