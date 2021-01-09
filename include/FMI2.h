#include <stdbool.h>
#include "fmi2Functions.h"

#ifdef _WIN32
#include <Windows.h>

static const char * exceptionCodeToString(DWORD exceptionCode) {
	switch (exceptionCode) {
	case STATUS_WAIT_0:						return "WAIT_0";
	case STATUS_ABANDONED_WAIT_0:			return "ABANDONED_WAIT_0";
	case STATUS_USER_APC:					return "USER_APC";
	case STATUS_TIMEOUT:					return "TIMEOUT";
	case STATUS_PENDING:					return "PENDING";
	case DBG_EXCEPTION_HANDLED:				return "EXCEPTION_HANDLED";
	case DBG_CONTINUE:						return "CONTINUE";
	case STATUS_SEGMENT_NOTIFICATION:		return "SEGMENT_NOTIFICATION";
	case STATUS_FATAL_APP_EXIT:				return "FATAL_APP_EXIT";
	case DBG_TERMINATE_THREAD:				return "TERMINATE_THREAD";
	case DBG_TERMINATE_PROCESS:				return "TERMINATE_PROCESS";
	case DBG_CONTROL_C:						return "CONTROL_C";
	case DBG_PRINTEXCEPTION_C:				return "PRINTEXCEPTION_C";
	case DBG_RIPEXCEPTION:					return "RIPEXCEPTION";
	case DBG_CONTROL_BREAK:					return "CONTROL_BREAK";
	case DBG_COMMAND_EXCEPTION:				return "COMMAND_EXCEPTION";
	case STATUS_GUARD_PAGE_VIOLATION:		return "GUARD_PAGE_VIOLATION";
	case STATUS_DATATYPE_MISALIGNMENT:		return "DATATYPE_MISALIGNMENT";
	case STATUS_BREAKPOINT:					return "BREAKPOINT";
	case STATUS_SINGLE_STEP:				return "SINGLE_STEP";
	case STATUS_LONGJUMP:					return "LONGJUMP";
	case STATUS_UNWIND_CONSOLIDATE:			return "UNWIND_CONSOLIDATE";
	case DBG_EXCEPTION_NOT_HANDLED:			return "EXCEPTION_NOT_HANDLED";
	case STATUS_ACCESS_VIOLATION:			return "ACCESS_VIOLATION";
	case STATUS_IN_PAGE_ERROR:				return "IN_PAGE_ERROR";
	case STATUS_INVALID_HANDLE:				return "INVALID_HANDLE";
	case STATUS_INVALID_PARAMETER:			return "INVALID_PARAMETER";
	case STATUS_NO_MEMORY:					return "NO_MEMORY";
	case STATUS_ILLEGAL_INSTRUCTION:		return "ILLEGAL_INSTRUCTION";
	case STATUS_NONCONTINUABLE_EXCEPTION:	return "NONCONTINUABLE_EXCEPTION";
	case STATUS_INVALID_DISPOSITION:		return "INVALID_DISPOSITION";
	case STATUS_ARRAY_BOUNDS_EXCEEDED:		return "ARRAY_BOUNDS_EXCEEDED";
	case STATUS_FLOAT_DENORMAL_OPERAND:		return "FLOAT_DENORMAL_OPERAND";
	case STATUS_FLOAT_DIVIDE_BY_ZERO:		return "FLOAT_DIVIDE_BY_ZERO";
	case STATUS_FLOAT_INEXACT_RESULT:		return "FLOAT_INEXACT_RESULT";
	case STATUS_FLOAT_INVALID_OPERATION:	return "FLOAT_INVALID_OPERATION";
	case STATUS_FLOAT_OVERFLOW:				return "FLOAT_OVERFLOW";
	case STATUS_FLOAT_STACK_CHECK:			return "FLOAT_STACK_CHECK";
	case STATUS_FLOAT_UNDERFLOW:			return "FLOAT_UNDERFLOW";
	case STATUS_INTEGER_DIVIDE_BY_ZERO:		return "INTEGER_DIVIDE_BY_ZERO";
	case STATUS_INTEGER_OVERFLOW:			return "INTEGER_OVERFLOW";
	case STATUS_PRIVILEGED_INSTRUCTION:		return "PRIVILEGED_INSTRUCTION";
	case STATUS_STACK_OVERFLOW:				return "STACK_OVERFLOW";
	case STATUS_DLL_NOT_FOUND:				return "DLL_NOT_FOUND";
	case STATUS_ORDINAL_NOT_FOUND:			return "ORDINAL_NOT_FOUND";
	case STATUS_ENTRYPOINT_NOT_FOUND:		return "ENTRYPOINT_NOT_FOUND";
	case STATUS_CONTROL_C_EXIT:				return "CONTROL_C_EXIT";
	case STATUS_DLL_INIT_FAILED:			return "DLL_INIT_FAILED";
	case STATUS_FLOAT_MULTIPLE_FAULTS:		return "FLOAT_MULTIPLE_FAULTS";
	case STATUS_FLOAT_MULTIPLE_TRAPS:		return "FLOAT_MULTIPLE_TRAPS";
	case STATUS_REG_NAT_CONSUMPTION:		return "REG_NAT_CONSUMPTION";
	case STATUS_HEAP_CORRUPTION:			return "HEAP_CORRUPTION";
	case STATUS_STACK_BUFFER_OVERRUN:		return "STACK_BUFFER_OVERRUN";
	case STATUS_INVALID_CRUNTIME_PARAMETER: return "INVALID_CRUNTIME_PARAMETER";
	case STATUS_ASSERTION_FAILURE:			return "ASSERTION_FAILURE";
	case STATUS_SXS_EARLY_DEACTIVATION:		return "SXS_EARLY_DEACTIVATION";
	case STATUS_SXS_INVALID_DEACTIVATION:	return "SXS_INVALID_DEACTIVATION";
	default:								return "UNKOWN_EXEPTION_CODE";
	}
}
#endif


typedef enum {

	FMI2StartAndEndState = 1 << 0,
	FMI2InstantiatedState = 1 << 1,
	FMI2InitializationModeState = 1 << 2,

	// model exchange states
	FMI2EventModeState = 1 << 3,
	FMI2ContinuousTimeModeState = 1 << 4,

	// co-simulation states
	FMI2StepCompleteState = 1 << 5,
	FMI2StepInProgressState = 1 << 6,
	FMI2StepFailedState = 1 << 7,
	FMI2StepCanceledState = 1 << 8,

	FMI2TerminatedState = 1 << 9,
	FMI2ErrorState = 1 << 10,
	FMI2FatalState = 1 << 11,

} FMI2State;

typedef void FMI2LogFunctionCallTYPE(fmi2Status status, const char *instanceName, const char *message);

typedef void FMI2LogMessageTYPE(fmi2String instanceName,
	fmi2Status status,
	fmi2String category,
	fmi2String message);

typedef struct {

	/***************************************************
	Common Functions for FMI 2.0
	****************************************************/

	/* required functions */
	fmi2GetTypesPlatformTYPE         *fmi2GetTypesPlatform;
	fmi2GetVersionTYPE               *fmi2GetVersion;
	fmi2SetDebugLoggingTYPE          *fmi2SetDebugLogging;
	fmi2InstantiateTYPE              *fmi2Instantiate;
	fmi2FreeInstanceTYPE             *fmi2FreeInstance;
	fmi2SetupExperimentTYPE          *fmi2SetupExperiment;
	fmi2EnterInitializationModeTYPE  *fmi2EnterInitializationMode;
	fmi2ExitInitializationModeTYPE   *fmi2ExitInitializationMode;
	fmi2TerminateTYPE                *fmi2Terminate;
	fmi2ResetTYPE                    *fmi2Reset;
	fmi2GetRealTYPE                  *fmi2GetReal;
	fmi2GetIntegerTYPE               *fmi2GetInteger;
	fmi2GetBooleanTYPE               *fmi2GetBoolean;
	fmi2GetStringTYPE                *fmi2GetString;
	fmi2SetRealTYPE                  *fmi2SetReal;
	fmi2SetIntegerTYPE               *fmi2SetInteger;
	fmi2SetBooleanTYPE               *fmi2SetBoolean;
	fmi2SetStringTYPE                *fmi2SetString;

	/* optional functions */
	fmi2GetFMUstateTYPE              *fmi2GetFMUstate;
	fmi2SetFMUstateTYPE              *fmi2SetFMUstate;
	fmi2FreeFMUstateTYPE             *fmi2FreeFMUstate;
	fmi2SerializedFMUstateSizeTYPE   *fmi2SerializedFMUstateSize;
	fmi2SerializeFMUstateTYPE        *fmi2SerializeFMUstate;
	fmi2DeSerializeFMUstateTYPE      *fmi2DeSerializeFMUstate;
	fmi2GetDirectionalDerivativeTYPE *fmi2GetDirectionalDerivative;

	/***************************************************
	Functions for FMI 2.0 for Model Exchange
	****************************************************/

	fmi2EnterEventModeTYPE                *fmi2EnterEventMode;
	fmi2NewDiscreteStatesTYPE             *fmi2NewDiscreteStates;
	fmi2EnterContinuousTimeModeTYPE       *fmi2EnterContinuousTimeMode;
	fmi2CompletedIntegratorStepTYPE       *fmi2CompletedIntegratorStep;
	fmi2SetTimeTYPE                       *fmi2SetTime;
	fmi2SetContinuousStatesTYPE           *fmi2SetContinuousStates;
	fmi2GetDerivativesTYPE                *fmi2GetDerivatives;
	fmi2GetEventIndicatorsTYPE            *fmi2GetEventIndicators;
	fmi2GetContinuousStatesTYPE           *fmi2GetContinuousStates;
	fmi2GetNominalsOfContinuousStatesTYPE *fmi2GetNominalsOfContinuousStates;

	/***************************************************
	Functions for FMI 2.0 for Co-Simulation
	****************************************************/

	fmi2SetRealInputDerivativesTYPE  *fmi2SetRealInputDerivatives;
	fmi2GetRealOutputDerivativesTYPE *fmi2GetRealOutputDerivatives;
	fmi2DoStepTYPE                   *fmi2DoStep;
	fmi2CancelStepTYPE               *fmi2CancelStep;
	fmi2GetStatusTYPE                *fmi2GetStatus;
	fmi2GetRealStatusTYPE            *fmi2GetRealStatus;
	fmi2GetIntegerStatusTYPE         *fmi2GetIntegerStatus;
	fmi2GetBooleanStatusTYPE         *fmi2GetBooleanStatus;
	fmi2GetStringStatusTYPE			 *fmi2GetStringStatus;

#ifdef _WIN32
	HMODULE libraryHandle;
#else
	void *libraryHandle;
#endif

	FMI2LogFunctionCallTYPE *logFunctionCall;

	fmi2Component component;

	fmi2String name;

	bool logFMICalls;

	FMI2State state;

	fmi2Type interfaceType;

} FMI2Instance;

FMI2Instance* FMI2Instantiate(const char *unzipdir, const char *modelIdentifier, fmi2String instanceName, fmi2Type fmuType, fmi2String guid, fmi2Boolean visible, fmi2Boolean loggingOn);

fmi2Status FMI2SetupExperiment(FMI2Instance *instance,
	fmi2Boolean toleranceDefined,
	fmi2Real tolerance,
	fmi2Real startTime,
	fmi2Boolean stopTimeDefined,
	fmi2Real stopTime);

fmi2Status FMI2EnterInitializationMode(FMI2Instance *instance);

fmi2Status FMI2ExitInitializationMode(FMI2Instance *instance);

fmi2Status FMI2Terminate(FMI2Instance *instance);

/* Getting and setting variable values */
fmi2Status FMI2GetReal(FMI2Instance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]);

/***************************************************
Types for Functions for FMI2 for Co-Simulation
****************************************************/

/* Simulating the slave */
fmi2Status FMI2DoStep(FMI2Instance *instance,
	fmi2Real      currentCommunicationPoint,
	fmi2Real      communicationStepSize,
	fmi2Boolean   noSetFMUStatePriorToCurrentPoint);