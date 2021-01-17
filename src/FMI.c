#ifdef _WIN32
#include <direct.h>
#include "Shlwapi.h"
#pragma comment(lib, "shlwapi.lib")
#define strdup _strdup
#define INTERNET_MAX_URL_LENGTH 2083 // from wininet.h
#else
#include <stdarg.h>
#include <dlfcn.h>
#endif

#include "FMI.h"

#define INITIAL_MESSAGE_BUFFER_SIZE 1024


/***************************************************
Utility Functions
****************************************************/

FMI2Instance *FMICreateInstance(const char *instanceName, const char *libraryPath, FMI2LogMessageTYPE *logMessage, FMI2LogFunctionCallTYPE *logFunctionCall) {

	FMI2Instance* instance = (FMI2Instance*)calloc(1, sizeof(FMI2Instance));

	instance->logMessage = logMessage;
	instance->logFunctionCall = logFunctionCall;

	instance->bufsize1 = INITIAL_MESSAGE_BUFFER_SIZE;
	instance->bufsize2 = INITIAL_MESSAGE_BUFFER_SIZE;

	instance->buf1 = (char *)calloc(instance->bufsize1, sizeof(char));
	instance->buf2 = (char *)calloc(instance->bufsize1, sizeof(char));

	instance->name = strdup(instanceName);

	instance->status = FMIOK;

# ifdef _WIN32
	WCHAR dllDirectory[MAX_PATH];

	// convert path to unicode
	mbstowcs(dllDirectory, libraryPath, MAX_PATH);

	// add the binaries directory temporarily to the DLL path to allow discovery of dependencies
	DLL_DIRECTORY_COOKIE dllDirectoryCookie = AddDllDirectory(dllDirectory);

	// TODO: log getLastSystemError()

	instance->libraryHandle = LoadLibraryEx(libraryPath, NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

	// remove the binaries directory from the DLL path
	if (dllDirectoryCookie) {
		RemoveDllDirectory(dllDirectoryCookie);
	}

	// TODO: log error

# else
	instance->libraryHandle = dlopen(libraryPath, RTLD_LAZY);
# endif

	return instance;
}

void FMIFreeInstance(FMI2Instance *instance) {

	// unload the shared library
	if (instance->libraryHandle) {
# ifdef _WIN32
		FreeLibrary(instance->libraryHandle);
# else
		dlclose(instance->libraryHandle);
# endif
		instance->libraryHandle = NULL;
	}
	
	free(instance);
}
