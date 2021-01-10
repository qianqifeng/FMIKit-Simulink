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

#define CALL(f) \
	fmi2Status status = instance-> ## f (instance->component); \
	if (instance->logFunctionCall) { \
		instance->logFunctionCall(status, instance->name, #f "(component=0x%p)", instance->component); \
	} \
	return status;

#define CALL_ARGS(f, m, ...) \
	fmi2Status status = instance-> ## f (instance->component, __VA_ARGS__); \
	if (instance->logFunctionCall) { \
		instance->logFunctionCall(status, instance->name, #f "(component=0x%p, " m ")", instance->component, __VA_ARGS__); \
	} \
	return status;

static void logArray(FMI2Instance *instance, const char *format, const fmi2ValueReference vr[], size_t nvr, void *value, FMI2VariableType variableType, fmi2Status status) {

	size_t pos = 0;

	do {
		pos += snprintf(&instance->buf1[pos], instance->bufsize1 - pos, "{");

		for (size_t i = 0; i < nvr; i++) {

			pos += snprintf(&instance->buf1[pos], instance->bufsize1 - pos, i < nvr - 1 ? "%u, " : "%u", vr[i]);

			if (pos > instance->bufsize1 - 2) {
				pos = 0;
				instance->bufsize1 *= 2;
				instance->buf1 = (char*)realloc(instance->buf1, instance->bufsize1);
				break;
			}
		}
	} while (pos == 0);

	pos += snprintf(&instance->buf1[pos], instance->bufsize1 - pos, "}");

	pos = 0;

	do {
		pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, "{");

		for (size_t i = 0; i < nvr; i++) {

			switch (variableType) {
			case FMI2RealType:
				pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, i < nvr - 1 ? "%g, " : "%g", ((fmi2Real *)value)[i]);
				break;
			case FMI2IntegerType:
			case FMI2BooleanType:
				pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, i < nvr - 1 ? "%d, " : "%d", ((int *)value)[i]);
				break;
			case FMI2StringType:
				pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, i < nvr - 1 ? "\"%s\", " : "\"%s\"", ((fmi2String *)value)[i]);
				break;
			}

			if (pos > instance->bufsize2 - 2) {
				pos = 0;
				instance->bufsize2 *= 2;
				instance->buf2 = (char*)realloc(instance->buf2, instance->bufsize2);
				break;
			}
		}
	} while (pos == 0);

	pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, "}");

	instance->logFunctionCall(status, instance->name, format, instance->component, instance->buf1, nvr, instance->buf2);
}

static const char* vrToString(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr) {

	size_t pos = 0;

	do {
		pos += snprintf(&instance->buf1[pos], instance->bufsize1 - pos, "{");

		for (size_t i = 0; i < nvr; i++) {

			pos += snprintf(&instance->buf1[pos], instance->bufsize1 - pos, i < nvr - 1 ? "%u, " : "%u", vr[i]);

			if (pos > instance->bufsize1 - 2) {
				pos = 0;
				instance->bufsize1 *= 2;
				instance->buf1 = (char*)realloc(instance->buf1, instance->bufsize1);
				break;
			}
		}
	} while (pos == 0);

	pos += snprintf(&instance->buf1[pos], instance->bufsize1 - pos, "}");

	return instance->buf1;
}

static const char* valueToString(FMI2Instance *instance, size_t nvr, const void *value, FMI2VariableType variableType) {

	size_t pos = 0;

	do {
		pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, "{");

		for (size_t i = 0; i < nvr; i++) {

			switch (variableType) {
			case FMI2RealType:
				pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, i < nvr - 1 ? "%g, " : "%g", ((fmi2Real *)value)[i]);
				break;
			case FMI2IntegerType:
			case FMI2BooleanType:
				pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, i < nvr - 1 ? "%d, " : "%d", ((int *)value)[i]);
				break;
			case FMI2StringType:
				pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, i < nvr - 1 ? "\"%s\", " : "\"%s\"", ((fmi2String *)value)[i]);
				break;
			}

			if (pos > instance->bufsize2 - 2) {
				pos = 0;
				instance->bufsize2 *= 2;
				instance->buf2 = (char*)realloc(instance->buf2, instance->bufsize2);
				break;
			}
		}
	} while (pos == 0);

	pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, "}");

	return instance->buf2;
}

#define CALL_ARRAY(s, t) \
	fmi2Status status = instance->fmi2 ## s ## t(instance->component, vr, nvr, value); \
	if (instance->logFunctionCall) { \
		vrToString(instance, vr, nvr); \
		valueToString(instance, nvr, value, FMI2 ## t ## Type); \
		instance->logFunctionCall(status, instance->name, "fmi2" #s #t "(component=0x%p, vr=%s, nvr=%zu, value=%s)", instance->component, instance->buf1, nvr, instance->buf2); \
	} \
	return status;

/***************************************************
Common Functions
****************************************************/

/* Inquire version numbers of header files and setting logging status */
const char* FMI2GetTypesPlatform(FMI2Instance *instance) {
	if (!instance) return NULL;
	if (instance->logFunctionCall) {
			instance->logFunctionCall(fmi2OK, instance->name, "fmi2GetTypesPlatform()");
	}
	return instance->fmi2GetTypesPlatform();
}

const char* FMI2GetVersion(FMI2Instance *instance) {
	if (!instance) return NULL;
	if (instance->logFunctionCall) {
		instance->logFunctionCall(fmi2OK, instance->name, "fmi2GetVersion()");
	}
	return instance->fmi2GetVersion();
}

fmi2Status FMI2SetDebugLogging(FMI2Instance *instance, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {
	fmi2Status status = instance->fmi2SetDebugLogging(instance->component, loggingOn, nCategories, categories);
	if (instance->logFunctionCall) {
		valueToString(instance, nCategories, categories, FMI2StringType);
		instance->logFunctionCall(status, instance->name, "fmi2SetDebugLogging(component=0x%p, loggingOn=%d, nCategories=%zu, categories=%s)",
			instance->component, loggingOn, nCategories, instance->buf2);
	}
	return status;
}

/* Creation and destruction of FMU instances and setting debug status */
FMI2Instance* FMI2Instantiate(const char *unzipdir, const char *modelIdentifier, fmi2String instanceName, fmi2Type fmuType, fmi2String guid, fmi2Boolean visible, fmi2Boolean loggingOn) {

	FMI2Instance* instance = (FMI2Instance*)calloc(1, sizeof(FMI2Instance));

	if (!instance) {
		// TODO: log error
		return NULL;
	}

	instance->bufsize1 = 4;
	instance->bufsize2 = 4;

	instance->buf1 = (char*)calloc(instance->bufsize1, sizeof(char));
	instance->buf2 = (char*)calloc(instance->bufsize1, sizeof(char));

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
	Common Functions
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
	Model Exchange
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
	Co-Simulation
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

	CALL_ARGS(fmi2SetupExperiment, "toleranceDefined=%d, tolerance=%g, startTime=%g, stopTimeDefined=%d, stopTime=%g", toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
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
	CALL_ARRAY(Get, Real)
}

fmi2Status FMI2GetInteger(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
	CALL_ARRAY(Get, Integer)
}

fmi2Status FMI2GetBoolean(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
	CALL_ARRAY(Get, Boolean)
}

fmi2Status FMI2GetString(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
	CALL_ARRAY(Get, String)
}

fmi2Status FMI2SetReal(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
	CALL_ARRAY(Set, Real)
}

fmi2Status FMI2SetInteger(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
	CALL_ARRAY(Set, Integer)
}

fmi2Status FMI2SetBoolean(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
	CALL_ARRAY(Set, Boolean)
}

fmi2Status FMI2SetString(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
	CALL_ARRAY(Set, String)
}

/* Getting and setting the internal FMU state */
fmi2Status FMI2GetFMUstate(FMI2Instance *instance, fmi2FMUstate* FMUstate) {
	CALL_ARGS(fmi2GetFMUstate, "FMUstate=%p", FMUstate)
}

fmi2Status FMI2SetFMUstate(FMI2Instance *instance, fmi2FMUstate  FMUstate) {
	CALL_ARGS(fmi2SetFMUstate, "FMUstate=%p", FMUstate)
}

fmi2Status FMI2FreeFMUstate(FMI2Instance *instance, fmi2FMUstate* FMUstate) {
	CALL_ARGS(fmi2FreeFMUstate, "FMUstate=%p", FMUstate)
}

fmi2Status FMI2SerializedFMUstateSize(FMI2Instance *instance, fmi2FMUstate  FMUstate, size_t* size) {
	CALL_ARGS(fmi2SerializedFMUstateSize, "FMUstate=%p, size=0x%p", FMUstate, size);
}

fmi2Status FMI2SerializeFMUstate(FMI2Instance *instance, fmi2FMUstate  FMUstate, fmi2Byte serializedState[], size_t size) {
	CALL_ARGS(fmi2SerializeFMUstate, "FMUstate=%p, serializedState=%p, size=%zu", FMUstate, serializedState, size);
}

fmi2Status FMI2DeSerializeFMUstate(FMI2Instance *instance, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
	CALL_ARGS(fmi2DeSerializeFMUstate, "serializedState=0x%p, size=%zu, FMUstate=0x%p", serializedState, size, FMUstate);
}

/* Getting partial derivatives */
fmi2Status FMI2GetDirectionalDerivative(FMI2Instance *instance,
	const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
	const fmi2ValueReference vKnown_ref[], size_t nKnown,
	const fmi2Real dvKnown[],
	fmi2Real dvUnknown[]) {
	CALL_ARGS(fmi2GetDirectionalDerivative, "vUnknown_ref=0x%p, nUnknown=%zu, vKnown_ref=0x%p, nKnown=%zu, dvKnown=0x%p, dvUnknown=0x%p", 
		vUnknown_ref, nUnknown, vKnown_ref, nKnown, dvKnown, dvUnknown)
}

/***************************************************
Model Exchange
****************************************************/

/* Enter and exit the different modes */
fmi2Status FMI2EnterEventMode(FMI2Instance *instance) {
	CALL(fmi2EnterEventMode)
}

fmi2Status FMI2NewDiscreteStates(FMI2Instance *instance, fmi2EventInfo* fmi2eventInfo) {
	CALL_ARGS(fmi2NewDiscreteStates, "fmi2eventInfo=0x%p", fmi2eventInfo);
}

fmi2Status FMI2EnterContinuousTimeMode(FMI2Instance *instance) {
	CALL(fmi2EnterContinuousTimeMode)
}

fmi2Status FMI2CompletedIntegratorStep(FMI2Instance *instance,
	fmi2Boolean   noSetFMUStatePriorToCurrentPoint,
	fmi2Boolean*  enterEventMode,
	fmi2Boolean*  terminateSimulation) {
	CALL_ARGS(fmi2CompletedIntegratorStep, "noSetFMUStatePriorToCurrentPoint=%d, enterEventMode=0x%p, terminateSimulation=0x%p", noSetFMUStatePriorToCurrentPoint, enterEventMode, terminateSimulation);
}

/* Providing independent variables and re-initialization of caching */
fmi2Status FMI2SetTime(FMI2Instance *instance, fmi2Real time) {
	CALL_ARGS(fmi2SetTime, ", time=%g", time)
}

fmi2Status FMI2SetContinuousStates(FMI2Instance *instance, const fmi2Real x[], size_t nx) {
	fmi2Status status = instance->fmi2SetContinuousStates(instance->component, x, nx);
	if (instance->logFunctionCall) {
		valueToString(instance, nx, x, FMI2RealType);
		instance->logFunctionCall(status, instance->name, "fmi2SetContinuousStates(component=0x%p, x=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

/* Evaluation of the model equations */
fmi2Status FMI2GetDerivatives(FMI2Instance *instance, fmi2Real derivatives[], size_t nx) {
	fmi2Status status = instance->fmi2GetDerivatives(instance->component, derivatives, nx);
	if (instance->logFunctionCall) {
		valueToString(instance, nx, derivatives, FMI2RealType);
		instance->logFunctionCall(status, instance->name, "fmi2GetDerivatives(component=0x%p, derivatives=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

fmi2Status FMI2GetEventIndicators(FMI2Instance *instance, fmi2Real eventIndicators[], size_t ni) {
	fmi2Status status = instance->fmi2GetEventIndicators(instance->component, eventIndicators, ni);
	if (instance->logFunctionCall) {
		valueToString(instance, ni, eventIndicators, FMI2RealType);
		instance->logFunctionCall(status, instance->name, "fmi2GetEventIndicators(component=0x%p, eventIndicators=%s, ni=%zu)", instance->component, instance->buf2, ni);
	}
	return status;
}

fmi2Status FMI2GetContinuousStates(FMI2Instance *instance, fmi2Real x[], size_t nx) {
	fmi2Status status = instance->fmi2GetContinuousStates(instance->component, x, nx);
	if (instance->logFunctionCall) {
		valueToString(instance, nx, x, FMI2RealType);
		instance->logFunctionCall(status, instance->name, "fmi2GetContinuousStates(component=0x%p, x=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

fmi2Status FMI2GetNominalsOfContinuousStates(FMI2Instance *instance, fmi2Real x_nominal[], size_t nx) {
	fmi2Status status = instance->fmi2GetNominalsOfContinuousStates(instance->component, x_nominal, nx);
	if (instance->logFunctionCall) {
		valueToString(instance, nx, x_nominal, FMI2RealType);
		instance->logFunctionCall(status, instance->name, "fmi2GetNominalsOfContinuousStates(component=0x%p, x=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

/***************************************************
Co-Simulation
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
	CALL_ARGS(fmi2GetRealOutputDerivatives, "vr=0x%p, nvr=%zu, order=0x%p, value=0x%p", vr, nvr, order, value)
}

fmi2Status FMI2DoStep(FMI2Instance *instance,
	fmi2Real      currentCommunicationPoint,
	fmi2Real      communicationStepSize,
	fmi2Boolean   noSetFMUStatePriorToCurrentPoint) {
	CALL_ARGS(fmi2DoStep, "currentCommunicationPoint=%g, communicationStepSize=%g, noSetFMUStatePriorToCurrentPoint=%d",
		currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint)
}

fmi2Status FMI2CancelStep(FMI2Instance *instance) {
	CALL(fmi2CancelStep);
}

/* Inquire slave status */
fmi2Status FMI2GetStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2Status* value) {
	CALL_ARGS(fmi2GetStatus, "s=%d, value=0x%p", s, value)
}

fmi2Status FMI2GetRealStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2Real* value) {
	CALL_ARGS(fmi2GetRealStatus, "s=%d, value=0x%p", s, value)
}

fmi2Status FMI2GetIntegerStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2Integer* value) {
	CALL_ARGS(fmi2GetIntegerStatus, "s=%d, value=0x%p", s, value)
}

fmi2Status FMI2GetBooleanStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2Boolean* value) {
	CALL_ARGS(fmi2GetBooleanStatus, "s=%d, value=0x%p", s, value)
}

fmi2Status FMI2GetStringStatus(FMI2Instance *instance, const fmi2StatusKind s, fmi2String* value) {
	CALL_ARGS(fmi2GetStringStatus, "s=%d, value=0x%p", s, value)
}
