Issue #27787: Call gc.collect() before checking each test for "dangling
threads", since the dangling threads are weak references.