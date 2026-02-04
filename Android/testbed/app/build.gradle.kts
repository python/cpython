import com.android.build.api.variant.*
import kotlin.math.max

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
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
