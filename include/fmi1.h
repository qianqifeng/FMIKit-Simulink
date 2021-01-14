#ifndef FMI1_H
#define FMI1_H

#ifdef _WIN32
#include <Windows.h>
#endif

#include "FMI2.h"

typedef enum {

    FMI1RealType,
    FMI1IntegerType,
    FMI1BooleanType,
    FMI1StringType
  
} FMI1VariableType;

/***************************************************
 Common Functions for FMI 1.0
****************************************************/
fmi1Status    FMI1SetReal         (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Real    value[]);
fmi1Status    FMI1SetInteger      (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer value[]);
fmi1Status    FMI1SetBoolean      (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Boolean value[]);
fmi1Status    FMI1SetString       (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1String  value[]);
fmi1Status    FMI1GetReal         (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1Real    value[]);
fmi1Status    FMI1GetInteger      (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1Integer value[]);
fmi1Status    FMI1GetBoolean      (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1Boolean value[]);
fmi1Status    FMI1GetString       (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1String  value[]);
fmi1Status    FMI1SetDebugLogging (FMI2Instance *instance, fmi1Boolean loggingOn);

/***************************************************
 FMI 1.0 for Model Exchange Functions
****************************************************/
const char*   FMI1GetModelTypesPlatform      (FMI2Instance *instance);
const char*   FMI1GetVersion                 (FMI2Instance *instance);
fmi1Status    FMI1InstantiateModel           (FMI2Instance *instance, fmi1String modelIdentifier, fmi1String GUID, fmi1Boolean loggingOn);
void          FMI1FreeModelInstance          (FMI2Instance *instance);
fmi1Status    FMI1SetTime                    (FMI2Instance *instance, fmi1Real time);
fmi1Status    FMI1SetContinuousStates        (FMI2Instance *instance, const fmi1Real x[], size_t nx);
fmi1Status    FMI1CompletedIntegratorStep    (FMI2Instance *instance, fmi1Boolean* callEventUpdate);
fmi1Status    FMI1Initialize                 (FMI2Instance *instance, fmi1Boolean toleranceControlled, fmi1Real relativeTolerance);
fmi1Status    FMI1GetDerivatives             (FMI2Instance *instance, fmi1Real derivatives[], size_t nx);
fmi1Status    FMI1GetEventIndicators         (FMI2Instance *instance, fmi1Real eventIndicators[], size_t ni);
fmi1Status    FMI1EventUpdate                (FMI2Instance *instance, fmi1Boolean intermediateResults, fmi1EventInfo* eventInfo);
fmi1Status    FMI1GetContinuousStates        (FMI2Instance *instance, fmi1Real states[], size_t nx);
fmi1Status    FMI1GetNominalContinuousStates (FMI2Instance *instance, fmi1Real x_nominal[], size_t nx);
fmi1Status    FMI1GetStateValueReferences    (FMI2Instance *instance, fmi1ValueReference vrx[], size_t nx);
fmi1Status    FMI1Terminate                  (FMI2Instance *instance);

/***************************************************
 FMI 1.0 for Co-Simulation Functions
****************************************************/
const char*   FMI1GetTypesPlatform         (FMI2Instance *instance);
fmi1Status    FMI1InstantiateSlave(FMI2Instance *instance, fmi1String modelIdentifier, fmi1String fmuGUID, fmi1String fmuLocation, fmi1String  mimeType, fmi1Real timeout, fmi1Boolean visible, fmi1Boolean interactive, fmi1Boolean loggingOn);
fmi1Status    FMI1InitializeSlave(FMI2Instance *instance, fmi1Real tStart, fmi1Boolean StopTimeDefined, fmi1Real tStop);
fmi1Status    FMI1TerminateSlave           (FMI2Instance *instance);
fmi1Status    FMI1ResetSlave               (FMI2Instance *instance);
void          FMI1FreeSlaveInstance        (FMI2Instance *instance);
fmi1Status    FMI1SetRealInputDerivatives  (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[], const fmi1Real value[]);
fmi1Status    FMI1GetRealOutputDerivatives (FMI2Instance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[],       fmi1Real value[]);
fmi1Status    FMI1CancelStep               (FMI2Instance *instance);
fmi1Status    FMI1DoStep                   (FMI2Instance *instance, fmi1Real currentCommunicationPoint, fmi1Real communicationStepSize, fmi1Boolean newStep);
fmi1Status    FMI1GetStatus                (FMI2Instance *instance, const fmi1StatusKind s, fmi1Status*  value);
fmi1Status    FMI1GetRealStatus            (FMI2Instance *instance, const fmi1StatusKind s, fmi1Real*    value);
fmi1Status    FMI1GetIntegerStatus         (FMI2Instance *instance, const fmi1StatusKind s, fmi1Integer* value);
fmi1Status    FMI1GetBooleanStatus         (FMI2Instance *instance, const fmi1StatusKind s, fmi1Boolean* value);
fmi1Status    FMI1GetStringStatus          (FMI2Instance *instance, const fmi1StatusKind s, fmi1String*  value);

#endif // FMI1_H
