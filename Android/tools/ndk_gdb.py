#!/usr/bin/env python2

"""Wrapper around the Android ndk-gdb.py script."""

import sys
import os
import re
import importlib
import atexit
import tempfile
from os.path import join, basename

pgm = basename(sys.argv[0])
if sys.version_info >= (3,):
    raise NotImplementedError('%s does not run on Python3.' % pgm)

# Import the Android ndk-gdb module.
opsys_info = os.uname()
sys.path.append(join(os.environ['ANDROID_NDK_ROOT'], 'prebuilt',
                '%s-%s' % (opsys_info[0].lower(), opsys_info[4]),
                'bin'))
ndk_gdb = importlib.import_module('ndk-gdb')
_generate_gdb_script = ndk_gdb.generate_gdb_script

def start_gdbserver(device, gdbserver_local_path, gdbserver_remote_path,
                    target_pid, run_cmd, debug_socket, port, user=None):
    """Start gdbserver in the background and forward necessary ports."""

    assert target_pid is None or run_cmd is None

    # Push gdbserver to the target.
    if gdbserver_local_path is not None:
        device.push(gdbserver_local_path, gdbserver_remote_path)

    # start_gdbserver() is monkey-patched because, starting with API level 24,
    # the 'shell' user cannot bind to a unix socket anymore so we use a tcp
    # socket instead since 'adbd', the daemon running on the emulator that
    # does the port forwarding, runs as a 'shell' user.
    debug_socket = '5040'
    gdbserver_cmd = [gdbserver_remote_path, "--once",
                     ":{}".format(debug_socket)]

    if target_pid is not None:
        gdbserver_cmd += ["--attach", str(target_pid)]
    else:
        gdbserver_cmd += run_cmd

    device.forward("tcp:{}".format(port), "tcp:{}".format(debug_socket))
    atexit.register(lambda: device.forward_remove("tcp:{}".format(port)))
    gdbserver_cmd = ["su", "0"] + gdbserver_cmd

    gdbserver_output_path = join(tempfile.gettempdir(), "gdbclient.log")
    print("Redirecting gdbserver output to {}".format(gdbserver_output_path))
    gdbserver_output = file(gdbserver_output_path, 'w')
    return device.shell_popen(gdbserver_cmd, stdout=gdbserver_output,
                              stderr=gdbserver_output)

def find_project(args):
    return join(os.environ['DIST_DIR'], 'gdb',
                'android-%(ANDROID_API)s-%(ANDROID_ARCH)s' % os.environ)

def get_app_data_dir(args, package_name):
    return os.environ['SYS_EXEC_PREFIX']

def get_gdbserver_path(args, package_name, app_data_dir, arch):
    app_gdbserver_path = "{}/lib/gdbserver".format(app_data_dir)
    cmd = ["ls", app_gdbserver_path, "2>/dev/null"]
    cmd = ndk_gdb.gdbrunner.get_run_as_cmd(package_name, cmd)
    (rc, _, _) = args.device.shell_nocheck(cmd)
    if rc == 0:
        ndk_gdb.log("Found app gdbserver: {}".format(app_gdbserver_path))
        return app_gdbserver_path

    # We need to upload our gdbserver
    ndk_gdb.log("App gdbserver not found at {}, uploading.".format(app_gdbserver_path))
    local_path = "{}/prebuilt/android-{}/gdbserver/gdbserver"
    local_path = local_path.format(ndk_gdb.NDK_PATH, arch)
    remote_path = "/data/local/tmp/{}-gdbserver".format(arch)
    args.device.push(local_path, remote_path)

    # get_run_as_cmd() is monkey-patched to avoid copying gdbserver to the
    # data directory: '"run-as", package_name' fails because package_name is
    # not a valid application on the device.

    ndk_gdb.log("Uploaded gdbserver to {}".format(remote_path))
    return remote_path

_make_variables = {
    'APP_ABI': os.environ['APP_ABI'],
    'TARGET_OUT': ('./obj/local/%s' % os.environ['ANDROID_ARCH']),
    'APP_STL': 'system',
}
def dump_var(args, variable, abi=None):
    return _make_variables[variable]

_py_srcdir = None
_SOLIB_SEARCH_PATH = r'^\s*(?P<cmd>set\s+solib-search-path)\s*(?P<arg>\S+)\s*$'
_re_solibpath = re.compile(_SOLIB_SEARCH_PATH)

def generate_gdb_script(args, sysroot, binary_path, app_64bit,
                        connect_timeout=5):
    assert _py_srcdir is not None
    gdb_commands = _generate_gdb_script(args, sysroot, binary_path, app_64bit)
    cmds = gdb_commands.split('\n')

    # Add Python symbols to solib_search_path.
    for idx, line in enumerate(cmds):
        matchobj = _re_solibpath.match(line)
        if matchobj:
            cmds[idx] = ('set solib-search-path %s:%s:%s' %
                            (matchobj.group('arg'), os.environ['PY_HOST_DIR'],
                             os.environ['LIB_DYNLOAD']))
            break

    if os.environ.get('GDB_PYTHON') == 'yes':
        cmds.append('python import sys; ' +
                    'sys.path.append("%s/Tools/gdb"); ' % _py_srcdir +
                    'import libpython')

    if os.environ.get('GDB_LOGGING') == 'yes':
        cmds.append('set logging file gdb.log')
        cmds.append('set logging overwrite off')
        cmds.append('set logging on')

    # Workaround SIGILL in __dl_notify_gdb_of_libraries when a library is
    # loaded on armv7.
    if os.environ.get('GDB_SIGILL') == 'yes':
        cmds.append('break __dl_rtld_db_dlactivity')
        cmds.append('commands')
        cmds.append('silent')
        cmds.append('return')
        cmds.append('sharedlibrary')
        cmds.append('continue')
        cmds.append('end')

    cmds.append('directory %s' % _py_srcdir)
    return '\n'.join(cmds)

def main(api_level, src_dir, pgm_name):
    global _py_srcdir
    _py_srcdir = src_dir

    # Monkey-patch the ndk-gdb module.
    ndk_gdb.find_project = find_project
    ndk_gdb.get_app_data_dir = get_app_data_dir
    ndk_gdb.dump_var = dump_var
    ndk_gdb.generate_gdb_script = generate_gdb_script
    ndk_gdb.get_gdbserver_path = get_gdbserver_path
    ndk_gdb.gdbrunner.start_gdbserver = start_gdbserver

    sys.argv = [pgm, '--verbose',  '--adb', os.environ['ADB'],
                '--attach', pgm_name, '--stdcxx-py-pr', 'none']
    ndk_gdb.main()

def usage():
    print('Usage: %s api_level py_srcdir program_name_to_attach_to' % pgm)
    sys.exit(2)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        usage()
    main(*sys.argv[1:])
