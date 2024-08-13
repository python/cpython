package org.python.testbed

import androidx.test.annotation.UiThreadTest
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.ext.junit.runners.AndroidJUnit4

import org.junit.Test
import org.junit.runner.RunWith

import org.junit.Assert.*


@RunWith(AndroidJUnit4::class)
class PythonSuite {
    @Test
    @UiThreadTest
    fun testPython() {
        val start = System.currentTimeMillis()
        try {
            val context =
                InstrumentationRegistry.getInstrumentation().targetContext
            val args =
                InstrumentationRegistry.getArguments().getString("pythonArgs", "")
            val status = PythonTestRunner(context).run(args)
            assertEquals(0, status)
        } finally {
            // Make sure the process lives long enough for the test script to
            // detect it (see `find_pid` in android.py).
            val delay = 2000 - (System.currentTimeMillis() - start)
            if (delay > 0) {
                Thread.sleep(delay)
            }
        }
    }
}
