package org.python.testbed

import android.content.Context
import android.os.Bundle
import androidx.test.annotation.UiThreadTest
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.json.JSONArray

import org.junit.Test
import org.junit.runner.RunWith

import org.junit.Assert.*

import java.io.File


@RunWith(AndroidJUnit4::class)
class PythonSuite {
    @Test
    @UiThreadTest
    fun testPython() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val args = InstrumentationRegistry.getArguments()
        val start = System.currentTimeMillis()
        try {
            val status = PythonTestRunner(instrumentation.targetContext).run(
                args.getString("pythonArgs")!!,
            )
            assertEquals(0, status)
        } finally {
            // Copy files requested via --pull to the directory that AGP/UTP
            // injected as `additionalTestOutputDir`. AGP will pull everything
            // written there back to the host before shutting down the emulator.
            copyOutputFiles(instrumentation.targetContext, args)

            // Make sure the process lives long enough for the test script to
            // detect it (see `find_pid` in android.py).
            val delay = 2000 - (System.currentTimeMillis() - start)
            if (delay > 0) {
                Thread.sleep(delay)
            }
        }
    }

    private fun copyOutputFiles(context: Context, args: Bundle) {
        // A list of file paths (relative to the Python working directory) that should be
        // copied back to the host after the test finishes.
        val pullPathsJson = args.getString("pythonPull") ?: return

        // The output directory is created by AGP/UTP and points to a location inside
        // the emulator's filesystem. AGP/UTP will pull everything from there back
        // to the host after the test finishes.
        val outputDir = args.getString("additionalTestOutputDir") ?: return

        // Pack all files into a single zip archive to avoid issues because the UTP copy
        // skips dotfiles like ".coverage".
        val archiveFile = File(outputDir, "org.python.testbed-output.zip")
        val srcBase = File(context.filesDir, "python/cwd")
        val paths = JSONArray(pullPathsJson)
        java.util.zip.ZipOutputStream(archiveFile.outputStream().buffered()).use { zip ->
            for (i in 0 until paths.length()) {
                val src = File(srcBase, paths.getString(i))
                if (!src.exists()) {
                    android.util.Log.w("python.stderr", "Pull path not found: $src\n")
                    continue
                }
                try {
                    addToZip(zip, src, src.name)
                } catch (e: Exception) {
                    android.util.Log.e("python.stderr", "Failed to zip $src: $e\n")
                }
            }
        }
        android.util.Log.i("python.stdout", "Created output archive: $archiveFile\n")
    }

    private fun addToZip(
        zip: java.util.zip.ZipOutputStream,
        file: File,
        entryName: String,
    ) {
        if (file.isDirectory) {
            for (child in file.listFiles() ?: emptyArray()) {
                addToZip(zip, child, "$entryName/${child.name}")
            }
        } else {
            zip.putNextEntry(java.util.zip.ZipEntry(entryName))
            file.inputStream().use { it.copyTo(zip) }
            zip.closeEntry()
        }
    }
}
