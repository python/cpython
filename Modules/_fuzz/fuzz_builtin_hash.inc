int fuzz_builtin_hash(const char* data, size_t size) {
    _Py_HashBytes(data, size);
    return 0;
}
