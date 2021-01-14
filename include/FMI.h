#ifndef FMI_H
#define FMI_H

#ifdef _WIN32
#include <Windows.h>
#endif

#include "fmi2Functions.h"

typedef enum {
	FMIModelExchange,
	FMICoSimulation
} FMIInterfaceType;

typedef enum {
	FMIOK,
	FMIWarning,
	FMIError,
	FMIFatal
} FMIStatus;

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

typedef enum {
	FMI2RealType,
	FMI2IntegerType,
	FMI2BooleanType,
	FMI2StringType
} FMI2VariableType;

struct _FMIInstance;

typedef struct _FMIInstance FMIInstance;

typedef void FMILogFunctionCallTYPE(FMIInstance *instance, FMIStatus status, const char *message, ...);

typedef void FMILogMessageTYPE(FMIInstance *instance, FMIStatus status, const char *category, const char *message);

struct _FMIInstance {

#ifdef _WIN32
	HMODULE libraryHandle;
#else
	void *libraryHandle;
#endif
	const char *name;

	FMIInterfaceType interfaceType;

	FMILogMessageTYPE *logMessage;

	FMILogFunctionCallTYPE *logFunctionCall;

	double time;

	char *buf1;
	char *buf2;

	size_t bufsize1;
	size_t bufsize2;

	void *component;

	FMI2State state;

	fmi2EventInfo eventInfo2;

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
	
};

FMIInstance *FMICreateInstance(const char *instanceName, const char *libraryPath, FMILogMessageTYPE *logMessage, FMILogFunctionCallTYPE *logFunctionCall);

void FMIFreeInstance(FMIInstance *instance);

#endif // FMI_H
