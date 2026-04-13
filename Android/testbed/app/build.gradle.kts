import com.android.build.api.variant.*
import kotlin.math.max

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val ANDROID_DIR = file("../..")
val PYTHON_DIR = ANDROID_DIR.parentFile!!
val PYTHON_CROSS_DIR = file(System.getenv("CROSS_BUILD_DIR") ?: "$PYTHON_DIR/cross-build")
val inSourceTree = (
    ANDROID_DIR.name == "Android" && file("$PYTHON_DIR/pyconfig.h.in").exists()
)

val KNOWN_ABIS = mapOf(
    "aarch64-linux-android" to "arm64-v8a",
    "arm-linux-androideabi" to "armeabi-v7a",
    "i686-linux-android" to "x86",
    "x86_64-linux-android" to "x86_64",
)

val osArch = System.getProperty("os.arch")
val NATIVE_ABI = mapOf(
    "aarch64" to "arm64-v8a",
    "amd64" to "x86_64",
    "arm64" to "arm64-v8a",
    "x86_64" to "x86_64",
)[osArch] ?: throw GradleException("Unknown os.arch '$osArch'")

// Discover prefixes.
val prefixes = ArrayList<File>()
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
lateinit var pythonVersion: String
var abis = HashMap<File, String>()
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
    externalNativeBuild.cmake {
        path("src/main/c/CMakeLists.txt")
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
                // systemImageSource should use what its documentation calls an
                // "explicit source", i.e. the sdkmanager package name format, because
                // that will be required in CreateEmulatorTask below.
                create("minVersion") {
                    device = "Small Phone"

                    // Managed devices have a minimum API level of 27.
                    apiLevel = max(27, defaultConfig.minSdk!!)

                    // ATD devices are smaller and faster, but have a minimum
                    // API level of 30.
                    systemImageSource = if (apiLevel >= 30) "aosp_atd" else "default"
                }

                create("maxVersion") {
                    device = "Small Phone"
                    apiLevel = defaultConfig.targetSdk!!
                    systemImageSource = "aosp_atd"
                }
            }

            // If the previous test run succeeded and nothing has changed,
            // Gradle thinks there's no need to run it again. Override that.
            afterEvaluate {
                (localDevices.names + listOf("connected")).forEach {
                    tasks.named("${it}DebugAndroidTest") {
                        outputs.upToDateWhen { false }
                    }
                }
            }
        }
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("com.google.android.material:material:1.11.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test:rules:1.5.0")
}


afterEvaluate {
    // Every new emulator has a maximum of 2 GB RAM, regardless of its hardware profile
    // (https://cs.android.com/android-studio/platform/tools/base/+/refs/tags/studio-2025.3.2:sdklib/src/main/java/com/android/sdklib/internal/avd/EmulatedProperties.java;l=68).
    // This is barely enough to test Python, and not enough to test Pandas
    // (https://github.com/python/cpython/pull/137186#issuecomment-3136301023,
    // https://github.com/pandas-dev/pandas/pull/63405#issuecomment-3667846159).
    // So we'll increase it by editing the emulator configuration files.
    //
    // If the emulator doesn't exist yet, we want to edit it after it's created, but
    // before it starts for the first time. Otherwise it'll need to be cold-booted
    // again, which would slow down the first run, which is likely the only run in CI
    // environments. But the Setup task both creates and starts the emulator if it
    // doesn't already exist. So we create it ourselves before the Setup task runs.
    for (device in android.testOptions.managedDevices.localDevices) {
        val createTask = tasks.register<CreateEmulatorTask>("${device.name}Create") {
            this.device = device.device
            apiLevel = device.apiLevel
            systemImageSource = device.systemImageSource
            abi = NATIVE_ABI
        }
        tasks.named("${device.name}Setup") {
            dependsOn(createTask)
        }
    }
}

abstract class CreateEmulatorTask : DefaultTask() {
    @get:Input abstract val device: Property<String>
    @get:Input abstract val apiLevel: Property<Int>
    @get:Input abstract val systemImageSource: Property<String>
    @get:Input abstract val abi: Property<String>
    @get:Inject abstract val execOps: ExecOperations

    private val avdName by lazy {
        listOf(
            "dev${apiLevel.get()}",
            systemImageSource.get(),
            abi.get(),
            device.get().replace(' ', '_'),
        ).joinToString("_")
    }

    private val avdDir by lazy {
        // XDG_CONFIG_HOME is respected by both avdmanager and Gradle.
        val userHome = System.getenv("ANDROID_USER_HOME") ?: (
            (System.getenv("XDG_CONFIG_HOME") ?: System.getProperty("user.home")!!)
            + "/.android"
        )
        File("$userHome/avd/gradle-managed", "$avdName.avd")
    }

    @TaskAction
    fun run() {
        if (!avdDir.exists()) {
            createAvd()
        }
        updateAvd()
    }

    fun createAvd() {
        val systemImage = listOf(
            "system-images",
            "android-${apiLevel.get()}",
            systemImageSource.get(),
            abi.get(),
        ).joinToString(";")

        runCmdlineTool("sdkmanager", systemImage)
        runCmdlineTool(
            "avdmanager", "create", "avd",
            "--name", avdName,
            "--path", avdDir,
            "--device", device.get().lowercase().replace(" ", "_"),
            "--package", systemImage,
        )

        val iniName = "$avdName.ini"
        if (!File(avdDir.parentFile.parentFile, iniName).renameTo(
            File(avdDir.parentFile, iniName)
        )) {
            throw GradleException("Failed to rename $iniName")
        }
    }

    fun updateAvd() {
        for (filename in listOf(
            "config.ini",  // Created by avdmanager; always exists
            "hardware-qemu.ini",  // Created on first run; might not exist
        )) {
            val iniFile = File(avdDir, filename)
            if (!iniFile.exists()) {
                if (filename == "config.ini") {
                    throw GradleException("$iniFile does not exist")
                }
                continue
            }

            val iniText = iniFile.readText()
            val pattern = Regex(
                """^\s*hw.ramSize\s*=\s*(.+?)\s*$""", RegexOption.MULTILINE
            )
            val matches = pattern.findAll(iniText).toList()
            if (matches.size != 1) {
                throw GradleException(
                    "Found ${matches.size} instances of $pattern in $iniFile; expected 1"
                )
            }

            val expectedRam = "4096"
            if (matches[0].groupValues[1] != expectedRam) {
                iniFile.writeText(
                    iniText.replace(pattern, "hw.ramSize = $expectedRam")
                )
            }
        }
    }

    fun runCmdlineTool(tool: String, vararg args: Any) {
        val androidHome = System.getenv("ANDROID_HOME")!!
        val exeSuffix =
            if (System.getProperty("os.name").lowercase().startsWith("win")) ".exe"
            else ""
        val command =
            listOf("$androidHome/cmdline-tools/latest/bin/$tool$exeSuffix", *args)
        println(command.joinToString(" "))
        execOps.exec {
            commandLine(command)
        }
    }
}


// Create some custom tasks to copy Python and its standard library from
// elsewhere in the repository.
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
