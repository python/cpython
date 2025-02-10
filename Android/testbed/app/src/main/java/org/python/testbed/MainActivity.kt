package org.python.testbed

import android.content.Context
import android.os.*
import android.system.Os
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

    private external fun redirectStdioToLogcat()
    private external fun runPython(home: String, runModule: String) : Int
}
