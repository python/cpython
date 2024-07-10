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
        val context = InstrumentationRegistry.getInstrumentation().targetContext
        val args = InstrumentationRegistry.getArguments().getString("pythonArgs", "")
        val status = PythonTestRunner(context).run(args)
        assertEquals(0, status)
    }
}