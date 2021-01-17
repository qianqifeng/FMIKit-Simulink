//#ifdef _WIN32
//#include <direct.h>
//#include "Shlwapi.h"
//#pragma comment(lib, "shlwapi.lib")
//#define strdup _strdup
//#define INTERNET_MAX_URL_LENGTH 2083 // from wininet.h
//#else
//#include <stdarg.h>
//#include <dlfcn.h>
//#endif
//
#include <stdio.h>
#include <string.h>

#include "FMI1.h"
#include "FMI2.h"
//
//#define INITIAL_MESSAGE_BUFFER_SIZE 1024

// callback functions
static void *cb_allocateMemory(size_t nobj, size_t size) {
	return calloc(nobj, size);
}

static void cb_freeMemory(void* obj) {
	free(obj);
}

static void cb_logMessage(fmi1Component c, fmi1String instanceName, fmi1Status status, fmi1String category, fmi1String message, ...) {
	FMI2Instance *instance = (FMI2Instance *)c;
	instance->logMessage(instanceName, status, category, message);
}

#define MAX_SYMBOL_LENGTH 256

static void *loadSymbol(FMI2Instance *instance, const char *prefix, const char *name) {
	char fname[MAX_SYMBOL_LENGTH];
	strcpy(fname, prefix);
	strcat(fname, name);
	void *addr = GetProcAddress(instance->libraryHandle, fname);
	if (!addr) {
		instance->logFunctionCall(fmi2Error, instance->name, "Failed to load function \"%s\".", fname);
	}
	return addr;
}

#define LOAD_SYMBOL(f) \
	instance->fmi1 ## f = (fmi1 ## f ## TYPE*)loadSymbol(instance, modelIdentifier, "_fmi" #f); \
	if (!instance->fmi1 ## f) { \
		status = fmi1Error; \
		goto fail; \
	}

#define CALL(f) \
	fmi1Status status = instance-> ## f (instance->component); \
	if (instance->logFunctionCall) { \
		instance->logFunctionCall(status, instance->name, #f "(component=0x%p)", instance->component); \
	} \
	return status;

#define CALL_ARGS(f, m, ...) \
	fmi1Status status = instance-> ## f (instance->component, __VA_ARGS__); \
	if (instance->logFunctionCall) { \
		instance->logFunctionCall(status, instance->name, #f "(component=0x%p, " m ")", instance->component, __VA_ARGS__); \
	} \
	return status;


static const char* vrToString(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr) {

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

static const char* valueToString(FMI2Instance *instance, size_t nvr, const void *value, FMI1VariableType variableType) {

	size_t pos = 0;

	do {
		pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, "{");

		for (size_t i = 0; i < nvr; i++) {

			switch (variableType) {
			case FMI1RealType:
				pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, i < nvr - 1 ? "%g, " : "%g", ((fmi1Real *)value)[i]);
				break;
			case FMI1IntegerType:
			case FMI1BooleanType:
				pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, i < nvr - 1 ? "%d, " : "%d", ((int *)value)[i]);
				break;
			case FMI1StringType:
				pos += snprintf(&instance->buf2[pos], instance->bufsize2 - pos, i < nvr - 1 ? "\"%s\", " : "\"%s\"", ((fmi1String *)value)[i]);
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
	fmi1Status status = instance->fmi1 ## s ## t(instance->component, vr, nvr, value); \
	if (instance->logFunctionCall) { \
		vrToString(instance, vr, nvr); \
		valueToString(instance, nvr, value, FMI1 ## t ## Type); \
		instance->logFunctionCall(status, instance->name, "fmi1" #s #t "(component=0x%p, vr=%s, nvr=%zu, value=%s)", instance->component, instance->buf1, nvr, instance->buf2); \
	} \
	return status;

/***************************************************
 Common Functions for FMI 1.0
****************************************************/

fmi1Status    FMI1SetReal(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Real    value[]) {
	CALL_ARRAY(Set, Real)
}

fmi1Status    FMI1SetInteger(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer value[]) {
	CALL_ARRAY(Set, Integer)
}

fmi1Status    FMI1SetBoolean(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Boolean value[]) {
	CALL_ARRAY(Set, Boolean)
}

fmi1Status    FMI1SetString(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1String  value[]) {
	CALL_ARRAY(Set, String)
}

fmi1Status    FMI1GetReal(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, fmi1Real    value[]) {
	CALL_ARRAY(Get, Real)
}

fmi1Status    FMI1GetInteger(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, fmi1Integer value[]) {
	CALL_ARRAY(Get, Integer)
}

fmi1Status    FMI1GetBoolean(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, fmi1Boolean value[]) {
	CALL_ARRAY(Get, Boolean)
}

fmi1Status    FMI1GetString(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, fmi1String  value[]) {
	CALL_ARRAY(Get, String)
}

fmi1Status    FMI1SetDebugLogging(FMI2Instance *instance, fmi1Boolean loggingOn) {
	CALL_ARGS(fmi1SetDebugLogging, "loggingOn=%d", loggingOn)
}


/***************************************************
 FMI 1.0 for Model Exchange Functions
****************************************************/

const char*   FMI1GetModelTypesPlatform(FMI2Instance *instance) {
	if (instance->logFunctionCall) {
		instance->logFunctionCall(fmi1OK, instance->name, "fmiGetModelTypesPlatform()");
	}
	return instance->fmi1GetModelTypesPlatform();
}

const char*   FMI1GetVersion(FMI2Instance *instance) {
	if (instance->logFunctionCall) {
		instance->logFunctionCall(fmi1OK, instance->name, "fmiGetVersion()");
	}
	return instance->fmi1GetVersion();
}

fmi1Status FMI1InstantiateModel(FMI2Instance *instance, fmi1String modelIdentifier, fmi1String GUID, fmi1Boolean loggingOn) {

	fmi1Status status = fmi1OK;
	
	/***************************************************
	 Common Functions for FMI 1.0
	****************************************************/
	LOAD_SYMBOL(SetReal)
	LOAD_SYMBOL(SetInteger)
	LOAD_SYMBOL(SetBoolean)
	LOAD_SYMBOL(SetString)
	LOAD_SYMBOL(GetReal)
	LOAD_SYMBOL(GetInteger)
	LOAD_SYMBOL(GetBoolean)
	LOAD_SYMBOL(GetString)
	LOAD_SYMBOL(SetDebugLogging)

	/***************************************************
		FMI 1.0 for Model Exchange Functions
	****************************************************/
	LOAD_SYMBOL(GetModelTypesPlatform)
	LOAD_SYMBOL(GetVersion)
	LOAD_SYMBOL(InstantiateModel)
	LOAD_SYMBOL(FreeModelInstance)
	LOAD_SYMBOL(SetTime)
	LOAD_SYMBOL(SetContinuousStates)
	LOAD_SYMBOL(CompletedIntegratorStep)
	LOAD_SYMBOL(Initialize)
	LOAD_SYMBOL(GetDerivatives)
	LOAD_SYMBOL(GetEventIndicators)
	LOAD_SYMBOL(EventUpdate)
	LOAD_SYMBOL(GetContinuousStates)
	LOAD_SYMBOL(GetNominalContinuousStates)
	LOAD_SYMBOL(GetStateValueReferences)
	LOAD_SYMBOL(Terminate)

	fmi1CallbackFunctions functions = {
		cb_logMessage,
		cb_allocateMemory,
		cb_freeMemory,
		NULL
	};

	instance->component = instance->fmi1InstantiateModel(instance->name, GUID, functions, loggingOn);

	status = instance->component ? fmi1OK : fmi1Error;

	if (instance->logFunctionCall) {
		instance->logFunctionCall(status, instance->name,
			"fmi1InstantiateModel(instanceName=\"%s\", GUID=\"%s\", functions=0x%p, loggingOn=%d)",
			instance->name, GUID, &functions, loggingOn);
	}

fail:
	return status;
}

void FMI1FreeModelInstance(FMI2Instance *instance) {

	instance->fmi1FreeModelInstance(instance->component);
	
	if (instance->logFunctionCall) {
		instance->logFunctionCall(fmi1OK, instance->name, "fmi1FreeModelInstance(component=0x%p)", instance->component);
	}
}

fmi1Status    FMI1SetTime(FMI2Instance *instance, fmi1Real time) {
	CALL_ARGS(fmi1SetTime, "time=%g", time)
}

fmi1Status    FMI1SetContinuousStates(FMI2Instance *instance, const fmi1Real x[], size_t nx) {
	fmi1Status status = instance->fmi1SetContinuousStates(instance->component, x, nx);
	if (instance->logFunctionCall) {
		valueToString(instance, nx, x, FMI1RealType);
		instance->logFunctionCall(status, instance->name, "fmi1SetContinuousStates(component=0x%p, x=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

fmi1Status    FMI1CompletedIntegratorStep(FMI2Instance *instance, fmi1Boolean* callEventUpdate) {
	CALL_ARGS(fmi1CompletedIntegratorStep, "callEventUpdate=0x%p", callEventUpdate);
}

fmi1Status    FMI1Initialize(FMI2Instance *instance, fmi1Boolean toleranceControlled, fmi1Real relativeTolerance) {
	CALL_ARGS(fmi1Initialize, "toleranceControlled=%d, relativeTolerance=%g, eventInfo=0x%p", toleranceControlled, relativeTolerance, &instance->eventInfo1);
}

fmi1Status    FMI1GetDerivatives(FMI2Instance *instance, fmi1Real derivatives[], size_t nx) {
	fmi1Status status = instance->fmi1GetDerivatives(instance->component, derivatives, nx);
	if (instance->logFunctionCall) {
		valueToString(instance, nx, derivatives, FMI1RealType);
		instance->logFunctionCall(status, instance->name, "fmi1GetDerivatives(component=0x%p, derivatives=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

fmi1Status    FMI1GetEventIndicators(FMI2Instance *instance, fmi1Real eventIndicators[], size_t ni) {
	fmi1Status status = instance->fmi1GetEventIndicators(instance->component, eventIndicators, ni);
	if (instance->logFunctionCall) {
		valueToString(instance, ni, eventIndicators, FMI1RealType);
		instance->logFunctionCall(status, instance->name, "fmi1GetEventIndicators(component=0x%p, eventIndicators=%s, ni=%zu)", instance->component, instance->buf2, ni);
	}
	return status;
}

fmi1Status    FMI1EventUpdate(FMI2Instance *instance, fmi1Boolean intermediateResults, fmi1EventInfo* eventInfo) {
	CALL_ARGS(fmi1EventUpdate, "intermediateResults=%d, eventInfo=0x%p", intermediateResults, eventInfo);
}

fmi1Status    FMI1GetContinuousStates(FMI2Instance *instance, fmi1Real states[], size_t nx) {
	fmi1Status status = instance->fmi1GetContinuousStates(instance->component, states, nx);
	if (instance->logFunctionCall) {
		valueToString(instance, nx, states, FMI1RealType);
		instance->logFunctionCall(status, instance->name, "fmi2GetContinuousStates(component=0x%p, x=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

fmi1Status    FMI1GetNominalContinuousStates(FMI2Instance *instance, fmi1Real x_nominal[], size_t nx) {
	fmi1Status status = instance->fmi1GetNominalContinuousStates(instance->component, x_nominal, nx);
	if (instance->logFunctionCall) {
		valueToString(instance, nx, x_nominal, FMI1RealType);
		instance->logFunctionCall(status, instance->name, "fmi1GetNominalContinuousStates(component=0x%p, x_nominal=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

fmi1Status    FMI1GetStateValueReferences(FMI2Instance *instance, fmi1ValueReference vrx[], size_t nx) {
	fmi1Status status = instance->fmi1GetStateValueReferences(instance->component, vrx, nx);
	if (instance->logFunctionCall) {
		// TODO
	}
	return status;
}

fmi1Status FMI1Terminate(FMI2Instance *instance) {
	CALL(fmi1Terminate)
}

/***************************************************
 FMI 1.0 for Co-Simulation Functions
****************************************************/

const char*   FMI1GetTypesPlatform(FMI2Instance *instance) {
	if (instance->logFunctionCall) {
		instance->logFunctionCall(fmi1OK, instance->name, "fmi1GetTypesPlatform()");
	}
	return instance->fmi1GetTypesPlatform();
}

fmi1Status FMI1InstantiateSlave(FMI2Instance *instance, fmi1String modelIdentifier, fmi1String fmuGUID, fmi1String fmuLocation, fmi1String  mimeType, fmi1Real timeout, fmi1Boolean visible, fmi1Boolean interactive, fmi1Boolean loggingOn) {

	//instance->fmiVersion = FMIVersion1;

	fmi1Status status = fmi1OK;

	/***************************************************
	 Common Functions for FMI 1.0
	****************************************************/
	LOAD_SYMBOL(SetReal)
	LOAD_SYMBOL(SetInteger)
	LOAD_SYMBOL(SetBoolean)
	LOAD_SYMBOL(SetString)
	LOAD_SYMBOL(GetReal)
	LOAD_SYMBOL(GetInteger)
	LOAD_SYMBOL(GetBoolean)
	LOAD_SYMBOL(GetString)
	LOAD_SYMBOL(SetDebugLogging)

	///***************************************************
	// FMI 1.0 for Model Exchange Functions
	//****************************************************/
	//LOAD_SYMBOL(GetModelTypesPlatform)
	//LOAD_SYMBOL(GetVersion)
	//LOAD_SYMBOL(InstantiateModel)
	//LOAD_SYMBOL(FreeModelInstance)
	//LOAD_SYMBOL(SetTime)
	//LOAD_SYMBOL(SetContinuousStates)
	//LOAD_SYMBOL(CompletedIntegratorStep)
	//LOAD_SYMBOL(Initialize)
	//LOAD_SYMBOL(GetDerivatives)
	//LOAD_SYMBOL(GetEventIndicators)
	//LOAD_SYMBOL(EventUpdate)
	//LOAD_SYMBOL(GetContinuousStates)
	//LOAD_SYMBOL(GetNominalContinuousStates)
	//LOAD_SYMBOL(GetStateValueReferences)
	//LOAD_SYMBOL(Terminate)

	/***************************************************
	 FMI 1.0 for Co-Simulation Functions
	****************************************************/
	LOAD_SYMBOL(GetTypesPlatform)
	LOAD_SYMBOL(InstantiateSlave)
	LOAD_SYMBOL(InitializeSlave)
	LOAD_SYMBOL(TerminateSlave)
	LOAD_SYMBOL(ResetSlave)
	LOAD_SYMBOL(FreeSlaveInstance)
	LOAD_SYMBOL(SetRealInputDerivatives)
	LOAD_SYMBOL(GetRealOutputDerivatives)
	LOAD_SYMBOL(CancelStep)
	LOAD_SYMBOL(DoStep)
	LOAD_SYMBOL(GetStatus)
	LOAD_SYMBOL(GetRealStatus)
	LOAD_SYMBOL(GetIntegerStatus)
	LOAD_SYMBOL(GetBooleanStatus)
	LOAD_SYMBOL(GetStringStatus)

	fmi1CallbackFunctions functions = {
		cb_logMessage,
		cb_allocateMemory,
		cb_freeMemory,
		NULL
	};

	instance->component = instance->fmi1InstantiateSlave(instance->name, fmuGUID, fmuLocation, mimeType, timeout, visible, interactive, functions, loggingOn);

	status = instance->component ? fmi1OK : fmi1Error;

	if (instance->logFunctionCall) {
		instance->logFunctionCall(status, instance->name,
			"fmi1InstantiateSlave(instanceName=\"%s\", fmuGUID=\"%s\", fmuLocation=\"%s\", mimeType=\"%s\", timeout=%g, visible=%d, interactive=%d, functions=0x%p, loggingOn=%d)",
			instance->name, fmuGUID, fmuLocation, mimeType, timeout, visible, interactive, &functions, loggingOn);
	}

fail:
	return status;
}

fmi1Status    FMI1InitializeSlave(FMI2Instance *instance, fmi1Real tStart, fmi1Boolean stopTimeDefined, fmi1Real tStop) {
	CALL_ARGS(fmi1InitializeSlave, "tStart=%g, stopTimeDefined=%d, tStop=%g", tStart, stopTimeDefined, tStop);
}

fmi1Status    FMI1TerminateSlave(FMI2Instance *instance) {
	CALL(fmi1TerminateSlave)
}

fmi1Status    FMI1ResetSlave(FMI2Instance *instance) {
	CALL(fmi1ResetSlave)
}

void FMI1FreeSlaveInstance(FMI2Instance *instance) {

	instance->fmi1FreeSlaveInstance(instance->component);

	if (instance->logFunctionCall) {
		instance->logFunctionCall(fmi1OK, instance->name, "fmi1FreeSlaveInstance(component=0x%p)", instance->component);
	}
}

fmi1Status    FMI1SetRealInputDerivatives(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[], const fmi1Real value[]) {
	CALL_ARGS(fmi1SetRealInputDerivatives, "vr=0x%p, nvr=%zu, order=0x%p, value=0x%p", vr, nvr, order, value);
}

fmi1Status    FMI1GetRealOutputDerivatives(FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[], fmi1Real value[]) {
	CALL_ARGS(fmi1GetRealOutputDerivatives, "vr=0x%p, nvr=%zu, order=0x%p, value=0x%p", vr, nvr, order, value)
}

fmi1Status    FMI1CancelStep(FMI2Instance *instance) {
	CALL(fmi1CancelStep)
}

fmi1Status    FMI1DoStep(FMI2Instance *instance, fmi1Real currentCommunicationPoint, fmi1Real communicationStepSize, fmi1Boolean newStep) {

	instance->time = currentCommunicationPoint + communicationStepSize;

	CALL_ARGS(fmi1DoStep, "currentCommunicationPoint=%g, communicationStepSize=%g, newStep=%d",
		currentCommunicationPoint, communicationStepSize, newStep)
}

fmi1Status FMI1GetStatus(FMI2Instance *instance, const fmi1StatusKind s, fmi1Status* value) {
	CALL_ARGS(fmi1GetStatus, "s=%d, value=0x%p", s, value)
}

fmi1Status FMI1GetRealStatus(FMI2Instance *instance, const fmi1StatusKind s, fmi1Real* value) {
	CALL_ARGS(fmi1GetRealStatus, "s=%d, value=0x%p", s, value)
}

fmi1Status FMI1GetIntegerStatus(FMI2Instance *instance, const fmi1StatusKind s, fmi1Integer* value) {
	CALL_ARGS(fmi1GetIntegerStatus, "s=%d, value=0x%p", s, value)
}

fmi1Status FMI1GetBooleanStatus(FMI2Instance *instance, const fmi1StatusKind s, fmi1Boolean* value) {
	CALL_ARGS(fmi1GetBooleanStatus, "s=%d, value=0x%p", s, value)
}

fmi1Status FMI1GetStringStatus(FMI2Instance *instance, const fmi1StatusKind s, fmi1String* value) {
	CALL_ARGS(fmi1GetStringStatus, "s=%d, value=0x%p", s, value)
}
