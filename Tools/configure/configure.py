"""configure.py — run CPython's Python-based build configuration.

Usage:
    python configure.py [--with-*] [--enable-*] [--prefix=PATH] ...
"""

import pyconf  # noqa: F401 — initialises the pyconf singleton
import conf_buildopts
import conf_compiler
import conf_extlibs
import conf_filesystem
import conf_init
import conf_macos
import conf_math
import conf_modules
import conf_net
import conf_optimization
import conf_output
import conf_paths
import conf_platform
import conf_security
import conf_sharedlib
import conf_syslibs
import conf_targets
import conf_threads
import conf_wasm


def _run(v):
    """Run configure functions.  Note that order matters here since we are
    matching the behaviour of the configure.ac version of this script.
    """
    conf_init.setup_platform_defines()
    conf_init.setup_source_dirs(v)
    conf_init.setup_git_metadata(v)
    conf_init.setup_canonical_host(v)
    conf_init.setup_build_python(v)
    conf_init.setup_prefix_and_dirs(v)
    conf_init.setup_version_and_config_args(v)
    conf_init.setup_pkg_config(v)
    conf_init.setup_missing_stdlib_config(v)

    conf_platform.setup_machdep(v)
    conf_platform.setup_host_prefix(v)
    conf_macos.setup_ios_cross_tools(v)
    conf_macos.setup_universalsdk(v)
    conf_macos.setup_framework_name(v)
    conf_macos.setup_framework(v)
    conf_macos.setup_app_store_compliance(v)
    conf_platform.setup_host_platform(v)

    conf_platform.setup_xopen_source(v)
    conf_macos.setup_deployment_targets_and_flags(v)
    conf_macos.setup_macos_compiler(v)
    conf_compiler.setup_compiler_detection(v)
    conf_compiler.setup_cxx(v)
    conf_platform.setup_platform_triplet(v)
    conf_compiler.setup_stack_direction(v)
    conf_platform.setup_pep11_tier(v)
    conf_targets.setup_android_api(v)
    conf_wasm.setup_wasm_options(v)
    conf_targets.setup_exe_suffix(v)
    conf_targets.setup_library_names(v)
    conf_targets.setup_shared_options(v)
    conf_targets.setup_ldlibrary(v)
    conf_targets.setup_hostrunner(v)
    conf_targets.setup_library_deps(v)
    conf_targets.setup_build_tools(v)

    conf_buildopts.setup_disable_gil(v)
    conf_buildopts.setup_pydebug(v)
    conf_buildopts.setup_trace_refs(v)
    conf_buildopts.setup_pystats(v)
    conf_buildopts.setup_assertions(v)
    conf_buildopts.setup_optimizations(v)
    conf_optimization.setup_profile_task(v)
    conf_optimization.locate_llvm_bin(v)
    conf_optimization.setup_lto(v)
    conf_optimization.setup_pgo_flags(v)
    conf_optimization.setup_bolt(v)
    conf_compiler.setup_strict_overflow(v)
    conf_compiler.setup_opt_and_debug_cflags(v)
    conf_wasm.setup_wasm_flags(v)
    conf_compiler.setup_basecflags_overflow(v)
    conf_compiler.setup_safety_options(v)
    conf_compiler.setup_gcc_warnings(v)

    conf_buildopts.setup_experimental_jit(v)
    conf_threads.check_pthreads(v)
    conf_platform.check_headers(v)
    conf_platform.check_types_and_macros(v)
    conf_compiler.check_sizes(v)
    conf_macos.setup_next_framework(v)
    conf_macos.setup_dsymutil(v)
    conf_macos.check_dyld(v)
    conf_buildopts.setup_sanitizers(v)
    conf_sharedlib.setup_shared_lib_suffix(v)
    conf_sharedlib.setup_ldshared(v)
    conf_sharedlib.setup_ccshared(v)
    conf_sharedlib.setup_linkforshared(v)
    conf_sharedlib.setup_shared_lib_exports(v)
    conf_buildopts.setup_perf_trampoline(v)
    conf_syslibs.check_base_libraries(v)
    conf_extlibs.detect_uuid(v)
    conf_syslibs.check_remaining_libs(v)
    conf_security.setup_hash_algorithm()
    conf_syslibs.setup_tzpath(v)
    conf_net.check_network_libs(v)

    conf_extlibs.setup_expat(v)
    conf_extlibs.detect_libffi(v)
    conf_math.detect_libmpdec(v)
    conf_extlibs.detect_sqlite3(v)
    conf_extlibs.detect_tcltk(v)
    conf_syslibs.detect_gdbm(v)
    conf_syslibs.detect_dbm(v)
    conf_threads.setup_pthreads(v)
    conf_net.check_ipv6(v)
    conf_net.check_can_sockets(v)
    conf_buildopts.setup_doc_strings()
    conf_compiler.check_stdatomic(v)
    conf_buildopts.setup_mimalloc(v)
    conf_buildopts.setup_pymalloc(v)
    conf_platform.setup_c_locale_coercion()
    conf_buildopts.setup_valgrind(v)
    conf_buildopts.setup_dtrace(v)
    conf_wasm.setup_platform_objects(v)
    conf_sharedlib.setup_dynload(v)
    conf_platform.check_posix_functions(v)
    conf_platform.check_special_functions(v)

    conf_extlibs.check_compression_libraries(v)
    conf_net.check_netdb_socket_funcs(v)
    conf_platform.check_declarations(v)
    conf_platform.check_pty_and_misc_funcs(v)
    conf_platform.check_clock_functions(v)
    conf_filesystem.check_device_macros(v)
    conf_net.check_getaddrinfo(v)
    conf_platform.check_structs(v)
    conf_compiler.check_compiler_characteristics(v)
    conf_net.check_gethostbyname_r(v)
    conf_math.check_math_library(v)
    conf_math.check_gcc_asm_and_floating_point(v)
    conf_math.check_c99_math(v)
    conf_threads.check_posix_semaphores(v)
    conf_math.check_big_digits(v)
    conf_math.check_wchar(v)
    conf_platform.check_endianness_and_soabi(v)
    conf_modules.setup_module_deps(v)
    conf_paths.setup_install_paths(v)
    conf_compiler.check_sign_extension_and_getc(v)
    conf_extlibs.check_readline(v)
    conf_paths.check_misc_runtime(v)
    conf_syslibs.check_stat_timestamps(v)

    conf_extlibs.check_curses(v)
    conf_filesystem.check_device_files(v)
    conf_net.check_socklen_t(v)
    conf_compiler.check_mbstowcs(v)
    conf_compiler.check_computed_gotos(v)
    conf_buildopts.check_tail_call_interp(v)
    conf_buildopts.check_remote_debug(v)
    conf_platform.check_aix_pipe_buf(v)
    conf_threads.setup_thread_headers_and_srcdirs(v)
    conf_compiler.check_compiler_bugs(v)
    conf_buildopts.check_ensurepip(v)
    conf_filesystem.check_dirent(v)
    conf_security.check_getrandom(v)
    conf_platform.check_posix_shmem(v)
    conf_security.check_openssl(v)
    conf_security.check_ssl_cipher_suites(v)
    conf_security.check_builtin_hashlib_hashes(v)
    conf_modules.check_test_modules(v)
    conf_syslibs.check_libatomic(v)
    conf_threads.check_thread_name_maxlen(v)
    conf_modules.setup_platform_na_modules(v)
    conf_modules.setup_stdlib_modules(v)
    conf_security.setup_hacl(v)
    conf_modules.setup_remaining_modules(v)
    conf_buildopts.check_jit_stencils(v)
    conf_output.generate_output(v)


def main() -> None:
    pyconf.init_args()
    if pyconf.check_help():
        return
    v = pyconf.vars
    _run(v)


if __name__ == "__main__":
    main()
