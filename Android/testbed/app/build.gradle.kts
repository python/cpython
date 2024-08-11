import com.android.build.api.variant.*
import kotlin.math.max

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val PYTHON_DIR = file("../../..").canonicalPath
val PYTHON_CROSS_DIR = "$PYTHON_DIR/cross-build"

val ABIS = mapOf(
    "arm64-v8a" to "aarch64-linux-android",
    "x86_64" to "x86_64-linux-android",
).filter { file("$PYTHON_CROSS_DIR/${it.value}").exists() }
if (ABIS.isEmpty()) {
    throw GradleException(
        "No Android ABIs found in $PYTHON_CROSS_DIR: see Android/README.md " +
        "for building instructions."
    )
}

val PYTHON_VERSION = file("$PYTHON_DIR/Include/patchlevel.h").useLines {
    for (line in it) {
        val match = """#define PY_VERSION\s+"(\d+\.\d+)""".toRegex().find(line)
        if (match != null) {
            return@useLines match.groupValues[1]
        }
    }
    throw GradleException("Failed to find Python version")
}

android.ndkVersion = file("../../android-env.sh").useLines {
    for (line in it) {
        val match = """ndk_version=(\S+)""".toRegex().find(line)
        if (match != null) {
            return@useLines match.groupValues[1]
        }
    }
    throw GradleException("Failed to find NDK version")
}


android {
    namespace = "org.python.testbed"
    compileSdk = 34

    defaultConfig {
        applicationId = "org.python.testbed"
        minSdk = 21
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        ndk.abiFilters.addAll(ABIS.keys)
        externalNativeBuild.cmake.arguments(
            "-DPYTHON_CROSS_DIR=$PYTHON_CROSS_DIR",
            "-DPYTHON_VERSION=$PYTHON_VERSION")

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
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
                    systemImageSource = "aosp-atd"

                    // Managed devices have a minimum API level of 27.
                    apiLevel = max(27, defaultConfig.minSdk!!)
                }

                create("maxVersion") {
                    device = "Small Phone"
                    systemImageSource = "aosp-atd"
                    apiLevel = defaultConfig.targetSdk!!
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
    generateTask(variant, variant.sources.assets!!) {
        into("python") {
            for (triplet in ABIS.values) {
                for (subDir in listOf("include", "lib")) {
                    into(subDir) {
                        from("$PYTHON_CROSS_DIR/$triplet/prefix/$subDir")
                        include("python$PYTHON_VERSION/**")
                        duplicatesStrategy = DuplicatesStrategy.EXCLUDE
                    }
                }
            }
            into("lib/python$PYTHON_VERSION") {
                // Uncomment this to pick up edits from the source directory
                // without having to rerun `make install`.
                // from("$PYTHON_DIR/Lib")
                // duplicatesStrategy = DuplicatesStrategy.INCLUDE

                into("site-packages") {
                    from("$projectDir/src/main/python")
                }
            }
        }
        exclude("**/__pycache__")
    }

    generateTask(variant, variant.sources.jniLibs!!) {
        for ((abi, triplet) in ABIS.entries) {
            into(abi) {
                from("$PYTHON_CROSS_DIR/$triplet/prefix/lib")
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
