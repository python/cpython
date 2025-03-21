import com.android.build.api.variant.*
import kotlin.math.max

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val ANDROID_DIR = file("../..").canonicalPath
val PREFIX_DIR = "$ANDROID_DIR/prefix"

val ABIS = mapOf(
    "arm64-v8a" to "aarch64-linux-android",
    "x86_64" to "x86_64-linux-android",
).filter { file("$PREFIX_DIR/${it.value}").exists() }
if (ABIS.isEmpty()) {
    throw GradleException(
        "No Android ABIs found in $PREFIX_DIR: see Android/README.md " +
        "for building instructions."
    )
}

val PYTHON_VERSION = run {
    val includeDir = file("$PREFIX_DIR/${ABIS.values.iterator().next()}/include")
    for (filename in includeDir.list()!!) {
        val match = """python(\d+\.\d+)""".toRegex().matchEntire(filename)
        if (match != null) {
            return@run match.groupValues[1]
        }
    }
    throw GradleException("Failed to find Python version")
}


android {
    val androidEnvFile = file("../../android-env.sh").absoluteFile

    namespace = "org.python.testbed"
    compileSdk = 34

    defaultConfig {
        applicationId = "org.python.testbed"

        minSdk = androidEnvFile.useLines {
            for (line in it) {
                """api_level:=(\d+)""".toRegex().find(line)?.let {
                    return@useLines it.groupValues[1].toInt()
                }
            }
            throw GradleException("Failed to find API level in $androidEnvFile")
        }
        targetSdk = 34

        versionCode = 1
        versionName = "1.0"

        ndk.abiFilters.addAll(ABIS.keys)
        externalNativeBuild.cmake.arguments(
            "-DPYTHON_PREFIX_DIR=$PREFIX_DIR",
            "-DPYTHON_VERSION=$PYTHON_VERSION",
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

    // Set this property to something non-empty, otherwise it'll use the default
    // list, which ignores asset directories beginning with an underscore.
    aaptOptions.ignoreAssetsPattern = ".git"

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
    val pyPlusVer = "python$PYTHON_VERSION"
    generateTask(variant, variant.sources.assets!!) {
        into("python") {
            // Include files such as pyconfig.h are used by some of the tests.
            into("include/$pyPlusVer") {
                for (triplet in ABIS.values) {
                    from("$PREFIX_DIR/$triplet/include/$pyPlusVer")
                }
                duplicatesStrategy = DuplicatesStrategy.EXCLUDE
            }

            into("lib/$pyPlusVer") {
                // To aid debugging, the source directory takes priority when
                // running inside a CPython source tree.
                if (
                    file(ANDROID_DIR).name == "Android"
                    && file("$ANDROID_DIR/../pyconfig.h.in").exists()
                ) {
                    from("$ANDROID_DIR/../Lib")
                }

                // The prefix directory provides ABI-specific files such as
                // sysconfigdata.
                for (triplet in ABIS.values) {
                    from("$PREFIX_DIR/$triplet/lib/$pyPlusVer")
                }

                into("site-packages") {
                    from("$projectDir/src/main/python")
                }

                duplicatesStrategy = DuplicatesStrategy.EXCLUDE
                exclude("**/__pycache__")
            }
        }
    }

    generateTask(variant, variant.sources.jniLibs!!) {
        for ((abi, triplet) in ABIS.entries) {
            into(abi) {
                from("$PREFIX_DIR/$triplet/lib")
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
