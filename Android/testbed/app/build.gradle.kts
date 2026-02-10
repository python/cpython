import com.android.build.api.variant.*
import kotlin.math.max
import java.io.File

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val requestedTaskNames = gradle.startParameter.taskNames
    .filterNot { it.startsWith("-") }
    .map { it.substringAfterLast(":") }
val isEmulatorSetupOnly = requestedTaskNames.isNotEmpty() && requestedTaskNames.all {
    it == "help" ||
        it == "tasks" ||
        it.endsWith("Setup") ||
        it.endsWith("PatchAvdRam")
}

val ANDROID_DIR = file("../..")
val PYTHON_DIR = ANDROID_DIR.parentFile!!
val PYTHON_CROSS_DIR = file("$PYTHON_DIR/cross-build")
val inSourceTree = (
    ANDROID_DIR.name == "Android" && file("$PYTHON_DIR/pyconfig.h.in").exists()
)

val KNOWN_ABIS = mapOf(
    "aarch64-linux-android" to "arm64-v8a",
    "x86_64-linux-android" to "x86_64",
)

// Discover prefixes.
val prefixes = ArrayList<File>()
lateinit var pythonVersion: String
var abis = HashMap<File, String>()
if (!isEmulatorSetupOnly) {
    if (inSourceTree) {
        for ((triplet, _) in KNOWN_ABIS.entries) {
            val prefix = file("$PYTHON_CROSS_DIR/$triplet/prefix")
            if (prefix.exists()) {
                prefixes.add(prefix)
            }
        }
    } else {
        // Testbed is inside a release package.
        val prefix = file("$ANDROID_DIR/prefix")
        if (prefix.exists()) {
            prefixes.add(prefix)
        }
    }
    if (prefixes.isEmpty()) {
        throw GradleException(
            "No Android prefixes found: see README.md for testing instructions"
        )
    }

    // Detect Python versions and ABIs.
    for ((i, prefix) in prefixes.withIndex()) {
        val libDir = file("$prefix/lib")
        val version = run {
            for (filename in libDir.list()!!) {
                """python(\d+\.\d+[a-z]*)""".toRegex().matchEntire(filename)?.let {
                    return@run it.groupValues[1]
                }
            }
            throw GradleException("Failed to find Python version in $libDir")
        }
        if (i == 0) {
            pythonVersion = version
        } else if (pythonVersion != version) {
            throw GradleException(
                "${prefixes[0]} is Python $pythonVersion, but $prefix is Python $version"
            )
        }

        val libPythonDir = file("$libDir/python$pythonVersion")
        val triplet = run {
            for (filename in libPythonDir.list()!!) {
                """_sysconfigdata_[a-z]*_android_(.+).py""".toRegex()
                    .matchEntire(filename)?.let {
                        return@run it.groupValues[1]
                    }
            }
            throw GradleException("Failed to find Python triplet in $libPythonDir")
        }
        abis[prefix] = KNOWN_ABIS[triplet]!!
    }
}


android {
    val androidEnvFile = file("../../android-env.sh").absoluteFile

    namespace = "org.python.testbed"
    compileSdk = 35

    defaultConfig {
        applicationId = "org.python.testbed"

        minSdk = androidEnvFile.useLines {
            for (line in it) {
                """ANDROID_API_LEVEL:=(\d+)""".toRegex().find(line)?.let {
                    return@useLines it.groupValues[1].toInt()
                }
            }
            throw GradleException("Failed to find API level in $androidEnvFile")
        }

        // This controls the API level of the maxVersion managed emulator, which is used
        // by CI and cibuildwheel.
        //  * 33 has excessive buffering in the logcat client
        //    (https://cs.android.com/android/_/android/platform/system/logging/+/d340721894f223327339010df59b0ac514308826).
        //  * 34 consumes too much disk space on GitHub Actions (#142289).
        //  * 35 has issues connecting to the internet (#142387).
        //  * 36 and later are not available as aosp_atd images yet.
        targetSdk = 32

        versionCode = 1
        versionName = "1.0"

        if (!isEmulatorSetupOnly) {
            ndk.abiFilters.addAll(abis.values)
            externalNativeBuild.cmake.arguments(
                "-DPYTHON_PREFIX_DIR=" + if (inSourceTree) {
                    // AGP uses the ${} syntax for its own purposes, so use a Jinja style
                    // placeholder.
                    "$PYTHON_CROSS_DIR/{{triplet}}/prefix"
                } else {
                    prefixes[0]
                },
                "-DPYTHON_VERSION=$pythonVersion",
                "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON",
            )
        }

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    ndkVersion = androidEnvFile.useLines {
        for (line in it) {
            """ndk_version=(\S+)""".toRegex().find(line)?.let {
                return@useLines it.groupValues[1]
            }
        }
        throw GradleException("Failed to find NDK version in $androidEnvFile")
    }
    if (!isEmulatorSetupOnly) {
        externalNativeBuild.cmake {
            path("src/main/c/CMakeLists.txt")
        }
    }

    // Set this property to something nonexistent but non-empty. Otherwise it'll use the
    // default list, which ignores asset directories beginning with an underscore, and
    // maybe also other files required by tests.
    aaptOptions.ignoreAssetsPattern = "android-testbed-dont-ignore-anything"

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    kotlinOptions {
        jvmTarget = "1.8"
    }

    testOptions {
        managedDevices {
            localDevices {
                create("minVersion") {
                    device = "Small Phone"

                    // Managed devices have a minimum API level of 27.
                    apiLevel = max(27, defaultConfig.minSdk!!)

                    // ATD devices are smaller and faster, but have a minimum
                    // API level of 30.
                    systemImageSource = if (apiLevel >= 30) "aosp-atd" else "aosp"
                }

                create("maxVersion") {
                    device = "Small Phone"
                    apiLevel = defaultConfig.targetSdk!!
                    systemImageSource = "aosp-atd"
                }
            }

            // If the previous test run succeeded and nothing has changed,
            // Gradle thinks there's no need to run it again. Override that.
            afterEvaluate {
                val managedDeviceNames = localDevices.names

                // Ensure the emulator has enough RAM to run the CPython test suite.
                // As of Android Studio Koala / AGP 8.x, Gradle Managed Devices may
                // create an AVD with 2 GB even when the device profile specifies
                // more. Patch the AVD config after the *Setup task creates it.
                managedDeviceNames.forEach { deviceName ->
                    val patchTaskName = "${deviceName}PatchAvdRam"
                    val patchTask = tasks.register(patchTaskName) {
                        group = "verification"
                        description = "Patch Gradle managed device AVD RAM to 4 GB for '$deviceName'."

                        // Ensure the AVD exists before we try to patch it.
                        dependsOn("${deviceName}Setup")

                        doLast {
                            val ramMb = 4096
                            val avdHome = resolveAvdHome()
                            if (!avdHome.exists()) {
                                throw GradleException(
                                    "AVD home directory not found: $avdHome (set ANDROID_AVD_HOME or ANDROID_USER_HOME)"
                                )
                            }

                            val avdDir = findManagedDeviceAvdDirectory(avdHome, deviceName)
                                ?: throw GradleException(
                                    "Failed to locate AVD directory for '$deviceName' under $avdHome (including gradle-managed)"
                                )

                            logger.lifecycle(
                                "Patching AVD RAM for '$deviceName': avdHome=$avdHome, avdDir=$avdDir, ramMb=$ramMb"
                            )

                            val configIni = File(avdDir, "config.ini")
                            val hardwareQemuIni = File(avdDir, "hardware-qemu.ini")

                            val changedConfig = setIniProperty(
                                configIni, "hw.ramSize", ramMb.toString()
                            )
                            val changedHardware = setIniProperty(
                                hardwareQemuIni, "hw.ramSize", ramMb.toString()
                            )

                            logger.lifecycle(
                                "AVD RAM patch for '$deviceName': config.ini=" +
                                    (if (changedConfig) "updated" else "unchanged") +
                                    ", hardware-qemu.ini=" +
                                    (if (changedHardware) "updated" else "unchanged")
                            )
                        }
                    }

                    // Ensure running *Setup also patches RAM (and running tests
                    // indirectly via *DebugAndroidTest will get this too).
                    tasks.matching { it.name == "${deviceName}Setup" }
                        .configureEach { finalizedBy(patchTask) }

                    // Also run the patch whenever tests run, even if *Setup is
                    // up-to-date from a previous invocation.
                    tasks.matching { it.name == "${deviceName}DebugAndroidTest" }
                        .configureEach { dependsOn(patchTask) }
                }

                (managedDeviceNames + listOf("connected")).forEach {
                    tasks.named("${it}DebugAndroidTest") {
                        outputs.upToDateWhen { false }
                    }
                }
            }
        }
    }
}


fun resolveAvdHome(): File {
    // ANDROID_AVD_HOME overrides the whole AVD directory.
    System.getenv("ANDROID_AVD_HOME")?.let { return File(it) }

    // ANDROID_USER_HOME is the parent of the .android directory (see Android docs).
    val androidUserHome = System.getenv("ANDROID_USER_HOME")
        ?: System.getProperty("user.home")
    return File(File(androidUserHome, ".android"), "avd")
}


fun findManagedDeviceAvdDirectory(avdHome: File, deviceName: String): File? {
    // Gradle Managed Devices create AVDs under <avdHome>/gradle-managed.
    // The AVD name is generated from device parameters and doesn't necessarily
    // match the managed device name (e.g. "maxVersion").
    val homes = listOf(avdHome, File(avdHome, "gradle-managed"))
        .filter { it.exists() }

    for (home in homes) {
        findAvdDirectory(home, deviceName)?.let { return it }
    }

    // As a fallback, pick the most recently modified AVD under gradle-managed.
    // This runs immediately after *Setup, so the newest one should correspond
    // to the device that was just created.
    for (home in homes) {
        findNewestAvdDirectory(home)?.let { return it }
    }

    return null
}


fun findNewestAvdDirectory(avdHome: File): File? {
    val candidates = avdHome.listFiles()?.filter {
        it.isDirectory && it.name.endsWith(".avd")
    } ?: return null
    return candidates.maxByOrNull { it.lastModified() }
}


fun findAvdDirectory(avdHome: File, deviceName: String): File? {
    // Prefer the AVD pointer file when it exists: <name>.ini contains the path
    // to the corresponding <name>.avd directory.
    resolveAvdDirFromIni(avdHome, "$deviceName.ini")?.let { return it }

    val exact = File(avdHome, "$deviceName.avd")
    if (exact.exists()) return exact

    // Gradle may add suffixes; try ini files first (more deterministic than
    // scanning directories), then fall back to directory names.
    val iniCandidates = avdHome.listFiles()?.filter {
        it.isFile && it.name.endsWith(".ini") && (
            it.name == "$deviceName.ini" ||
            it.name.startsWith("${deviceName}_") ||
            it.name.startsWith(deviceName)
        )
    }?.sortedBy { it.name } ?: emptyList()

    for (ini in iniCandidates) {
        resolveAvdDirFromIni(avdHome, ini.name)?.let { return it }
    }

    val dirCandidates = avdHome.listFiles()?.filter {
        it.isDirectory && it.name.endsWith(".avd") && (
            it.name == "$deviceName.avd" ||
            it.name.startsWith("${deviceName}_") ||
            it.name.startsWith(deviceName)
        )
    }?.sortedBy { it.name } ?: emptyList()

    return dirCandidates.firstOrNull()
}


fun resolveAvdDirFromIni(avdHome: File, iniFilename: String): File? {
    val iniFile = File(avdHome, iniFilename)
    if (!iniFile.exists()) return null

    for (line in iniFile.readLines()) {
        if (line.startsWith("path=")) {
            val pathValue = line.removePrefix("path=").trim()
            if (pathValue.isEmpty()) return null
            val path = File(pathValue)
            return if (path.isAbsolute) path else File(avdHome, pathValue)
        }
    }
    return null
}


fun setIniProperty(file: File, key: String, value: String): Boolean {
    val newLine = "$key=$value"
    val newline = System.lineSeparator()

    if (!file.exists()) {
        file.parentFile?.mkdirs()
        file.writeText(newLine + newline)
        return true
    }

    val lines = file.readLines().toMutableList()
    var found = false
    var changed = false

    for (i in lines.indices) {
        if (lines[i].startsWith("$key=")) {
            found = true
            if (lines[i] != newLine) {
                lines[i] = newLine
                changed = true
            }
        }
    }

    if (!found) {
        lines.add(newLine)
        changed = true
    }

    if (changed) {
        file.writeText(lines.joinToString(newline) + newline)
    }

    return changed
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("com.google.android.material:material:1.11.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test:rules:1.5.0")
}


// Create some custom tasks to copy Python and its standard library from
// elsewhere in the repository.
if (!isEmulatorSetupOnly) {
    androidComponents.onVariants { variant ->
        val pyPlusVer = "python$pythonVersion"
        generateTask(variant, variant.sources.assets!!) {
            into("python") {
                // Include files such as pyconfig.h are used by some of the tests.
                into("include/$pyPlusVer") {
                    for (prefix in prefixes) {
                        from("$prefix/include/$pyPlusVer")
                    }
                    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
                }

                into("lib/$pyPlusVer") {
                    // To aid debugging, the source directory takes priority when
                    // running inside a CPython source tree.
                    if (inSourceTree) {
                        from("$PYTHON_DIR/Lib")
                    }
                    for (prefix in prefixes) {
                        from("$prefix/lib/$pyPlusVer")
                    }

                    into("site-packages") {
                        from("$projectDir/src/main/python")

                        val sitePackages = findProperty("python.sitePackages") as String?
                        if (!sitePackages.isNullOrEmpty()) {
                            if (!file(sitePackages).exists()) {
                                throw GradleException("$sitePackages does not exist")
                            }
                            from(sitePackages)
                        }
                    }

                    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
                    exclude("**/__pycache__")
                }

                into("cwd") {
                    val cwd = findProperty("python.cwd") as String?
                    if (!cwd.isNullOrEmpty()) {
                        if (!file(cwd).exists()) {
                            throw GradleException("$cwd does not exist")
                        }
                        from(cwd)
                    }
                }

                // A filename ending with .gz will be automatically decompressed
                // while building the APK. Avoid this by adding a dash to the end,
                // and add an extra dash to any filenames that already end with one.
                // This will be undone in MainActivity.kt.
                rename(""".*(\.gz|-)""", "$0-")
            }
        }

        generateTask(variant, variant.sources.jniLibs!!) {
            for ((prefix, abi) in abis.entries) {
                into(abi) {
                    from("$prefix/lib")
                    include("libpython*.*.so")
                    include("lib*_python.so")
                }
            }
        }
    }
}


fun generateTask(
    variant: ApplicationVariant, directories: SourceDirectories,
    configure: GenerateTask.() -> Unit
) {
    val taskName = "generate" +
        listOf(variant.name, "Python", directories.name)
            .map { it.replaceFirstChar(Char::uppercase) }
            .joinToString("")

    directories.addGeneratedSourceDirectory(
        tasks.register<GenerateTask>(taskName) {
            into(outputDir)
            configure()
        },
        GenerateTask::outputDir)
}


// addGeneratedSourceDirectory requires the task to have a DirectoryProperty.
abstract class GenerateTask: Sync() {
    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty
}
