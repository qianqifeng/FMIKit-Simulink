#ifndef FMI1_H
#define FMI1_H

#ifdef _WIN32
#include <Windows.h>
#endif

#include <stdbool.h>
#include "fmi1Functions.h"

//  typedef enum {
//
//      FMI2StartAndEndState = 1 << 0,
//      FMI2InstantiatedState = 1 << 1,
//      FMI2InitializationModeState = 1 << 2,
//
//      // model exchange states
//      FMI2EventModeState = 1 << 3,
//      FMI2ContinuousTimeModeState = 1 << 4,
//
//      // co-simulation states
//      FMI2StepCompleteState = 1 << 5,
//      FMI2StepInProgressState = 1 << 6,
//      FMI2StepFailedState = 1 << 7,
//      FMI2StepCanceledState = 1 << 8,
//
//      FMI2TerminatedState = 1 << 9,
//      FMI2ErrorState = 1 << 10,
//      FMI2FatalState = 1 << 11,
//
//  } FMI2State;

typedef enum {

    FMI1RealType,
    FMI1IntegerType,
    FMI1BooleanType,
    FMI1StringType
  
} FMI1VariableType;

typedef void FMI1LogFunctionCallTYPE(fmi1Status status, const char *instanceName, const char *message, ...);

typedef void FMI1LogMessageTYPE(fmi1String instanceName,
    fmi1Status status,
    fmi1String category,
    fmi1String message);

typedef struct {

    /***************************************************
     Common Functions for FMI 1.0
    ****************************************************/
	fmi1SetRealTYPE         *fmi1SetReal;
    fmi1SetIntegerTYPE      *fmi1SetInteger;
    fmi1SetBooleanTYPE      *fmi1SetBoolean;
    fmi1SetStringTYPE       *fmi1SetString;
    fmi1GetRealTYPE         *fmi1GetReal;
    fmi1GetIntegerTYPE      *fmi1GetInteger;
    fmi1GetBooleanTYPE      *fmi1GetBoolean;
    fmi1GetStringTYPE       *fmi1GetString;
    fmi1SetDebugLoggingTYPE *fmi1SetDebugLogging;

    /***************************************************
     FMI 1.0 for Model Exchange Functions
    ****************************************************/
    fmi1GetModelTypesPlatformTYPE      *fmi1GetModelTypesPlatform;
    fmi1GetVersionTYPE                 *fmi1GetVersion;
    fmi1InstantiateModelTYPE           *fmi1InstantiateModel;
    fmi1FreeModelInstanceTYPE          *fmi1FreeModelInstance;
    fmi1SetTimeTYPE                    *fmi1SetTime;
    fmi1SetContinuousStatesTYPE        *fmi1SetContinuousStates;
    fmi1CompletedIntegratorStepTYPE    *fmi1CompletedIntegratorStep;
    fmi1InitializeTYPE                 *fmi1Initialize;
    fmi1GetDerivativesTYPE             *fmi1GetDerivatives;
    fmi1GetEventIndicatorsTYPE         *fmi1GetEventIndicators;
    fmi1EventUpdateTYPE                *fmi1EventUpdate;
    fmi1GetContinuousStatesTYPE        *fmi1GetContinuousStates;
    fmi1GetNominalContinuousStatesTYPE *fmi1GetNominalContinuousStates;
    fmi1GetStateValueReferencesTYPE    *fmi1GetStateValueReferences;    
    fmi1TerminateTYPE                  *fmi1Terminate;

    /***************************************************
     FMI 1.0 for Co-Simulation Functions
    ****************************************************/
	fmi1GetTypesPlatformTYPE         *fmi1GetTypesPlatform;
    fmi1InstantiateSlaveTYPE         *fmi1InstantiateSlave;
    fmi1InitializeSlaveTYPE          *fmi1InitializeSlave;
    fmi1TerminateSlaveTYPE           *fmi1TerminateSlave;
    fmi1ResetSlaveTYPE               *fmi1ResetSlave;
    fmi1FreeSlaveInstanceTYPE        *fmi1FreeSlaveInstance;
    fmi1SetRealInputDerivativesTYPE  *fmi1SetRealInputDerivatives;
    fmi1GetRealOutputDerivativesTYPE *fmi1GetRealOutputDerivatives;
    fmi1CancelStepTYPE               *fmi1CancelStep;
    fmi1DoStepTYPE                   *fmi1DoStep;
    fmi1GetStatusTYPE                *fmi1GetStatus;
    fmi1GetRealStatusTYPE            *fmi1GetRealStatus;
    fmi1GetIntegerStatusTYPE         *fmi1GetIntegerStatus;
    fmi1GetBooleanStatusTYPE         *fmi1GetBooleanStatus;
    fmi1GetStringStatusTYPE          *fmi1GetStringStatus;

#ifdef _WIN32
      HMODULE libraryHandle;
#else
      void *libraryHandle;
#endif

      FMI1LogFunctionCallTYPE *logFunctionCall;

      fmi1Real time;

      char *buf1;
      char *buf2;

      size_t bufsize1;
      size_t bufsize2;

      fmi1Component component;

      fmi1String name;
//
//      bool logFMICalls;
//
//      FMI2State state;
//
//      fmi2Type interfaceType;
//
//      fmi2EventInfo eventInfo;
//
} FMI1Instance;


/***************************************************
 Common Functions for FMI 1.0
****************************************************/
fmi1Status    FMI1SetReal         (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Real    value[]);
fmi1Status    FMI1SetInteger      (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer value[]);
fmi1Status    FMI1SetBoolean      (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Boolean value[]);
fmi1Status    FMI1SetString       (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1String  value[]);
fmi1Status    FMI1GetReal         (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1Real    value[]);
fmi1Status    FMI1GetInteger      (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1Integer value[]);
fmi1Status    FMI1GetBoolean      (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1Boolean value[]);
fmi1Status    FMI1GetString       (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1String  value[]);
fmi1Status    FMI1SetDebugLogging (FMI1Instance *instance, fmi1Boolean loggingOn);

/***************************************************
 FMI 1.0 for Model Exchange Functions
****************************************************/
const char*   FMI1GetModelTypesPlatform      (FMI1Instance *instance);
const char*   FMI1GetVersion                 (FMI1Instance *instance);
fmi1Component FMI1InstantiateModelTYPE       (fmi1String instanceName, fmi1String GUID, fmi1CallbackFunctions functions, fmi1Boolean loggingOn);
void          FMI1FreeModelInstance          (FMI1Instance *instance);
fmi1Status    FMI1SetTime                    (FMI1Instance *instance, fmi1Real time);
fmi1Status    FMI1SetContinuousStates        (FMI1Instance *instance, const fmi1Real x[], size_t nx);
fmi1Status    FMI1CompletedIntegratorStep    (FMI1Instance *instance, fmi1Boolean* callEventUpdate);
fmi1Status    FMI1Initialize                 (FMI1Instance *instance, fmi1Boolean toleranceControlled, fmi1Real relativeTolerance, fmi1EventInfo* eventInfo);
fmi1Status    FMI1GetDerivatives             (FMI1Instance *instance, fmi1Real derivatives[], size_t nx);
fmi1Status    FMI1GetEventIndicators         (FMI1Instance *instance, fmi1Real eventIndicators[], size_t ni);
fmi1Status    FMI1EventUpdate                (FMI1Instance *instance, fmi1Boolean intermediateResults, fmi1EventInfo* eventInfo);
fmi1Status    FMI1GetContinuousStates        (FMI1Instance *instance, fmi1Real states[], size_t nx);
fmi1Status    FMI1GetNominalContinuousStates (FMI1Instance *instance, fmi1Real x_nominal[], size_t nx);
fmi1Status    FMI1GetStateValueReferences    (FMI1Instance *instance, fmi1ValueReference vrx[], size_t nx);
fmi1Status    FMI1Terminate                  (FMI1Instance *instance);

/***************************************************
 FMI 1.0 for Co-Simulation Functions
****************************************************/
const char*   FMI1GetTypesPlatform         (FMI1Instance *instance);
fmi1Component FMI1InstantiateSlave         (fmi1String  instanceName, fmi1String  fmuGUID, fmi1String  fmuLocation,fmi1String  mimeType, fmi1Real timeout, fmi1Boolean visible, fmi1Boolean interactive, fmi1CallbackFunctions functions, fmi1Boolean loggingOn);
fmi1Status    FMI1InitializeSlave          (FMI1Instance *instance, fmi1Real tStart, fmi1Boolean StopTimeDefined, fmi1Real tStop);
fmi1Status    FMI1TerminateSlave           (FMI1Instance *instance);
fmi1Status    FMI1ResetSlave               (FMI1Instance *instance);
void          FMI1FreeSlaveInstance        (FMI1Instance *instance);
fmi1Status    FMI1SetRealInputDerivatives  (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[], const fmi1Real value[]);
fmi1Status    FMI1GetRealOutputDerivatives (FMI1Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[],       fmi1Real value[]);
fmi1Status    FMI1CancelStep               (FMI1Instance *instance);
fmi1Status    FMI1DoStep                   (FMI1Instance *instance, fmi1Real currentCommunicationPoint, fmi1Real communicationStepSize, fmi1Boolean newStep);
fmi1Status    FMI1GetStatus                (FMI1Instance *instance, const fmi1StatusKind s, fmi1Status*  value);
fmi1Status    FMI1GetRealStatus            (FMI1Instance *instance, const fmi1StatusKind s, fmi1Real*    value);
fmi1Status    FMI1GetIntegerStatus         (FMI1Instance *instance, const fmi1StatusKind s, fmi1Integer* value);
fmi1Status    FMI1GetBooleanStatus         (FMI1Instance *instance, const fmi1StatusKind s, fmi1Boolean* value);
fmi1Status    FMI1GetStringStatus          (FMI1Instance *instance, const fmi1StatusKind s, fmi1String*  value);

#endif // FMI1_H
