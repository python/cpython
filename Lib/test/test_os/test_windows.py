import sys
import unittest

if sys.platform != "win32":
    raise unittest.SkipTest("Win32 specific tests")

import _winapi
import fnmatch
import mmap
import os
import shutil
import signal
import stat
import subprocess
import textwrap
import time
import uuid
from test import support
from test.support import import_helper
from test.support import os_helper
from .utils import create_file


class Win32KillTests(unittest.TestCase):
    def _kill(self, sig):
        # Start sys.executable as a subprocess and communicate from the
        # subprocess to the parent that the interpreter is ready. When it
        # becomes ready, send *sig* via os.kill to the subprocess and check
        # that the return code is equal to *sig*.
        import ctypes
        from ctypes import wintypes
        import msvcrt

        # Since we can't access the contents of the process' stdout until the
        # process has exited, use PeekNamedPipe to see what's inside stdout
        # without waiting. This is done so we can tell that the interpreter
        # is started and running at a point where it could handle a signal.
        PeekNamedPipe = ctypes.windll.kernel32.PeekNamedPipe
        PeekNamedPipe.restype = wintypes.BOOL
        PeekNamedPipe.argtypes = (wintypes.HANDLE, # Pipe handle
                                  ctypes.POINTER(ctypes.c_char), # stdout buf
                                  wintypes.DWORD, # Buffer size
                                  ctypes.POINTER(wintypes.DWORD), # bytes read
                                  ctypes.POINTER(wintypes.DWORD), # bytes avail
                                  ctypes.POINTER(wintypes.DWORD)) # bytes left
        msg = "running"
        proc = subprocess.Popen([sys.executable, "-c",
                                 "import sys;"
                                 "sys.stdout.write('{}');"
                                 "sys.stdout.flush();"
                                 "input()".format(msg)],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                stdin=subprocess.PIPE)
        self.addCleanup(proc.stdout.close)
        self.addCleanup(proc.stderr.close)
        self.addCleanup(proc.stdin.close)

        count, max = 0, 100
        while count < max and proc.poll() is None:
            # Create a string buffer to store the result of stdout from the pipe
            buf = ctypes.create_string_buffer(len(msg))
            # Obtain the text currently in proc.stdout
            # Bytes read/avail/left are left as NULL and unused
            rslt = PeekNamedPipe(msvcrt.get_osfhandle(proc.stdout.fileno()),
                                 buf, ctypes.sizeof(buf), None, None, None)
            self.assertNotEqual(rslt, 0, "PeekNamedPipe failed")
            if buf.value:
                self.assertEqual(msg, buf.value.decode())
                break
            time.sleep(0.1)
            count += 1
        else:
            self.fail("Did not receive communication from the subprocess")

        os.kill(proc.pid, sig)
        self.assertEqual(proc.wait(), sig)

    def test_kill_sigterm(self):
        # SIGTERM doesn't mean anything special, but make sure it works
        self._kill(signal.SIGTERM)

    def test_kill_int(self):
        # os.kill on Windows can take an int which gets set as the exit code
        self._kill(100)

    @unittest.skipIf(mmap is None, "requires mmap")
    def _kill_with_event(self, event, name):
        tagname = "test_os_%s" % uuid.uuid1()
        m = mmap.mmap(-1, 1, tagname)
        m[0] = 0

        # Run a script which has console control handling enabled.
        script = os.path.join(os.path.dirname(__file__),
                              "win_console_handler.py")
        cmd = [sys.executable, script, tagname]
        proc = subprocess.Popen(cmd,
                                creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)

        with proc:
            # Let the interpreter startup before we send signals. See #3137.
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if proc.poll() is None:
                    break
            else:
                # Forcefully kill the process if we weren't able to signal it.
                proc.kill()
                self.fail("Subprocess didn't finish initialization")

            os.kill(proc.pid, event)

            try:
                # proc.send_signal(event) could also be done here.
                # Allow time for the signal to be passed and the process to exit.
                proc.wait(timeout=support.SHORT_TIMEOUT)
            except subprocess.TimeoutExpired:
                # Forcefully kill the process if we weren't able to signal it.
                proc.kill()
                self.fail("subprocess did not stop on {}".format(name))

    @unittest.skip("subprocesses aren't inheriting Ctrl+C property")
    @support.requires_subprocess()
    def test_CTRL_C_EVENT(self):
        from ctypes import wintypes
        import ctypes

        # Make a NULL value by creating a pointer with no argument.
        NULL = ctypes.POINTER(ctypes.c_int)()
        SetConsoleCtrlHandler = ctypes.windll.kernel32.SetConsoleCtrlHandler
        SetConsoleCtrlHandler.argtypes = (ctypes.POINTER(ctypes.c_int),
                                          wintypes.BOOL)
        SetConsoleCtrlHandler.restype = wintypes.BOOL

        # Calling this with NULL and FALSE causes the calling process to
        # handle Ctrl+C, rather than ignore it. This property is inherited
        # by subprocesses.
        SetConsoleCtrlHandler(NULL, 0)

        self._kill_with_event(signal.CTRL_C_EVENT, "CTRL_C_EVENT")

    @support.requires_subprocess()
    def test_CTRL_BREAK_EVENT(self):
        self._kill_with_event(signal.CTRL_BREAK_EVENT, "CTRL_BREAK_EVENT")


class Win32ListdirTests(unittest.TestCase):
    """Test listdir on Windows."""

    def setUp(self):
        self.created_paths = []
        for i in range(2):
            dir_name = 'SUB%d' % i
            dir_path = os.path.join(os_helper.TESTFN, dir_name)
            file_name = 'FILE%d' % i
            file_path = os.path.join(os_helper.TESTFN, file_name)
            os.makedirs(dir_path)
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write("I'm %s and proud of it. Blame test_os.\n" % file_path)
            self.created_paths.extend([dir_name, file_name])
        self.created_paths.sort()

    def tearDown(self):
        shutil.rmtree(os_helper.TESTFN)

    def test_listdir_no_extended_path(self):
        """Test when the path is not an "extended" path."""
        # unicode
        self.assertEqual(
                sorted(os.listdir(os_helper.TESTFN)),
                self.created_paths)

        # bytes
        self.assertEqual(
                sorted(os.listdir(os.fsencode(os_helper.TESTFN))),
                [os.fsencode(path) for path in self.created_paths])

    def test_listdir_extended_path(self):
        """Test when the path starts with '\\\\?\\'."""
        # See: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#maxpath
        # unicode
        path = '\\\\?\\' + os.path.abspath(os_helper.TESTFN)
        self.assertEqual(
                sorted(os.listdir(path)),
                self.created_paths)

        # bytes
        path = b'\\\\?\\' + os.fsencode(os.path.abspath(os_helper.TESTFN))
        self.assertEqual(
                sorted(os.listdir(path)),
                [os.fsencode(path) for path in self.created_paths])


@unittest.skipUnless(os.name == "nt", "NT specific tests")
class Win32ListdriveTests(unittest.TestCase):
    """Test listdrive, listmounts and listvolume on Windows."""

    def setUp(self):
        # Get drives and volumes from fsutil
        out = subprocess.check_output(
            ["fsutil.exe", "volume", "list"],
            cwd=os.path.join(os.getenv("SystemRoot", "\\Windows"), "System32"),
            encoding="mbcs",
            errors="ignore",
        )
        lines = out.splitlines()
        self.known_volumes = {l for l in lines if l.startswith('\\\\?\\')}
        self.known_drives = {l for l in lines if l[1:] == ':\\'}
        self.known_mounts = {l for l in lines if l[1:3] == ':\\'}

    def test_listdrives(self):
        drives = os.listdrives()
        self.assertIsInstance(drives, list)
        self.assertSetEqual(
            self.known_drives,
            self.known_drives & set(drives),
        )

    def test_listvolumes(self):
        volumes = os.listvolumes()
        self.assertIsInstance(volumes, list)
        self.assertSetEqual(
            self.known_volumes,
            self.known_volumes & set(volumes),
        )

    def test_listmounts(self):
        for volume in os.listvolumes():
            try:
                mounts = os.listmounts(volume)
            except OSError as ex:
                if support.verbose:
                    print("Skipping", volume, "because of", ex)
            else:
                self.assertIsInstance(mounts, list)
                self.assertSetEqual(
                    set(mounts),
                    self.known_mounts & set(mounts),
                )


@os_helper.skip_unless_symlink
class Win32SymlinkTests(unittest.TestCase):
    filelink = 'filelinktest'
    filelink_target = os.path.abspath(__file__)
    dirlink = 'dirlinktest'
    dirlink_target = os.path.dirname(filelink_target)
    missing_link = 'missing link'

    def setUp(self):
        assert os.path.exists(self.dirlink_target)
        assert os.path.exists(self.filelink_target)
        assert not os.path.exists(self.dirlink)
        assert not os.path.exists(self.filelink)
        assert not os.path.exists(self.missing_link)

    def tearDown(self):
        if os.path.exists(self.filelink):
            os.remove(self.filelink)
        if os.path.exists(self.dirlink):
            os.rmdir(self.dirlink)
        if os.path.lexists(self.missing_link):
            os.remove(self.missing_link)

    def test_directory_link(self):
        os.symlink(self.dirlink_target, self.dirlink)
        self.assertTrue(os.path.exists(self.dirlink))
        self.assertTrue(os.path.isdir(self.dirlink))
        self.assertTrue(os.path.islink(self.dirlink))
        self.check_stat(self.dirlink, self.dirlink_target)

    def test_file_link(self):
        os.symlink(self.filelink_target, self.filelink)
        self.assertTrue(os.path.exists(self.filelink))
        self.assertTrue(os.path.isfile(self.filelink))
        self.assertTrue(os.path.islink(self.filelink))
        self.check_stat(self.filelink, self.filelink_target)

    def _create_missing_dir_link(self):
        'Create a "directory" link to a non-existent target'
        linkname = self.missing_link
        if os.path.lexists(linkname):
            os.remove(linkname)
        target = r'c:\\target does not exist.29r3c740'
        assert not os.path.exists(target)
        target_is_dir = True
        os.symlink(target, linkname, target_is_dir)

    def test_remove_directory_link_to_missing_target(self):
        self._create_missing_dir_link()
        # For compatibility with Unix, os.remove will check the
        #  directory status and call RemoveDirectory if the symlink
        #  was created with target_is_dir==True.
        os.remove(self.missing_link)

    def test_isdir_on_directory_link_to_missing_target(self):
        self._create_missing_dir_link()
        self.assertFalse(os.path.isdir(self.missing_link))

    def test_rmdir_on_directory_link_to_missing_target(self):
        self._create_missing_dir_link()
        os.rmdir(self.missing_link)

    def check_stat(self, link, target):
        self.assertEqual(os.stat(link), os.stat(target))
        self.assertNotEqual(os.lstat(link), os.stat(link))

        bytes_link = os.fsencode(link)
        self.assertEqual(os.stat(bytes_link), os.stat(target))
        self.assertNotEqual(os.lstat(bytes_link), os.stat(bytes_link))

    def test_12084(self):
        level1 = os.path.abspath(os_helper.TESTFN)
        level2 = os.path.join(level1, "level2")
        level3 = os.path.join(level2, "level3")
        self.addCleanup(os_helper.rmtree, level1)

        os.mkdir(level1)
        os.mkdir(level2)
        os.mkdir(level3)

        file1 = os.path.abspath(os.path.join(level1, "file1"))
        create_file(file1)

        orig_dir = os.getcwd()
        try:
            os.chdir(level2)
            link = os.path.join(level2, "link")
            os.symlink(os.path.relpath(file1), "link")
            self.assertIn("link", os.listdir(os.getcwd()))

            # Check os.stat calls from the same dir as the link
            self.assertEqual(os.stat(file1), os.stat("link"))

            # Check os.stat calls from a dir below the link
            os.chdir(level1)
            self.assertEqual(os.stat(file1),
                             os.stat(os.path.relpath(link)))

            # Check os.stat calls from a dir above the link
            os.chdir(level3)
            self.assertEqual(os.stat(file1),
                             os.stat(os.path.relpath(link)))
        finally:
            os.chdir(orig_dir)

    @unittest.skipUnless(os.path.lexists(r'C:\Users\All Users')
                            and os.path.exists(r'C:\ProgramData'),
                            'Test directories not found')
    def test_29248(self):
        # os.symlink() calls CreateSymbolicLink, which creates
        # the reparse data buffer with the print name stored
        # first, so the offset is always 0. CreateSymbolicLink
        # stores the "PrintName" DOS path (e.g. "C:\") first,
        # with an offset of 0, followed by the "SubstituteName"
        # NT path (e.g. "\??\C:\"). The "All Users" link, on
        # the other hand, seems to have been created manually
        # with an inverted order.
        target = os.readlink(r'C:\Users\All Users')
        self.assertTrue(os.path.samefile(target, r'C:\ProgramData'))

    def test_buffer_overflow(self):
        # Older versions would have a buffer overflow when detecting
        # whether a link source was a directory. This test ensures we
        # no longer crash, but does not otherwise validate the behavior
        segment = 'X' * 27
        path = os.path.join(*[segment] * 10)
        test_cases = [
            # overflow with absolute src
            ('\\' + path, segment),
            # overflow dest with relative src
            (segment, path),
            # overflow when joining src
            (path[:180], path[:180]),
        ]
        for src, dest in test_cases:
            try:
                os.symlink(src, dest)
            except FileNotFoundError:
                pass
            else:
                try:
                    os.remove(dest)
                except OSError:
                    pass
            # Also test with bytes, since that is a separate code path.
            try:
                os.symlink(os.fsencode(src), os.fsencode(dest))
            except FileNotFoundError:
                pass
            else:
                try:
                    os.remove(dest)
                except OSError:
                    pass

    def test_appexeclink(self):
        root = os.path.expandvars(r'%LOCALAPPDATA%\Microsoft\WindowsApps')
        if not os.path.isdir(root):
            self.skipTest("test requires a WindowsApps directory")

        aliases = [os.path.join(root, a)
                   for a in fnmatch.filter(os.listdir(root), '*.exe')]

        for alias in aliases:
            if support.verbose:
                print()
                print("Testing with", alias)
            st = os.lstat(alias)
            self.assertEqual(st, os.stat(alias))
            self.assertFalse(stat.S_ISLNK(st.st_mode))
            self.assertEqual(st.st_reparse_tag, stat.IO_REPARSE_TAG_APPEXECLINK)
            self.assertTrue(os.path.isfile(alias))
            # testing the first one we see is sufficient
            break
        else:
            self.skipTest("test requires an app execution alias")


class Win32JunctionTests(unittest.TestCase):
    junction = 'junctiontest'
    junction_target = os.path.dirname(os.path.abspath(__file__))

    def setUp(self):
        assert os.path.exists(self.junction_target)
        assert not os.path.lexists(self.junction)

    def tearDown(self):
        if os.path.lexists(self.junction):
            os.unlink(self.junction)

    def test_create_junction(self):
        _winapi.CreateJunction(self.junction_target, self.junction)
        self.assertTrue(os.path.lexists(self.junction))
        self.assertTrue(os.path.exists(self.junction))
        self.assertTrue(os.path.isdir(self.junction))
        self.assertNotEqual(os.stat(self.junction), os.lstat(self.junction))
        self.assertEqual(os.stat(self.junction), os.stat(self.junction_target))

        # bpo-37834: Junctions are not recognized as links.
        self.assertFalse(os.path.islink(self.junction))
        self.assertEqual(os.path.normcase("\\\\?\\" + self.junction_target),
                         os.path.normcase(os.readlink(self.junction)))

    def test_unlink_removes_junction(self):
        _winapi.CreateJunction(self.junction_target, self.junction)
        self.assertTrue(os.path.exists(self.junction))
        self.assertTrue(os.path.lexists(self.junction))

        os.unlink(self.junction)
        self.assertFalse(os.path.exists(self.junction))


class Win32NtTests(unittest.TestCase):
    def test_getfinalpathname_handles(self):
        nt = import_helper.import_module('nt')
        ctypes = import_helper.import_module('ctypes')
        # Ruff false positive -- it thinks we're redefining `ctypes` here
        import ctypes.wintypes  # noqa: F811

        kernel = ctypes.WinDLL('Kernel32.dll', use_last_error=True)
        kernel.GetCurrentProcess.restype = ctypes.wintypes.HANDLE

        kernel.GetProcessHandleCount.restype = ctypes.wintypes.BOOL
        kernel.GetProcessHandleCount.argtypes = (ctypes.wintypes.HANDLE,
                                                 ctypes.wintypes.LPDWORD)

        # This is a pseudo-handle that doesn't need to be closed
        hproc = kernel.GetCurrentProcess()

        handle_count = ctypes.wintypes.DWORD()
        ok = kernel.GetProcessHandleCount(hproc, ctypes.byref(handle_count))
        self.assertEqual(1, ok)

        before_count = handle_count.value

        # The first two test the error path, __file__ tests the success path
        filenames = [
            r'\\?\C:',
            r'\\?\NUL',
            r'\\?\CONIN',
            __file__,
        ]

        for _ in range(10):
            for name in filenames:
                try:
                    nt._getfinalpathname(name)
                except Exception:
                    # Failure is expected
                    pass
                try:
                    os.stat(name)
                except Exception:
                    pass

        ok = kernel.GetProcessHandleCount(hproc, ctypes.byref(handle_count))
        self.assertEqual(1, ok)

        handle_delta = handle_count.value - before_count

        self.assertEqual(0, handle_delta)

    @support.requires_subprocess()
    def test_stat_unlink_race(self):
        # bpo-46785: the implementation of os.stat() falls back to reading
        # the parent directory if CreateFileW() fails with a permission
        # error. If reading the parent directory fails because the file or
        # directory are subsequently unlinked, or because the volume or
        # share are no longer available, then the original permission error
        # should not be restored.
        filename =  os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)
        deadline = time.time() + 5
        command = textwrap.dedent("""\
            import os
            import sys
            import time

            filename = sys.argv[1]
            deadline = float(sys.argv[2])

            while time.time() < deadline:
                try:
                    with open(filename, "w") as f:
                        pass
                except OSError:
                    pass
                try:
                    os.remove(filename)
                except OSError:
                    pass
            """)

        with subprocess.Popen([sys.executable, '-c', command, filename, str(deadline)]) as proc:
            while time.time() < deadline:
                try:
                    os.stat(filename)
                except FileNotFoundError as e:
                    assert e.winerror == 2  # ERROR_FILE_NOT_FOUND
            try:
                proc.wait(1)
            except subprocess.TimeoutExpired:
                proc.terminate()

    @support.requires_subprocess()
    def test_stat_inaccessible_file(self):
        filename = os_helper.TESTFN
        ICACLS = os.path.expandvars(r"%SystemRoot%\System32\icacls.exe")

        with open(filename, "wb") as f:
            f.write(b'Test data')

        stat1 = os.stat(filename)

        try:
            # Remove all permissions from the file
            subprocess.check_output([ICACLS, filename, "/inheritance:r"],
                                    stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as ex:
            if support.verbose:
                print(ICACLS, filename, "/inheritance:r", "failed.")
                print(ex.stdout.decode("oem", "replace").rstrip())
            try:
                os.unlink(filename)
            except OSError:
                pass
            self.skipTest("Unable to create inaccessible file")

        def cleanup():
            # Give delete permission to the owner (us)
            subprocess.check_output([ICACLS, filename, "/grant", "*WD:(D)"],
                                    stderr=subprocess.STDOUT)
            os.unlink(filename)

        self.addCleanup(cleanup)

        if support.verbose:
            print("File:", filename)
            print("stat with access:", stat1)

        # First test - we shouldn't raise here, because we still have access to
        # the directory and can extract enough information from its metadata.
        stat2 = os.stat(filename)

        if support.verbose:
            print(" without access:", stat2)

        # We may not get st_dev/st_ino, so ensure those are 0 or match
        self.assertIn(stat2.st_dev, (0, stat1.st_dev))
        self.assertIn(stat2.st_ino, (0, stat1.st_ino))

        # st_mode and st_size should match (for a normal file, at least)
        self.assertEqual(stat1.st_mode, stat2.st_mode)
        self.assertEqual(stat1.st_size, stat2.st_size)

        # st_ctime and st_mtime should be the same
        self.assertEqual(stat1.st_ctime, stat2.st_ctime)
        self.assertEqual(stat1.st_mtime, stat2.st_mtime)

        # st_atime should be the same or later
        self.assertGreaterEqual(stat1.st_atime, stat2.st_atime)


if __name__ == "__main__":
    unittest.main()
