package org.python.testbed

import android.content.Context
import android.os.*
import android.system.*
import android.widget.TextView
import androidx.appcompat.app.*
import java.io.*


// Launching the tests from an activity is OK for a quick check, but for
// anything more complicated it'll be more convenient to use `android.py test`
// to launch the tests via PythonSuite.
class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        val status = PythonTestRunner(this).run("-W -uall")
        findViewById<TextView>(R.id.tvHello).text = "Exit status $status"
    }
}


class PythonTestRunner(val context: Context) {
    /** @param args Extra arguments for `python -m test`.
     * @return The Python exit status: zero if the tests passed, nonzero if
     * they failed. */
    fun run(args: String = "") : Int {
        Os.setenv("PYTHON_ARGS", args, true)

        // Python needs this variable to help it find the temporary directory,
        // but Android only sets it on API level 33 and later.
        Os.setenv("TMPDIR", context.cacheDir.toString(), false)

        val pythonHome = extractAssets()
        System.loadLibrary("main_activity")
        redirectStdioToLogcat()
        setupSignals()

        // The main module is in src/main/python/main.py.
        return runPython(pythonHome.toString(), "main")
    }

    private fun extractAssets() : File {
        val pythonHome = File(context.filesDir, "python")
        if (pythonHome.exists() && !pythonHome.deleteRecursively()) {
            throw RuntimeException("Failed to delete $pythonHome")
        }
        extractAssetDir("python", context.filesDir)
        return pythonHome
    }

    private fun extractAssetDir(path: String, targetDir: File) {
        val names = context.assets.list(path)
            ?: throw RuntimeException("Failed to list $path")
        val targetSubdir = File(targetDir, path)
        if (!targetSubdir.mkdirs()) {
            throw RuntimeException("Failed to create $targetSubdir")
        }

        for (name in names) {
            val subPath = "$path/$name"
            val input: InputStream
            try {
                input = context.assets.open(subPath)
            } catch (e: FileNotFoundException) {
                extractAssetDir(subPath, targetDir)
                continue
            }
            input.use {
                File(targetSubdir, name).outputStream().use { output ->
                    input.copyTo(output)
                }
            }
        }
    }

    // Some tests use SIGUSR1, but Android blocks that by default in order to
    // make it available to `sigwait` in the SignalCatcher thread
    // (https://cs.android.com/android/platform/superproject/+/android14-qpr3-release:art/runtime/signal_catcher.cc).
    // That thread is only needed for debugging the JVM, so disabling it should
    // not weaken the tests.
    //
    // Simply unblocking SIGUSR1 is enough to fix simple tests, but in tests
    // that involve multiple different signals in quick succession (e.g.
    // test_stress_delivery_simultaneous), it's possible for SIGUSR1 to arrive
    // while the main thread is running the C-level handler for a different
    // signal, in which case the SIGUSR1 may be consumed by the SignalCatcher
    // thread instead.
    //
    // Even if there are other threads with the signal unblocked, it looks like
    // these don't have any priority over the `sigwait` – only the main thread
    // is special-cased (see `complete_signal` and `do_sigtimedwait` in
    // kernel/signal.c). So the only reliable solution is to stop the
    // SignalCatcher.
    private fun setupSignals() {
        val tid = getSignalCatcherTid()
        if (tid == 0) {
            System.err.println(
                "Failed to detect SignalCatcher - signal tests may be unreliable"
            )
        } else {
            // Small delay to make sure the target thread is idle.
            Thread.sleep(100);

            // This is potentially dangerous, so it's worth always logging here
            // in case it causes a deadlock or crash.
            System.err.println("Killing SignalCatcher TID $tid")
            killThread(tid);
        }
        unblockSignal(OsConstants.SIGUSR1)
    }

    // Determine the SignalCatcher's thread ID by sending a signal and waiting
    // for it to write a log message.
    private fun getSignalCatcherTid() : Int {
        sendSignal(OsConstants.SIGUSR1)

        val deadline = System.currentTimeMillis() + 1000
        try {
            while (System.currentTimeMillis() < deadline) {
                ProcessBuilder(
                    // --pid requires API level 24 or higher.
                    "logcat", "-d", "--pid", Os.getpid().toString()
                ).start().inputStream.reader().useLines {
                    var tid = 0;
                    for (line in it) {
                        val fields = line.split("""\s+""".toRegex(), 6)
                        if (fields.size != 6) {
                            continue
                        }
                        if (fields[5].contains("reacting to signal")) {
                            tid = fields[3].toInt()
                        }

                        // SIGUSR1 starts a Java garbage collection, so wait for
                        // a second message indicating that has completed.
                        if (
                            tid != 0 && fields[3].toInt() == tid
                            && fields[5].contains("GC freed")
                        ) {
                            return tid
                        }
                    }
                }
                Thread.sleep(100)
            }
        } catch (e: IOException) {
            // This may happen on ARM64 emulators with API level < 23, where
            // SELinux blocks apps from reading their own logs.
            e.printStackTrace()
        }
        return 0;
    }

    // Native functions are implemented in main_activity.c.
    private external fun redirectStdioToLogcat()
    private external fun sendSignal(sig: Int)
    private external fun killThread(tid: Int)
    private external fun unblockSignal(sig: Int)
    private external fun runPython(home: String, runModule: String) : Int
}
