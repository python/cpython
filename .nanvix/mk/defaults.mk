# defaults.mk - Shared variable defaults for all Makefile paths
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Included by common.mk (native / Linux-Docker builds) and docker-host.mk
# (Windows host-side re-invocation) so that platform defaults are defined
# in exactly one place.

# Nanvix Docker image for cross-compilation
NANVIX_DOCKER_IMAGE ?= nanvix/toolchain:latest-minimal

# Platform and deployment configuration (passed by nanvix-zutil / z.py)
PLATFORM ?= microvm
PROCESS_MODE ?= standalone
MEMORY_SIZE ?= 256mb

# Install prefix baked into the python binary (sys.prefix, sys.path)
INSTALL_PREFIX ?= /sysroot

# Set to 'yes' for release packaging
NANVIX_RELEASE ?= no

# Space-separated list of stdlib test modules to run via 'python3 -m test'.
# Keep this list to pure-Python, non-networking tests known to pass on Nanvix.
# Expand it as more tests are enabled (see issues linked from #320).
#
# Trimmed to validated-green modules only. The remaining modules fail due to
# missing platform capabilities (fork/exec, tempdir, asyncio event loop) or
# memory limits (test_json MemoryError). Those belong to #321 and #322 where
# each module gets individual skip/xfail annotations before being re-added.
# Excluded: test_exception_hierarchy (crashes at import time: errno.ESHUTDOWN missing on Nanvix)
#           test_inspect (VM hangs: IsolatedAsyncioTestCase sets up asyncio loop before skip is evaluated;
#                         module too large for 256MB VM causing resource exhaustion)
# The list is defined with := (not ?=) so that gen-cross-imports.py can
# rely on it as the canonical source without risk of env overrides causing
# the allow-list to drift out of sync.
NANVIX_TEST_LIST := \
    test_float test_complex test_bool test_struct \
    test_int test_range test_slice test_memoryview test_bytes test_tuple \
    test_builtin test_operator test_binop test_unary \
    test_compare test_richcmp test_augassign test_contains \
    test_grammar test_syntax test_compile test_compiler_assemble \
    test_compiler_codegen test_ast test_symtable test_opcache \
    test_peepholer test_dis test_code test_keyword test_tokenize \
    test_perf_profiler \
    test_call test_extcall test_positional_only_arg \
    test_scope test_global test_dynamic test_with \
    test_types test_typechecks test_isinstance test_hash test_index test_super test_property \
    test_math test_cmath test_decimal test_fractions test_statistics test_random test_numeric_tower \
    test_exception_group test_exceptions test_raise test_traceback \
    test_frame test_contextlib test_contextlib_async test_pprint test_reprlib \
    test_list test_dict

# Maximum number of test modules per regrtest VM invocation.
# The 32MB heap exhausts after ~5 test modules due to cumulative
# fragmentation.  Tests are split into batches of this size and
# each batch gets its own nanvixd.elf invocation.
NANVIX_TEST_BATCH_SIZE ?= 4

# Nanvix sysroot directory (set by z.py / environment)
NANVIX_HOME ?= $(HOME)/nanvix
