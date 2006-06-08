/*
 * This wrapper program executes a python executable hidden inside an
 * application bundle inside the Python framework. This is needed to run
 * GUI code: some GUI API's don't work unless the program is inside an
 * application bundle.
 */
#include <unistd.h>
#include <err.h>

static char Python[] = PYTHONWEXECUTABLE;

int main(int argc, char **argv) {
	argv[0] = Python;
	execv(Python, argv);
	err(1, "execv: %s", Python);
	/* NOTREACHED */
}
