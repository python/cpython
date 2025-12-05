# Pickle Chunked Reading Benchmark

This benchmark measures the performance impact of the chunked reading optimization in GH PR #119204 for the pickle module.

## What This Tests

The PR adds chunked reading (1MB chunks) to prevent memory exhaustion when unpickling large objects:
- **BINBYTES8** - Large bytes objects (protocol 4+)
- **BINUNICODE8** - Large strings (protocol 4+)
- **BYTEARRAY8** - Large bytearrays (protocol 5)
- **FRAME** - Large frames
- **LONG4** - Large integers
- An antagonistic mode that tests using memory denial of service inducing malicious pickles.

## Quick Start

```bash
# Run full benchmark suite (1MiB → 200MiB, takes several minutes)
build/python Tools/picklebench/memory_dos_impact.py

# Test just a few sizes (quick test: 1, 10, 50 MiB)
build/python Tools/picklebench/memory_dos_impact.py --sizes 1 10 50

# Test smaller range for faster results
build/python Tools/picklebench/memory_dos_impact.py --sizes 1 5 10

# Output as markdown for reports
build/python Tools/picklebench/memory_dos_impact.py --format markdown > results.md

# Test with protocol 4 instead of 5
build/python Tools/picklebench/memory_dos_impact.py --protocol 4
```

**Note:** Sizes are specified in MiB. Use `--sizes 1 2 5` for 1MiB, 2MiB, 5MiB objects.

## Antagonistic Mode (DoS Protection Test)

The `--antagonistic` flag tests **malicious pickles** that demonstrate the memory DoS protection:

```bash
# Quick DoS protection test (claims 10, 50, 100 MB but provides 1KB)
build/python Tools/picklebench/memory_dos_impact.py --antagonistic --sizes 10 50 100

# Full DoS test (default: 10, 50, 100, 500, 1000, 5000 MB claimed)
build/python Tools/picklebench/memory_dos_impact.py --antagonistic
```

### What This Tests

Unlike normal benchmarks that test **legitimate pickles**, antagonistic mode tests:
- **Truncated BINBYTES8**: Claims 100MB but provides only 1KB (will fail to unpickle)
- **Truncated BINUNICODE8**: Same for strings
- **Truncated BYTEARRAY8**: Same for bytearrays
- **Sparse memo attacks**: PUT at index 1 billion (would allocate huge array before PR)

**Key difference:**
- **Normal mode**: Tests real data, shows ~5% time overhead
- **Antagonistic mode**: Tests malicious data, shows ~99% memory savings

### Expected Results

```
100MB Claimed (actual: 1KB)
  binbytes8_100MB_claim
    Peak memory:     1.00 MB (claimed: 100 MB, saved: 99.00 MB, 99.0%)
    Error: UnpicklingError  ← Expected!

Summary:
  Average claimed: 126.2 MB
  Average peak:    0.54 MB
  Average saved:   125.7 MB (99.6% reduction)
Protection Status: ✓ Memory DoS attacks mitigated by chunked reading
```

**Before PR**: Would allocate full claimed size (100MB+), potentially crash
**After PR**: Allocates 1MB chunks, fails fast with minimal memory

This demonstrates the **security improvement** - protection against memory exhaustion attacks.

## Before/After Comparison

The benchmark includes an automatic comparison feature that runs the same tests on both a baseline and current Python build.

### Option 1: Automatic Comparison (Recommended)

Build both versions, then use `--baseline` to automatically compare:

```bash
# Build the baseline (main branch without PR)
git checkout main
mkdir -p build-main
cd build-main && ../configure && make -j $(nproc) && cd ..

# Build the current version (with PR)
git checkout unpickle-overallocate
mkdir -p build
cd build && ../configure && make -j $(nproc) && cd ..

# Run automatic comparison (quick test with a few sizes)
build/python Tools/picklebench/memory_dos_impact.py \
  --baseline build-main/python \
  --sizes 1 10 50

# Full comparison (all default sizes)
build/python Tools/picklebench/memory_dos_impact.py \
  --baseline build-main/python
```

The comparison output shows:
- Side-by-side metrics (Current vs Baseline)
- Percentage change for time and memory
- Overall summary statistics

### Interpreting Comparison Results

- **Time change**: Small positive % is expected (chunking adds overhead, typically 5-10%)
- **Memory change**: Negative % is good (chunking saves memory, especially for large objects)
- **Trade-off**: Slightly slower but much safer against memory exhaustion attacks

### Option 2: Manual Comparison

Save results separately and compare manually:

```bash
# Baseline results
build-main/python Tools/picklebench/memory_dos_impact.py --format json > baseline.json

# Current results
build/python Tools/picklebench/memory_dos_impact.py --format json > current.json

# Manual comparison
diff -y <(jq '.' baseline.json) <(jq '.' current.json)
```

## Understanding the Results

### Critical Sizes

The default test suite includes:
- **< 1MiB (999,000 bytes)**: No chunking, allocates full size upfront
- **= 1MiB (1,048,576 bytes)**: Threshold, chunking just starts
- **> 1MiB (1,048,577 bytes)**: Chunked reading engaged
- **1, 2, 5, 10MiB**: Show scaling behavior with chunking
- **20, 50, 100, 200MiB**: Stress test large object handling

**Note:** The full suite may require more than 16GiB of RAM.

### Key Metrics

- **Time (mean)**: Average unpickling time - should be similar before/after
- **Time (stdev)**: Consistency - lower is better
- **Peak Memory**: Maximum memory during unpickling - **expected to be LOWER after PR**
- **Pickle Size**: Size of the serialized data on disk

### Test Types

| Test | What It Stresses |
|------|------------------|
| `bytes_*` | BINBYTES8 opcode, raw binary data |
| `string_ascii_*` | BINUNICODE8 with simple ASCII |
| `string_utf8_*` | BINUNICODE8 with multibyte UTF-8 (€ chars) |
| `bytearray_*` | BYTEARRAY8 opcode (protocol 5) |
| `list_large_items_*` | Multiple chunked reads in sequence |
| `dict_large_values_*` | Chunking in dict deserialization |
| `nested_*` | Realistic mixed data structures |
| `tuple_*` | Immutable structures |

## Expected Results

### Before PR (main branch)
- Single large allocation per object
- Risk of memory exhaustion with malicious pickles

### After PR (unpickle-overallocate branch)
- Chunked allocation (1MB at a time)
- **Slightly higher CPU time** (multiple allocations + resizing)
- **Significantly lower peak memory** (no large pre-allocation)
- Protection against DoS via memory exhaustion

## Advanced Usage

### Test Specific Sizes

```bash
# Test only 5MiB and 10MiB objects
build/python Tools/picklebench/memory_dos_impact.py --sizes 5 10

# Test large objects: 50, 100, 200 MiB
build/python Tools/picklebench/memory_dos_impact.py --sizes 50 100 200
```

### More Iterations for Stable Timing

```bash
# Run 10 iterations per test for better statistics
build/python Tools/picklebench/memory_dos_impact.py --iterations 10 --sizes 1 10
```

### JSON Output for Analysis

```bash
# Generate JSON for programmatic analysis
build/python Tools/picklebench/memory_dos_impact.py --format json | python -m json.tool
```

## Interpreting Memory Results

The **peak memory** metric shows the maximum memory allocated during unpickling:

- **Without chunking**: Allocates full size immediately
  - 10MB object → 10MB allocation upfront

- **With chunking**: Allocates in 1MB chunks, grows geometrically
  - 10MB object → starts with 1MB, grows: 2MB, 4MB, 8MB (final: ~10MB total)
  - Peak is lower because allocation is incremental

## Typical Results

On a system with the PR applied, you should see:

```
1.00MiB Test Results
  bytes_1.00MiB:     ~0.3ms, 1.00MiB peak  (just at threshold)

2.00MiB Test Results
  bytes_2.00MiB:     ~0.8ms, 2.00MiB peak  (chunked: 1MiB → 2MiB)

10.00MiB Test Results
  bytes_10.00MiB:    ~3-5ms, 10.00MiB peak (chunked: 1→2→4→8→10 MiB)
```

Time overhead is minimal (~10-20% for very large objects), but memory safety is significantly improved.
