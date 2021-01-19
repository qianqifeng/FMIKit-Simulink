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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "FMI2.h"


static void cb_logMessage2(fmi2ComponentEnvironment componentEnvironment, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...) {
	FMIInstance *instance = componentEnvironment;
	// TODO: process variadic args
	instance->logMessage(instance, status, category, message);
}

#ifdef FMI2_FUNCTION_PREFIX
#define LOAD_SYMBOL(f) \
	instance->fmi2 ## f = fmi2 ## f;
#else
#define LOAD_SYMBOL(f) \
	instance->fmi2 ## f = (fmi2 ## f ## TYPE*)GetProcAddress(instance->libraryHandle, "fmi2" #f); \
	if (!instance->fmi2 ## f) goto fail;
#endif

#define CALL(f) \
	fmi2Status status = instance-> ## f (instance->component); \
	if (instance->logFunctionCall) { \
		instance->logFunctionCall(instance, status, #f "(component=0x%p)", instance->component); \
	} \
	return status;

#define CALL_ARGS(f, m, ...) \
	fmi2Status status = instance-> ## f (instance->component, __VA_ARGS__); \
	if (instance->logFunctionCall) { \
		instance->logFunctionCall(instance, status, #f "(component=0x%p, " m ")", instance->component, __VA_ARGS__); \
	} \
	return status;

#define CALL_ARRAY(s, t) \
	fmi2Status status = instance->fmi2 ## s ## t(instance->component, vr, nvr, value); \
	if (instance->logFunctionCall) { \
		FMIValueReferencesToString(instance, vr, nvr); \
		FMIValuesToString(instance, nvr, value, FMI2 ## t ## Type); \
		instance->logFunctionCall(instance, status, "fmi2" #s #t "(component=0x%p, vr=%s, nvr=%zu, value=%s)", instance->component, instance->buf1, nvr, instance->buf2); \
	} \
	return status;

/***************************************************
Common Functions
****************************************************/

/* Inquire version numbers of header files and setting logging status */
const char* FMI2GetTypesPlatform(FMIInstance *instance) {
	if (instance->logFunctionCall) {
			instance->logFunctionCall(instance, FMIOK, "fmi2GetTypesPlatform()");
	}
	return instance->fmi2GetTypesPlatform();
}

const char* FMI2GetVersion(FMIInstance *instance) {
	if (instance->logFunctionCall) {
		instance->logFunctionCall(instance, FMIOK, "fmi2GetVersion()");
	}
	return instance->fmi2GetVersion();
}

fmi2Status FMI2SetDebugLogging(FMIInstance *instance, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {
	fmi2Status status = instance->fmi2SetDebugLogging(instance->component, loggingOn, nCategories, categories);
	if (instance->logFunctionCall) {
		FMIValuesToString(instance, nCategories, categories, FMI2StringType);
		instance->logFunctionCall(instance, status, "fmi2SetDebugLogging(component=0x%p, loggingOn=%d, nCategories=%zu, categories=%s)",
			instance->component, loggingOn, nCategories, instance->buf2);
	}
	return status;
}

/* Creation and destruction of FMU instances and setting debug status */
fmi2Status FMI2Instantiate(FMIInstance *instance, const char *fmuResourceLocation, fmi2Type fmuType, fmi2String fmuGUID,
	fmi2Boolean visible, fmi2Boolean loggingOn) {

	instance->fmiVersion = FMIVersion2;

	instance->eventInfo2.newDiscreteStatesNeeded = fmi2False;
	instance->eventInfo2.terminateSimulation = fmi2False;
	instance->eventInfo2.nominalsOfContinuousStatesChanged = fmi2False;
	instance->eventInfo2.valuesOfContinuousStatesChanged = fmi2False;
	instance->eventInfo2.nextEventTimeDefined = fmi2False;
	instance->eventInfo2.nextEventTime = 0.0;

	instance->state = FMI2StartAndEndState;

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

	if (fmuType == fmi2ModelExchange) {

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

	} else {

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
	}

	instance->functions2.logger = cb_logMessage2;
	instance->functions2.allocateMemory = calloc;
	instance->functions2.freeMemory = free;
	instance->functions2.stepFinished = NULL;
	instance->functions2.componentEnvironment = instance;

	instance->component = instance->fmi2Instantiate(instance->name, fmuType, fmuGUID, fmuResourceLocation, &instance->functions2, visible, loggingOn);

	if (instance->logFunctionCall) {
		instance->logFunctionCall(instance, instance->component ? FMIOK : FMIError,
			"fmi2Instantiate(instanceName=\"%s\", fmuType=%d, fmuGUID=\"%s\", fmuResourceLocation=\"%s\", functions=0x%p, visible=%d, loggingOn=%d)",
			instance->name, fmuType, fmuGUID, fmuResourceLocation, &instance->functions2, visible, loggingOn);
	}

	if (!instance->component) goto fail;

	instance->state = FMI2InstantiatedState;

	return fmi2OK;

fail:
	return fmi2Error;
}

void FMI2FreeInstance(FMIInstance *instance) {
	
	instance->fmi2FreeInstance(instance->component);
	
	if (instance->logFunctionCall) {
		instance->logFunctionCall(instance, FMIOK, "fmi2FreeInstance(component=0x%p)", instance->component);
	}
}

/* Enter and exit initialization mode, terminate and reset */
fmi2Status FMI2SetupExperiment(FMIInstance *instance,
	fmi2Boolean toleranceDefined,
	fmi2Real tolerance,
	fmi2Real startTime,
	fmi2Boolean stopTimeDefined,
	fmi2Real stopTime) {

	instance->time = startTime;
	
	CALL_ARGS(fmi2SetupExperiment, "toleranceDefined=%d, tolerance=%g, startTime=%g, stopTimeDefined=%d, stopTime=%g", toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
}

fmi2Status FMI2EnterInitializationMode(FMIInstance *instance) {
	instance->state = FMI2InitializationModeState;
	CALL(fmi2EnterInitializationMode)
}

fmi2Status FMI2ExitInitializationMode(FMIInstance *instance) {
	instance->state = instance->interfaceType == fmi2ModelExchange ? FMI2EventModeState : FMI2StepCompleteState;
	CALL(fmi2ExitInitializationMode)
}

fmi2Status FMI2Terminate(FMIInstance *instance) {
	instance->state = FMI2TerminatedState;
	CALL(fmi2Terminate)
}

fmi2Status FMI2Reset(FMIInstance *instance) {
	CALL(fmi2Reset)
}

/* Getting and setting variable values */
fmi2Status FMI2GetReal(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
	CALL_ARRAY(Get, Real)
}

fmi2Status FMI2GetInteger(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
	CALL_ARRAY(Get, Integer)
}

fmi2Status FMI2GetBoolean(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
	CALL_ARRAY(Get, Boolean)
}

fmi2Status FMI2GetString(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
	CALL_ARRAY(Get, String)
}

fmi2Status FMI2SetReal(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
	CALL_ARRAY(Set, Real)
}

fmi2Status FMI2SetInteger(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
	CALL_ARRAY(Set, Integer)
}

fmi2Status FMI2SetBoolean(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
	CALL_ARRAY(Set, Boolean)
}

fmi2Status FMI2SetString(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
	CALL_ARRAY(Set, String)
}

/* Getting and setting the internal FMU state */
fmi2Status FMI2GetFMUstate(FMIInstance *instance, fmi2FMUstate* FMUstate) {
	CALL_ARGS(fmi2GetFMUstate, "FMUstate=%p", FMUstate)
}

fmi2Status FMI2SetFMUstate(FMIInstance *instance, fmi2FMUstate  FMUstate) {
	CALL_ARGS(fmi2SetFMUstate, "FMUstate=%p", FMUstate)
}

fmi2Status FMI2FreeFMUstate(FMIInstance *instance, fmi2FMUstate* FMUstate) {
	CALL_ARGS(fmi2FreeFMUstate, "FMUstate=%p", FMUstate)
}

fmi2Status FMI2SerializedFMUstateSize(FMIInstance *instance, fmi2FMUstate  FMUstate, size_t* size) {
	CALL_ARGS(fmi2SerializedFMUstateSize, "FMUstate=%p, size=0x%p", FMUstate, size);
}

fmi2Status FMI2SerializeFMUstate(FMIInstance *instance, fmi2FMUstate  FMUstate, fmi2Byte serializedState[], size_t size) {
	CALL_ARGS(fmi2SerializeFMUstate, "FMUstate=%p, serializedState=%p, size=%zu", FMUstate, serializedState, size);
}

fmi2Status FMI2DeSerializeFMUstate(FMIInstance *instance, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
	CALL_ARGS(fmi2DeSerializeFMUstate, "serializedState=0x%p, size=%zu, FMUstate=0x%p", serializedState, size, FMUstate);
}

/* Getting partial derivatives */
fmi2Status FMI2GetDirectionalDerivative(FMIInstance *instance,
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
fmi2Status FMI2EnterEventMode(FMIInstance *instance) {
	instance->state = FMI2EventModeState;
	CALL(fmi2EnterEventMode)
}

fmi2Status FMI2NewDiscreteStates(FMIInstance *instance, fmi2EventInfo* fmi2eventInfo) {
	CALL_ARGS(fmi2NewDiscreteStates, "fmi2eventInfo=0x%p", fmi2eventInfo);
}

fmi2Status FMI2EnterContinuousTimeMode(FMIInstance *instance) {
	instance->state = FMI2ContinuousTimeModeState;
	CALL(fmi2EnterContinuousTimeMode)
}

fmi2Status FMI2CompletedIntegratorStep(FMIInstance *instance,
	fmi2Boolean   noSetFMUStatePriorToCurrentPoint,
	fmi2Boolean*  enterEventMode,
	fmi2Boolean*  terminateSimulation) {
	CALL_ARGS(fmi2CompletedIntegratorStep, "noSetFMUStatePriorToCurrentPoint=%d, enterEventMode=0x%p, terminateSimulation=0x%p", noSetFMUStatePriorToCurrentPoint, enterEventMode, terminateSimulation);
}

/* Providing independent variables and re-initialization of caching */
fmi2Status FMI2SetTime(FMIInstance *instance, fmi2Real time) {
	CALL_ARGS(fmi2SetTime, "time=%g", time)
}

fmi2Status FMI2SetContinuousStates(FMIInstance *instance, const fmi2Real x[], size_t nx) {
	fmi2Status status = instance->fmi2SetContinuousStates(instance->component, x, nx);
	if (instance->logFunctionCall) {
		FMIValuesToString(instance, nx, x, FMI2RealType);
		instance->logFunctionCall(instance, status, "fmi2SetContinuousStates(component=0x%p, x=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

/* Evaluation of the model equations */
fmi2Status FMI2GetDerivatives(FMIInstance *instance, fmi2Real derivatives[], size_t nx) {
	fmi2Status status = instance->fmi2GetDerivatives(instance->component, derivatives, nx);
	if (instance->logFunctionCall) {
		FMIValuesToString(instance, nx, derivatives, FMI2RealType);
		instance->logFunctionCall(instance, status, "fmi2GetDerivatives(component=0x%p, derivatives=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

fmi2Status FMI2GetEventIndicators(FMIInstance *instance, fmi2Real eventIndicators[], size_t ni) {
	fmi2Status status = instance->fmi2GetEventIndicators(instance->component, eventIndicators, ni);
	if (instance->logFunctionCall) {
		FMIValuesToString(instance, ni, eventIndicators, FMI2RealType);
		instance->logFunctionCall(instance, status, "fmi2GetEventIndicators(component=0x%p, eventIndicators=%s, ni=%zu)", instance->component, instance->buf2, ni);
	}
	return status;
}

fmi2Status FMI2GetContinuousStates(FMIInstance *instance, fmi2Real x[], size_t nx) {
	fmi2Status status = instance->fmi2GetContinuousStates(instance->component, x, nx);
	if (instance->logFunctionCall) {
		FMIValuesToString(instance, nx, x, FMI2RealType);
		instance->logFunctionCall(instance, status, "fmi2GetContinuousStates(component=0x%p, x=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

fmi2Status FMI2GetNominalsOfContinuousStates(FMIInstance *instance, fmi2Real x_nominal[], size_t nx) {
	fmi2Status status = instance->fmi2GetNominalsOfContinuousStates(instance->component, x_nominal, nx);
	if (instance->logFunctionCall) {
		FMIValuesToString(instance, nx, x_nominal, FMI2RealType);
		instance->logFunctionCall(instance, status, "fmi2GetNominalsOfContinuousStates(component=0x%p, x_nominal=%s, nx=%zu)", instance->component, instance->buf2, nx);
	}
	return status;
}

/***************************************************
Co-Simulation
****************************************************/

/* Simulating the slave */
fmi2Status FMI2SetRealInputDerivatives(FMIInstance *instance,
	const fmi2ValueReference vr[], size_t nvr,
	const fmi2Integer order[],
	const fmi2Real value[]) {
	CALL_ARGS(fmi2SetRealInputDerivatives, "vr=0x%p, nvr=%zu, order=0x%p, value=0x%p", vr, nvr, order, value);
}

fmi2Status FMI2GetRealOutputDerivatives(FMIInstance *instance,
	const fmi2ValueReference vr[], size_t nvr,
	const fmi2Integer order[],
	fmi2Real value[]) {
	CALL_ARGS(fmi2GetRealOutputDerivatives, "vr=0x%p, nvr=%zu, order=0x%p, value=0x%p", vr, nvr, order, value)
}

fmi2Status FMI2DoStep(FMIInstance *instance,
	fmi2Real      currentCommunicationPoint,
	fmi2Real      communicationStepSize,
	fmi2Boolean   noSetFMUStatePriorToCurrentPoint) {

	instance->time = currentCommunicationPoint + communicationStepSize;
	
	CALL_ARGS(fmi2DoStep, "currentCommunicationPoint=%g, communicationStepSize=%g, noSetFMUStatePriorToCurrentPoint=%d",
		currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint)
}

fmi2Status FMI2CancelStep(FMIInstance *instance) {
	CALL(fmi2CancelStep);
}

/* Inquire slave status */
fmi2Status FMI2GetStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2Status* value) {
	CALL_ARGS(fmi2GetStatus, "s=%d, value=0x%p", s, value)
}

fmi2Status FMI2GetRealStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2Real* value) {
	CALL_ARGS(fmi2GetRealStatus, "s=%d, value=0x%p", s, value)
}

fmi2Status FMI2GetIntegerStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2Integer* value) {
	CALL_ARGS(fmi2GetIntegerStatus, "s=%d, value=0x%p", s, value)
}

fmi2Status FMI2GetBooleanStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2Boolean* value) {
	CALL_ARGS(fmi2GetBooleanStatus, "s=%d, value=0x%p", s, value)
}

fmi2Status FMI2GetStringStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2String* value) {
	CALL_ARGS(fmi2GetStringStatus, "s=%d, value=0x%p", s, value)
}

#undef LOAD_SYMBOL
#undef CALL
#undef CALL_ARGS
#undef CALL_ARRAY
