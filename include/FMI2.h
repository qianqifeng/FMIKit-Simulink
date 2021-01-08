#include <stdbool.h>
#include "fmi2Functions.h"

#ifdef _WIN32
#include <Windows.h>
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

	fmi2Component component;

	fmi2String name;

	bool logFMICalls;

	FMI2State state;

	fmi2Type interfaceType;

} FMI2Instance;

FMI2Instance* FMI2Instantiate(const char *unzipdir, fmi2String instanceName, fmi2Type fmuType, fmi2String guid, fmi2Boolean visible, fmi2Boolean loggingOn);

fmi2Status FMI2SetupExperiment(FMI2Instance *instance,
	fmi2Boolean toleranceDefined,
	fmi2Real tolerance,
	fmi2Real startTime,
	fmi2Boolean stopTimeDefined,
	fmi2Real stopTime);

fmi2Status FMI2EnterInitializationMode(fmi2Component c);

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