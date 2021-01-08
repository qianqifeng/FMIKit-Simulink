#ifdef _WIN32
#include <direct.h>
#include "Shlwapi.h"
#pragma comment(lib, "shlwapi.lib")
#else
#include <stdarg.h>
#include <dlfcn.h>
#endif

#include "FMI2.h"

// callback functions
static void cb_logMessage(fmi2ComponentEnvironment componentEnvironment, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...) {
	printf("%s\n", message);
}

static void* cb_allocateMemory(size_t nobj, size_t size) {
	return calloc(nobj, size);
}

static void cb_freeMemory(void* obj) {
	free(obj);
}


FMI2Instance* FMI2Instantiate(const char *unzipdir, fmi2String instanceName, fmi2Type fmuType, fmi2String guid, fmi2Boolean visible, fmi2Boolean loggingOn) {

	FMI2Instance* instance = (FMI2Instance*)calloc(1, sizeof(FMI2Instance));

	if (!instance) {
		// TODO: log error
		return NULL;
	}

	instance->name = strdup(instanceName);




# ifdef _WIN32
	char libraryPath[MAX_PATH];

	strncpy(libraryPath, unzipdir, MAX_PATH);

	PathAppend(libraryPath, "binaries");
	PathAppend(libraryPath, "win64");
	PathAppend(libraryPath, "BouncingBall.dll");

	instance->libraryHandle = LoadLibraryEx(libraryPath, NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
# else
	instance->libraryHandle = dlopen(libraryPath, RTLD_LAZY);
# endif

	if (!instance->libraryHandle) goto fail;

	// TODO: load symbols
# ifdef _WIN32
	instance->fmi2Instantiate = (fmi2InstantiateTYPE*)GetProcAddress(instance->libraryHandle, "fmi2Instantiate");
	instance->fmi2SetupExperiment = (fmi2InstantiateTYPE*)GetProcAddress(instance->libraryHandle, "fmi2SetupExperiment");
	instance->fmi2EnterInitializationMode = (fmi2InstantiateTYPE*)GetProcAddress(instance->libraryHandle, "fmi2EnterInitializationMode");
	instance->fmi2ExitInitializationMode = (fmi2InstantiateTYPE*)GetProcAddress(instance->libraryHandle, "fmi2ExitInitializationMode");
	instance->fmi2DoStep = (fmi2InstantiateTYPE*)GetProcAddress(instance->libraryHandle, "fmi2DoStep");
	instance->fmi2GetReal = (fmi2InstantiateTYPE*)GetProcAddress(instance->libraryHandle, "fmi2GetReal");
# else
	auto *fp = dlsym(m_libraryHandle, functionName);
# endif

	const fmi2CallbackFunctions functions = {
		cb_logMessage, cb_allocateMemory, cb_freeMemory, NULL, instance
	};

	_try {
		instance->component = instance->fmi2Instantiate(instanceName, fmuType, guid, "fmuResourceLocation", &functions, visible, loggingOn);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		// TODO: log exception
		goto fail;
	}

	// TODO: log call

	if (!instance->component) goto fail;

	return instance;

fail:
	if (instance->name) free((void *)instance->name);
	if (instance->libraryHandle) FreeLibrary(instance->libraryHandle);
	free(instance);
	return NULL;
}

// #define ASSERT_NO_ERROR(F, M) __try { assertNoError(F, M); } __except (EXCEPTION_EXECUTE_HANDLER) { error("%s. The FMU crashed (exception code: %s).", M, exceptionCodeToString(GetExceptionCode())); }

/* Enter and exit initialization mode, terminate and reset */
fmi2Status FMI2SetupExperiment(FMI2Instance *instance,
	fmi2Boolean toleranceDefined,
	fmi2Real tolerance,
	fmi2Real startTime,
	fmi2Boolean stopTimeDefined,
	fmi2Real stopTime) {

	return instance->fmi2SetupExperiment(instance->component, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
}

fmi2Status FMI2EnterInitializationMode(FMI2Instance *instance) {

	if (!instance) return fmi2Error;

	fmi2Status status;

	__try {
		status = instance->fmi2EnterInitializationMode(instance->component);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		// TODO: log exception
		return fmi2Fatal;
	}

	if (instance->logFMICalls) {
		// TODO: log call
	}

	return status;
}

fmi2Status FMI2ExitInitializationMode(FMI2Instance *instance) {
	return instance->fmi2ExitInitializationMode(instance->component);
}

fmi2Status FMI2Terminate(FMI2Instance *instance) {
	return instance->fmi2Terminate(instance->component);
}

/* Getting and setting variable values */
fmi2Status FMI2GetReal(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
	return instance->fmi2GetReal(instance->component, vr, nvr, value);
}

/***************************************************
Types for Functions for FMI2 for Co-Simulation
****************************************************/

/* Simulating the slave */
fmi2Status FMI2DoStep(FMI2Instance *instance,
	fmi2Real      currentCommunicationPoint,
	fmi2Real      communicationStepSize,
	fmi2Boolean   noSetFMUStatePriorToCurrentPoint) {
	return instance->fmi2DoStep(instance->component, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);
}
