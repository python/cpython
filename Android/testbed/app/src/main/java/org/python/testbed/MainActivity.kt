package org.python.testbed

import android.content.Context
import android.os.*
import android.system.Os
import android.widget.TextView
import androidx.appcompat.app.*
import org.json.JSONArray
import java.io.*


// Launching the tests from an activity is OK for a quick check, but for
// anything more complicated it'll be more convenient to use `android.py test`
// to launch the tests via PythonSuite.
class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        val status = PythonTestRunner(this).run("""["-m", "test", "-W", "-uall"]""")
        findViewById<TextView>(R.id.tvHello).text = "Exit status $status"
    }
}


class PythonTestRunner(val context: Context) {
    /** Run Python.
     *
     * @param args Python command-line, encoded as JSON.
     * @return The Python exit status: zero on success, nonzero on failure. */
    fun run(args: String) : Int {
        // We leave argument 0 as an empty string, which is a placeholder for the
        // executable name in embedded mode.
        val argsJsonArray = JSONArray(args)
        val argsStringArray = Array<String>(argsJsonArray.length() + 1) { it -> ""}
        for (i in 0..<argsJsonArray.length()) {
            argsStringArray[i + 1] = argsJsonArray.getString(i)
        }

        // Python needs this variable to help it find the temporary directory,
        // but Android only sets it on API level 33 and later.
        Os.setenv("TMPDIR", context.cacheDir.toString(), false)

        val pythonHome = extractAssets()
        System.loadLibrary("main_activity")
        redirectStdioToLogcat()
        return runPython(pythonHome.toString(), argsStringArray)
    }

    private fun extractAssets() : File {
        val pythonHome = File(context.filesDir, "python")
        if (pythonHome.exists() && !pythonHome.deleteRecursively()) {
            throw RuntimeException("Failed to delete $pythonHome")
        }
        extractAssetDir("python", context.filesDir)

        // Empty directories are lost in the asset packing/unpacking process.
        val cwd = File(pythonHome, "cwd")
        if (!cwd.exists()) {
            cwd.mkdir()
        }

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
                // Undo the .gz workaround from build.gradle.kts.
                val outputName = name.replace(Regex("""(.*)-"""), "$1")
                File(targetSubdir, outputName).outputStream().use { output ->
                    input.copyTo(output)
                }
            }
        }
    }

    private external fun redirectStdioToLogcat()
    private external fun runPython(home: String, args: Array<String>) : Int
}
