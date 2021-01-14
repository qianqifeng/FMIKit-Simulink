#include "FMI.h"


#define INITIAL_MESSAGE_BUFFER_SIZE 1024


FMIInstance *FMICreateInstance(const char *instanceName, const char *libraryPath, FMILogMessageTYPE *logMessage, FMILogFunctionCallTYPE *logFunctionCall) {

	FMIInstance* instance = (FMIInstance*)calloc(1, sizeof(FMIInstance));

	instance->logMessage = logMessage;
	instance->logFunctionCall = logFunctionCall;

	instance->bufsize1 = INITIAL_MESSAGE_BUFFER_SIZE;
	instance->bufsize2 = INITIAL_MESSAGE_BUFFER_SIZE;

	instance->buf1 = (char *)calloc(instance->bufsize1, sizeof(char));
	instance->buf2 = (char *)calloc(instance->bufsize1, sizeof(char));

	instance->name = strdup(instanceName);

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

void FMIFreeInstance(FMIInstance *instance) {
	// TODO
}
