"""Benchmark for Python/pystrhex.c hex conversion functions.

This benchmarks the Python APIs that use pystrhex.c under the hood:
- bytes.hex(), bytearray.hex(), memoryview.hex()
- binascii.hexlify()
- hashlib digest .hexdigest() methods

Focus is on realistic cases: 0-512 bits (0-64 bytes) primarily,
with some larger sizes up to 4096 bytes.
"""

import binascii
import hashlib
from timeit import Timer

# Test data at various sizes (bytes)
# Focus on 0-64 bytes (0-512 bits) with some larger sizes
SIZES = [0, 1, 3, 4, 7, 8, 15, 16, 20, 32, 33, 64, 128, 256, 512, 4096]

def make_data(size):
    """Create test data of given size."""
    if size == 0:
        return b''
    # Use repeating pattern of 0-255
    full, remainder = divmod(size, 256)
    return bytes(range(256)) * full + bytes(range(remainder))

DATA = {size: make_data(size) for size in SIZES}

# Pre-create bytearray and memoryview versions
DATA_BYTEARRAY = {size: bytearray(data) for size, data in DATA.items()}
DATA_MEMORYVIEW = {size: memoryview(data) for size, data in DATA.items()}

# Timing parameters
REPEATS = 7
ITERATIONS = 50000


def run_benchmark(func, repeats=REPEATS, iterations=ITERATIONS):
    """Run benchmark and return time per operation in nanoseconds."""
    timing = min(Timer(func).repeat(repeats, iterations))
    return timing * 1_000_000_000 / iterations


def format_size(size):
    """Format size for display."""
    if size == 1:
        return "1 byte"
    return f"{size} bytes"


def bench_bytes_hex():
    """Benchmark bytes.hex() across sizes."""
    print("bytes.hex() by size:")
    for size in SIZES:
        data = DATA[size]
        ns = run_benchmark(data.hex)
        print(f"  {ns:7.1f} ns    {format_size(size)}")


def bench_bytes_hex_sep():
    """Benchmark bytes.hex() with separator."""
    print("\nbytes.hex(':') with separator (every byte):")
    for size in SIZES:
        data = DATA[size]
        ns = run_benchmark(lambda d=data: d.hex(':'))
        print(f"  {ns:7.1f} ns    {format_size(size)}")


def bench_bytes_hex_sep_group():
    """Benchmark bytes.hex() with separator every 2 bytes."""
    print("\nbytes.hex(':', 2) with separator (every 2 bytes):")
    # Skip 0 and 1 byte sizes since grouping doesn't apply well
    for size in SIZES:
        if size < 2:
            continue
        data = DATA[size]
        ns = run_benchmark(lambda d=data: d.hex(':', 2))
        print(f"  {ns:7.1f} ns    {format_size(size)}")


def bench_bytearray_hex():
    """Benchmark bytearray.hex() for comparison."""
    print("\nbytearray.hex() by size:")
    for size in SIZES:
        data = DATA_BYTEARRAY[size]
        ns = run_benchmark(data.hex)
        print(f"  {ns:7.1f} ns    {format_size(size)}")


def bench_memoryview_hex():
    """Benchmark memoryview.hex() for comparison."""
    print("\nmemoryview.hex() by size:")
    for size in SIZES:
        data = DATA_MEMORYVIEW[size]
        ns = run_benchmark(data.hex)
        print(f"  {ns:7.1f} ns    {format_size(size)}")


def bench_binascii_hexlify():
    """Benchmark binascii.hexlify() for comparison."""
    print("\nbinascii.hexlify() by size:")
    for size in SIZES:
        data = DATA[size]
        ns = run_benchmark(lambda d=data: binascii.hexlify(d))
        print(f"  {ns:7.1f} ns    {format_size(size)}")


def bench_binascii_hexlify_sep():
    """Benchmark binascii.hexlify() with separator."""
    print("\nbinascii.hexlify(sep=':') with separator:")
    for size in SIZES:
        data = DATA[size]
        ns = run_benchmark(lambda d=data: binascii.hexlify(d, ':'))
        print(f"  {ns:7.1f} ns    {format_size(size)}")


def bench_hash_hexdigest():
    """Benchmark hash .hexdigest() methods.

    These use _Py_strhex() internally to convert the digest to hex.
    Note: This measures hash computation + hex conversion together.
    """
    print("\nhashlib hexdigest (hash + hex conversion):")

    # Use 32 bytes of input data (256 bits) - realistic message size
    input_data = DATA[32]

    # MD5: 16-byte (128-bit) digest
    ns = run_benchmark(lambda: hashlib.md5(input_data).hexdigest())
    print(f"  {ns:7.1f} ns    md5 (16 byte digest)")

    # SHA1: 20-byte (160-bit) digest
    ns = run_benchmark(lambda: hashlib.sha1(input_data).hexdigest())
    print(f"  {ns:7.1f} ns    sha1 (20 byte digest)")

    # SHA256: 32-byte (256-bit) digest
    ns = run_benchmark(lambda: hashlib.sha256(input_data).hexdigest())
    print(f"  {ns:7.1f} ns    sha256 (32 byte digest)")

    # SHA512: 64-byte (512-bit) digest
    ns = run_benchmark(lambda: hashlib.sha512(input_data).hexdigest())
    print(f"  {ns:7.1f} ns    sha512 (64 byte digest)")


def bench_hash_hexdigest_only():
    """Benchmark just the hexdigest conversion (excluding hash computation).

    Pre-compute the hash object, then measure only the hexdigest() call.
    """
    print("\nhashlib hexdigest only (hex conversion, pre-computed hash):")

    input_data = DATA[32]

    # Pre-compute hash objects and copy for each iteration
    md5_hash = hashlib.md5(input_data)
    sha1_hash = hashlib.sha1(input_data)
    sha256_hash = hashlib.sha256(input_data)
    sha512_hash = hashlib.sha512(input_data)

    ns = run_benchmark(lambda h=md5_hash: h.hexdigest())
    print(f"  {ns:7.1f} ns    md5 (16 byte digest)")

    ns = run_benchmark(lambda h=sha1_hash: h.hexdigest())
    print(f"  {ns:7.1f} ns    sha1 (20 byte digest)")

    ns = run_benchmark(lambda h=sha256_hash: h.hexdigest())
    print(f"  {ns:7.1f} ns    sha256 (32 byte digest)")

    ns = run_benchmark(lambda h=sha512_hash: h.hexdigest())
    print(f"  {ns:7.1f} ns    sha512 (64 byte digest)")


if __name__ == '__main__':
    print("pystrhex.c benchmark")
    print("=" * 50)
    print(f"Timing: best of {REPEATS} runs, {ITERATIONS} iterations each\n")

    bench_bytes_hex()
    bench_bytes_hex_sep()
    bench_bytes_hex_sep_group()
    bench_bytearray_hex()
    bench_memoryview_hex()
    bench_binascii_hexlify()
    bench_binascii_hexlify_sep()
    bench_hash_hexdigest()
    bench_hash_hexdigest_only()

    print("\n" + "=" * 50)
    print("Done.")
