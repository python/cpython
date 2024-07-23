package org.python.testbed

import android.os.*
import android.system.Os
import android.widget.TextView
import androidx.appcompat.app.*
import java.io.*

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Python needs this variable to help it find the temporary directory,
        // but Android only sets it on API level 33 and later.
        Os.setenv("TMPDIR", cacheDir.toString(), false)

        val pythonHome = extractAssets()
        System.loadLibrary("main_activity")
        redirectStdioToLogcat()
        runPython(pythonHome.toString(), "main")
        findViewById<TextView>(R.id.tvHello).text = "Python complete"
    }

    private fun extractAssets() : File {
        val pythonHome = File(filesDir, "python")
        if (pythonHome.exists() && !pythonHome.deleteRecursively()) {
            throw RuntimeException("Failed to delete $pythonHome")
        }
        extractAssetDir("python", filesDir)
        return pythonHome
    }

    private fun extractAssetDir(path: String, targetDir: File) {
        val names = assets.list(path)
            ?: throw RuntimeException("Failed to list $path")
        val targetSubdir = File(targetDir, path)
        if (!targetSubdir.mkdirs()) {
            throw RuntimeException("Failed to create $targetSubdir")
        }

        for (name in names) {
            val subPath = "$path/$name"
            val input: InputStream
            try {
                input = assets.open(subPath)
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
    private external fun runPython(home: String, runModule: String)
}