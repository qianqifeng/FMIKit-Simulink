#ifdef _WIN32
#include <direct.h>
#include "Shlwapi.h"
#pragma comment(lib, "shlwapi.lib")
#define strdup _strdup
#else
#include <stdarg.h>
#include <dlfcn.h>
#endif

#include <stdio.h>
#include <string.h>

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

#define LOAD_SYMBOL(f) \
	instance->fmi2 ## f = (fmi2 ## f ## TYPE*)GetProcAddress(instance->libraryHandle, "fmi2" #f); \
	if (!instance->fmi2 ## f) goto fail;

#define LOG(...) \
	if (instance->logFunctionCall) { \
		instance->logFunctionCall(status, instance->name, __VA_ARGS__); \
	}

#define CALL(f) \
	fmi2Status status = instance-> ## f (instance->component); \
	if (instance->logFunctionCall) { \
		instance->logFunctionCall(status, instance->name, #f "(component=0x%p)", instance->component); \
	} \
	return status;

#define CALL_ARGS(f, m, ...) \
	fmi2Status status = instance-> ## f (instance->component, __VA_ARGS__); \
	if (instance->logFunctionCall) { \
		instance->logFunctionCall(status, instance->name, #f "(component=0x%p" m ")", instance->component, __VA_ARGS__); \
	} \
	return status;

/***************************************************
Types for Functions for FMI2 for Co-Simulation
****************************************************/

/* Inquire version numbers of header files and setting logging status */
//const char* fmi2GetTypesPlatform(void);

//const char* fmi2GetVersion(void);

//fmi2Status  fmi2SetDebugLogging(fmi2Component c,
//	fmi2Boolean loggingOn,
//	size_t nCategories,
//	const fmi2String categories[]);

/* Creation and destruction of FMU instances and setting debug status */
FMI2Instance* FMI2Instantiate(const char *unzipdir, const char *modelIdentifier, fmi2String instanceName, fmi2Type fmuType, fmi2String guid, fmi2Boolean visible, fmi2Boolean loggingOn) {

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
	PathAppend(libraryPath, modelIdentifier);
	strncat(libraryPath, ".dll", MAX_PATH);

	instance->libraryHandle = LoadLibraryEx(libraryPath, NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
# else
	instance->libraryHandle = dlopen(libraryPath, RTLD_LAZY);
# endif

# ifdef _WIN32
# else
	auto *fp = dlsym(m_libraryHandle, functionName);
# endif

	if (!instance->libraryHandle) goto fail;

	// load symbols

	/***************************************************
	Common Functions for FMI 2.0
	****************************************************/

	/* required functions */
	LOAD_SYMBOL(GetTypesPlatform)
	LOAD_SYMBOL(GetVersion)
	LOAD_SYMBOL(SetDebugLogging)
	LOAD_SYMBOL(Instantiate)
	LOAD_SYMBOL(FreeInstance)
	LOAD_SYMBOL(SetupExperiment)
	LOAD_SYMBOL(EnterInitializationMode)
	LOAD_SYMBOL(ExitInitializationMode)
	LOAD_SYMBOL(Terminate)
	LOAD_SYMBOL(Reset)
	LOAD_SYMBOL(GetReal)
	LOAD_SYMBOL(GetInteger)
	LOAD_SYMBOL(GetBoolean)
	LOAD_SYMBOL(GetString)
	LOAD_SYMBOL(SetReal)
	LOAD_SYMBOL(SetInteger)
	LOAD_SYMBOL(SetBoolean)
	LOAD_SYMBOL(SetString)

	/* optional functions */
	LOAD_SYMBOL(GetFMUstate)
	LOAD_SYMBOL(SetFMUstate)
	LOAD_SYMBOL(FreeFMUstate)
	LOAD_SYMBOL(SerializedFMUstateSize)
	LOAD_SYMBOL(SerializeFMUstate)
	LOAD_SYMBOL(DeSerializeFMUstate)
	LOAD_SYMBOL(GetDirectionalDerivative)

	/***************************************************
	Functions for FMI 2.0 for Model Exchange
	****************************************************/

	LOAD_SYMBOL(EnterEventMode)
	LOAD_SYMBOL(NewDiscreteStates)
	LOAD_SYMBOL(EnterContinuousTimeMode)
	LOAD_SYMBOL(CompletedIntegratorStep)
	LOAD_SYMBOL(SetTime)
	LOAD_SYMBOL(SetContinuousStates)
	LOAD_SYMBOL(GetDerivatives)
	LOAD_SYMBOL(GetEventIndicators)
	LOAD_SYMBOL(GetContinuousStates)
	LOAD_SYMBOL(GetNominalsOfContinuousStates)

	/***************************************************
	Functions for FMI 2.0 for Co-Simulation
	****************************************************/

	LOAD_SYMBOL(SetRealInputDerivatives)
	LOAD_SYMBOL(GetRealOutputDerivatives)
	LOAD_SYMBOL(DoStep)
	LOAD_SYMBOL(CancelStep)
	LOAD_SYMBOL(GetStatus)
	LOAD_SYMBOL(GetRealStatus)
	LOAD_SYMBOL(GetIntegerStatus)
	LOAD_SYMBOL(GetBooleanStatus)
	LOAD_SYMBOL(GetStringStatus)

	const fmi2CallbackFunctions functions = {
		cb_logMessage, cb_allocateMemory, cb_freeMemory, NULL, instance
	};

	_try{
		instance->component = instance->fmi2Instantiate(instanceName, fmuType, guid, "", &functions, visible, loggingOn);
	}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		// TODO: log exception
		// #define ASSERT_NO_ERROR(F, M) __try { assertNoError(F, M); } __except (EXCEPTION_EXECUTE_HANDLER) { error("%s. The FMU crashed (exception code: %s).", M, exceptionCodeToString(GetExceptionCode())); }
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

void FMI2FreeInstance(FMI2Instance *instance) {
	instance->fmi2FreeInstance(instance->component);
}

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
	CALL(fmi2EnterInitializationMode)
}

fmi2Status FMI2ExitInitializationMode(FMI2Instance *instance) {
	CALL(fmi2ExitInitializationMode)
}

fmi2Status FMI2Terminate(FMI2Instance *instance) {
	CALL(fmi2Terminate)
}

fmi2Status FMI2Reset(FMI2Instance *instance) {
	CALL(fmi2Reset)
}

/* Getting and setting variable values */
fmi2Status FMI2GetReal(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
	return instance->fmi2GetReal(instance->component, vr, nvr, value);
}

fmi2Status FMI2GetInteger(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
	return instance->fmi2GetInteger(instance->component, vr, nvr, value);
}

fmi2Status FMI2GetBoolean(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
	return instance->fmi2GetBoolean(instance->component, vr, nvr, value);
}

fmi2Status FMI2GetString(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2String  value[]) {
	return instance->fmi2GetString(instance->component, vr, nvr, value);
}

fmi2Status FMI2SetReal(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Real    value[]) {
	return instance->fmi2SetReal(instance->component, vr, nvr, value);
}

fmi2Status FMI2SetInteger(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
	return instance->fmi2SetInteger(instance->component, vr, nvr, value);
}

fmi2Status FMI2SetBoolean(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
	return instance->fmi2SetBoolean(instance->component, vr, nvr, value);
}

fmi2Status FMI2SetString(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2String  value[]) {
	return instance->fmi2SetString(instance->component, vr, nvr, value);
}

/* Getting and setting the internal FMU state */
fmi2Status FMI2GetFMUstate(FMI2Instance *instance, fmi2FMUstate* FMUstate) {
	//return instance->fmi2GetFMUstate(instance->component, FMUstate);
	CALL_ARGS(fmi2GetFMUstate, ", FMUstate=%p", FMUstate)
}

fmi2Status FMI2SetFMUstate(FMI2Instance *instance, fmi2FMUstate  FMUstate) {
	CALL_ARGS(fmi2SetFMUstate, ", FMUstate=%p", FMUstate)
}

fmi2Status FMI2FreeFMUstate(FMI2Instance *instance, fmi2FMUstate* FMUstate) {
	CALL_ARGS(fmi2FreeFMUstate, ", FMUstate=%p", FMUstate)
}

fmi2Status FMI2SerializedFMUstateSize(FMI2Instance *instance, fmi2FMUstate  FMUstate, size_t* size) {
	CALL_ARGS(fmi2SerializedFMUstateSize, ", FMUstate=%p, size=%zu", FMUstate, size);
}

fmi2Status FMI2SerializeFMUstate(FMI2Instance *instance, fmi2FMUstate  FMUstate, fmi2Byte serializedState[], size_t size) {
	CALL_ARGS(fmi2SerializeFMUstate, ", FMUstate=%p, serializedState=%p, size=%zu", FMUstate, serializedState, size);
}

fmi2Status FMI2DeSerializeFMUstate(FMI2Instance *instance, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
	CALL_ARGS(fmi2DeSerializeFMUstate, ", serializedState=%p, size=%zu, FMUstate=%p", serializedState, size, FMUstate);
}

/* Getting partial derivatives */
fmi2Status FMI2GetDirectionalDerivative(FMI2Instance *instance,
	const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
	const fmi2ValueReference vKnown_ref[], size_t nKnown,
	const fmi2Real dvKnown[],
	fmi2Real dvUnknown[]) {
	return instance->fmi2GetDirectionalDerivative(instance->component, vUnknown_ref, nUnknown, vKnown_ref, nKnown, dvKnown, dvUnknown);
}

/***************************************************
Types for Functions for FMI2 for Model Exchange
****************************************************/

/* Enter and exit the different modes */
fmi2Status FMI2EnterEventMode(FMI2Instance *instance) {
	CALL(fmi2EnterEventMode)
}

fmi2Status FMI2NewDiscreteStates(FMI2Instance *instance, fmi2EventInfo* fmi2eventInfo) {
	return instance->fmi2NewDiscreteStates(instance->component, fmi2eventInfo);
}

fmi2Status FMI2EnterContinuousTimeMode(FMI2Instance *instance) {
	CALL(fmi2EnterContinuousTimeMode)
}

fmi2Status FMI2CompletedIntegratorStep(FMI2Instance *instance,
	fmi2Boolean   noSetFMUStatePriorToCurrentPoint,
	fmi2Boolean*  enterEventMode,
	fmi2Boolean*  terminateSimulation) {
	return instance->fmi2CompletedIntegratorStep(instance->component, noSetFMUStatePriorToCurrentPoint, enterEventMode, terminateSimulation);
}

/* Providing independent variables and re-initialization of caching */
fmi2Status FMI2SetTime(FMI2Instance *instance, fmi2Real time) {
	return instance->fmi2SetTime(instance->component, time);
}

fmi2Status FMI2SetContinuousStates(FMI2Instance *instance, const fmi2Real x[], size_t nx) {
	return instance->fmi2SetContinuousStates(instance->component, x, nx);
}

/* Evaluation of the model equations */
fmi2Status FMI2GetDerivatives(FMI2Instance *instance, fmi2Real derivatives[], size_t nx) {
	return instance->fmi2GetDerivatives(instance->component, derivatives, nx);
}

fmi2Status FMI2GetEventIndicators(FMI2Instance *instance, fmi2Real eventIndicators[], size_t ni) {
	return instance->fmi2GetEventIndicators(instance->component, eventIndicators, ni);
}

fmi2Status FMI2GetContinuousStates(FMI2Instance *instance, fmi2Real x[], size_t nx) {
	return instance->fmi2GetContinuousStates(instance->component, x, nx);
}

fmi2Status FMI2GetNominalsOfContinuousStates(FMI2Instance *instance, fmi2Real x_nominal[], size_t nx) {
	return instance->fmi2GetNominalsOfContinuousStates(instance->component, x_nominal, nx);
}

/***************************************************
Types for Functions for FMI2 for Co-Simulation
****************************************************/

/* Simulating the slave */
fmi2Status FMI2SetRealInputDerivatives(FMI2Instance *instance,
	const fmi2ValueReference vr[], size_t nvr,
	const fmi2Integer order[],
	const fmi2Real value[]) {
	return instance->fmi2SetRealInputDerivatives(instance->component, vr, nvr, order, value);
}

fmi2Status FMI2GetRealOutputDerivatives(FMI2Instance *instance,
	const fmi2ValueReference vr[], size_t nvr,
	const fmi2Integer order[],
	fmi2Real value[]) {
	return instance->fmi2GetRealOutputDerivatives(instance->component, vr, nvr, order, value);
}

fmi2Status FMI2DoStep(FMI2Instance *instance,
	fmi2Real      currentCommunicationPoint,
	fmi2Real      communicationStepSize,
	fmi2Boolean   noSetFMUStatePriorToCurrentPoint) {
	CALL_ARGS(fmi2DoStep, ", currentCommunicationPoint=%g, communicationStepSize=%g, noSetFMUStatePriorToCurrentPoint=%d",
		currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint)
}

fmi2Status FMI2CancelStep(FMI2Instance *instance) {
	CALL(fmi2CancelStep);
}

/* Inquire slave status */
fmi2Status FMI2GetStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2Status*  value) {
	return instance->fmi2GetStatus(instance->component, s, value);
}

fmi2Status FMI2GetRealStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2Real*    value) {
	return instance->fmi2GetRealStatus(instance->component, s, value);
}

fmi2Status FMI2GetIntegerStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2Integer* value) {
	return instance->fmi2GetIntegerStatus(instance->component, s, value);
}

fmi2Status FMI2GetBooleanStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2Boolean* value) {
	return instance->fmi2GetBooleanStatus(instance->component, s, value);
}

fmi2Status FMI2GetStringStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2String*  value) {
	return instance->fmi2GetStringStatus(instance->component, s, value);
}
