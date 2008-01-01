/* This program looks for processes which have build\PCbuild\python.exe
   in their path and terminates them. */
#include <windows.h>
#include <psapi.h>
#include <stdio.h>

int main()
{
	DWORD pids[1024], cbNeeded;
	int i, num_processes;
	if (!EnumProcesses(pids, sizeof(pids), &cbNeeded)) {
		printf("EnumProcesses failed\n");
		return 1;
	}
	num_processes = cbNeeded/sizeof(pids[0]);
	for (i = 0; i < num_processes; i++) {
		HANDLE hProcess;
		char path[MAX_PATH];
		HMODULE mods[1024];
		int k, num_mods;
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION 
					| PROCESS_VM_READ 
					|  PROCESS_TERMINATE ,
					FALSE, pids[i]);
		if (!hProcess)
			/* process not accessible */
			continue;
		if (!EnumProcessModules(hProcess, mods, sizeof(mods), &cbNeeded)) {
			/* For unknown reasons, this sometimes returns ERROR_PARTIAL_COPY;
			   this apparently means we are not supposed to read the process. */
			if (GetLastError() == ERROR_PARTIAL_COPY) {
				CloseHandle(hProcess);
				continue;
			}
			printf("EnumProcessModules failed: %d\n", GetLastError());
			return 1;
		}
		if (!GetModuleFileNameEx(hProcess, NULL, path, sizeof(path))) {
			printf("GetProcessImageFileName failed\n");
			return 1;
		}

		_strlwr(path);
		/* printf("%s\n", path); */

		/* Check if we are running a buildbot version of Python.

		   On Windows, this will always be a debug build from the
		   PCbuild directory.  build\\PCbuild\\python_d.exe
		   
		   On Cygwin, the pathname is similar to other Unixes.
		   Use \\build\\python.exe to ensure we don't match
		   PCbuild\\python.exe which could be a normal instance
		   of Python running on vanilla Windows.
		*/
		if ((strstr(path, "pcbuild\\python_d.exe") != NULL) ||
		    (strstr(path, "\\build\\python.exe") != NULL)) {
			printf("Terminating %s (pid %d)\n", path, pids[i]);
			if (!TerminateProcess(hProcess, 1)) {
				printf("Termination failed: %d\n", GetLastError());
				return 1;
			}
			return 0;
		}

		CloseHandle(hProcess);
	}
}
