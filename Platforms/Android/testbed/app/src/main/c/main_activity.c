#include <android/log.h>
#include <errno.h>
#include <jni.h>
#include <pthread.h>
#include <Python.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


static void throw_runtime_exception(JNIEnv *env, const char *message) {
    (*env)->ThrowNew(
        env,
        (*env)->FindClass(env, "java/lang/RuntimeException"),
        message);
}

static void throw_errno(JNIEnv *env, const char *error_prefix) {
    char error_message[1024];
    snprintf(error_message, sizeof(error_message),
             "%s: %s", error_prefix, strerror(errno));
    throw_runtime_exception(env, error_message);
}


// --- Stdio redirection ------------------------------------------------------

// Most apps won't need this, because the Python-level sys.stdout and sys.stderr
// are redirected to the Android logcat by Python itself. However, in the
// testbed it's useful to redirect the native streams as well, to debug problems
// in the Python startup or redirection process.
//
// Based on
// https://github.com/beeware/briefcase-android-gradle-template/blob/v0.3.11/%7B%7B%20cookiecutter.safe_formal_name%20%7D%7D/app/src/main/cpp/native-lib.cpp

typedef struct {
    FILE *file;
    int fd;
    android_LogPriority priority;
    char *tag;
    int pipe[2];
} StreamInfo;

// The FILE member can't be initialized here because stdout and stderr are not
// compile-time constants. Instead, it's initialized immediately before the
// redirection.
static StreamInfo STREAMS[] = {
    {NULL, STDOUT_FILENO, ANDROID_LOG_INFO, "native.stdout", {-1, -1}},
    {NULL, STDERR_FILENO, ANDROID_LOG_WARN, "native.stderr", {-1, -1}},
    {NULL, -1, ANDROID_LOG_UNKNOWN, NULL, {-1, -1}},
};

// The maximum length of a log message in bytes, including the level marker and
// tag, is defined as LOGGER_ENTRY_MAX_PAYLOAD in
// platform/system/logging/liblog/include/log/log.h. As of API level 30, messages
// longer than this will be be truncated by logcat. This limit has already been
// reduced at least once in the history of Android (from 4076 to 4068 between API
// level 23 and 26), so leave some headroom.
static const int MAX_BYTES_PER_WRITE = 4000;

static void *redirection_thread(void *arg) {
    StreamInfo *si = (StreamInfo*)arg;
    ssize_t read_size;
    char buf[MAX_BYTES_PER_WRITE];
    while ((read_size = read(si->pipe[0], buf, sizeof buf - 1)) > 0) {
        buf[read_size] = '\0'; /* add null-terminator */
        __android_log_write(si->priority, si->tag, buf);
    }
    return 0;
}

static char *redirect_stream(StreamInfo *si) {
    /* make the FILE unbuffered, to ensure messages are never lost */
    if (setvbuf(si->file, 0, _IONBF, 0)) {
        return "setvbuf";
    }

    /* create the pipe and redirect the file descriptor */
    if (pipe(si->pipe)) {
        return "pipe";
    }
    if (dup2(si->pipe[1], si->fd) == -1) {
        return "dup2";
    }

    /* start the logging thread */
    pthread_t thr;
    if ((errno = pthread_create(&thr, 0, redirection_thread, si))) {
        return "pthread_create";
    }
    if ((errno = pthread_detach(thr))) {
        return "pthread_detach";
    }
    return 0;
}

JNIEXPORT void JNICALL Java_org_python_testbed_PythonTestRunner_redirectStdioToLogcat(
    JNIEnv *env, jobject obj
) {
    STREAMS[0].file = stdout;
    STREAMS[1].file = stderr;
    for (StreamInfo *si = STREAMS; si->file; si++) {
        char *error_prefix;
        if ((error_prefix = redirect_stream(si))) {
            throw_errno(env, error_prefix);
            return;
        }
    }
}


// --- Python initialization ---------------------------------------------------

static char *init_signals() {
    // Some tests use SIGUSR1, but that's blocked by default in an Android app in
    // order to make it available to `sigwait` in the Signal Catcher thread.
    // (https://cs.android.com/android/platform/superproject/+/android14-qpr3-release:art/runtime/signal_catcher.cc).
    // That thread's functionality is only useful for debugging the JVM, so disabling
    // it should not weaken the tests.
    //
    // There's no safe way of stopping the thread completely (#123982), but simply
    // unblocking SIGUSR1 is enough to fix most tests.
    //
    // However, in tests that generate multiple different signals in quick
    // succession, it's possible for SIGUSR1 to arrive while the main thread is busy
    // running the C-level handler for a different signal. In that case, the SIGUSR1
    // may be sent to the Signal Catcher thread instead, which will generate a log
    // message containing the text "reacting to signal".
    //
    // Such tests may need to be changed in one of the following ways:
    //   * Use a signal other than SIGUSR1 (e.g. test_stress_delivery_simultaneous in
    //     test_signal.py).
    //   * Send the signal to a specific thread rather than the whole process (e.g.
    //     test_signals in test_threadsignals.py.
    sigset_t set;
    if (sigemptyset(&set)) {
        return "sigemptyset";
    }
    if (sigaddset(&set, SIGUSR1)) {
        return "sigaddset";
    }
    if ((errno = pthread_sigmask(SIG_UNBLOCK, &set, NULL))) {
        return "pthread_sigmask";
    }
    return NULL;
}

static void throw_status(JNIEnv *env, PyStatus status) {
    throw_runtime_exception(env, status.err_msg ? status.err_msg : "");
}

JNIEXPORT int JNICALL Java_org_python_testbed_PythonTestRunner_runPython(
    JNIEnv *env, jobject obj, jstring home, jarray args
) {
    const char *home_utf8 = (*env)->GetStringUTFChars(env, home, NULL);
    char cwd[PATH_MAX];
    snprintf(cwd, sizeof(cwd), "%s/%s", home_utf8, "cwd");
    if (chdir(cwd)) {
        throw_errno(env, "chdir");
        return 1;
    }

    char *error_prefix;
    if ((error_prefix = init_signals())) {
        throw_errno(env, error_prefix);
        return 1;
    }

    PyConfig config;
    PyStatus status;
    PyConfig_InitPythonConfig(&config);

    jsize argc = (*env)->GetArrayLength(env, args);
    const char *argv[argc + 1];
    for (int i = 0; i < argc; i++) {
        jobject arg = (*env)->GetObjectArrayElement(env, args, i);
        argv[i] = (*env)->GetStringUTFChars(env, arg, NULL);
    }
    argv[argc] = NULL;

    // PyConfig_SetBytesArgv "must be called before other methods, since the
    // preinitialization configuration depends on command line arguments"
    if (PyStatus_Exception(status = PyConfig_SetBytesArgv(&config, argc, (char**)argv))) {
        throw_status(env, status);
        return 1;
    }

    status = PyConfig_SetBytesString(&config, &config.home, home_utf8);
    if (PyStatus_Exception(status)) {
        throw_status(env, status);
        return 1;
    }

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        throw_status(env, status);
        return 1;
    }

    return Py_RunMain();
}
