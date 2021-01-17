/*****************************************************************
 *  Copyright (c) Dassault Systemes. All rights reserved.        *
 *  This file is part of FMIKit. See LICENSE.txt in the project  *
 *  root for license information.                                *
 *****************************************************************/

#define S_FUNCTION_NAME  sfun_fmurun
#define S_FUNCTION_LEVEL 2

#include "FMI.h"
#include "FMI1.h"
#include "FMI2.h"

#include "simstruc.h"


#ifdef _WIN32
#include "shlwapi.h"
#include <wininet.h>
#pragma comment(lib, "shlwapi.lib")
#endif

//#include <stdio.h>
//#include <stdarg.h>
//#include <string>
//
//extern "C" {
//#include "simstruc.h"
//
//#ifdef GRTFMI
//extern const char *FMU_RESOURCES_DIR;
//#endif
//}
//

//#include "FMU1.h"
//#include "FMU2.h"
//
//using namespace std;
//using namespace fmikit;

#define MAX_MESSAGE_SIZE 4096

typedef enum {

	fmiVersionParam,
	runAsKindParam,
	guidParam,
	modelIdentifierParam,
	unzipDirectoryParam,
    debugLoggingParam,
    logFMICallsParam,
    logLevelParam,
    logFileParam,
	relativeToleranceParam,
	sampleTimeParam,
	offsetTimeParam,
	nxParam,
	nzParam,
	scalarStartTypesParam,
	scalarStartVRsParam,
	scalarStartValuesParam,
	stringStartVRsParam,
	stringStartValuesParam,
	inputPortWidthsParam,
	inputPortDirectFeedThroughParam,
	inputPortTypesParam,
	inputPortVariableVRsParam,
	outputPortWidthsParam,
	outputPortTypesParam,
	outputPortVariableVRsParam,
	numParams

} Parameter;

//typedef enum {
//	FMI_REAL,
//	FMI_INTEGER,
//	FMI_BOOLEAN,
//	FMI_STRING
//} Type;
//
//typedef unsigned int ValueReference;

//static string getStringParam(SimStruct *S, int index) {
//
//	const mxArray *pa = ssGetSFcnParam(S, index);
//
//	auto n = mxGetN(pa);
//	auto m = mxGetM(pa);
//
//	// TODO: assert m == 1
//
//	if (n < 1) return "";
//
//	auto data = static_cast<const mxChar*>(mxGetData(pa));
//
//	if (!data) return "";
//
//	auto cstr = static_cast<char *>(mxMalloc(n));
//
//	// convert real_T to ASCII char
//	for (int i = 0; i < n; i++) {
//		// TODO: assert 0 <= data[i] <= 127
//		cstr[i] = data[i];
//	}
//
//	string cppstr(cstr, n);
//	mxFree(cstr);
//	return cppstr;
//}

static const char* getStringParam(SimStruct *S, int index) {

	const mxArray *pa = ssGetSFcnParam(S, index);

	size_t n = mxGetN(pa);
	size_t m = mxGetM(pa);

	// TODO: assert m == 1

	if (n < 1) return "";

	const mxChar* data = (const mxChar*)mxGetData(pa);

	char *string = (char *)mxMalloc(n + 1);
	memset(string, '\0', n + 1);
	wcstombs(string, data, n);

	return string;
}

//static const wchar_t* fmiVersion(SimStruct *S) {
//	return getStringParam(S, fmiVersionParam);
//}

static bool isFMI1(SimStruct *S) {
	const mxArray *pa = ssGetSFcnParam(S, fmiVersionParam);
	const mxChar* data = (const mxChar*)mxGetData(pa);
	return mxGetN(pa) == 3 && wcsncmp(data, L"1.0", 3) == 0;
}

static bool isFMI2(SimStruct *S) {
	const mxArray *pa = ssGetSFcnParam(S, fmiVersionParam);
	const mxChar* data = (const mxChar*)mxGetData(pa);
	return mxGetN(pa) == 3 && wcsncmp(data, L"2.0", 3) == 0;
}

static bool isME(SimStruct *S) { 
	return mxGetScalar(ssGetSFcnParam(S, runAsKindParam)) == fmi2ModelExchange;
}

static bool isCS(SimStruct *S) {
	return mxGetScalar(ssGetSFcnParam(S, runAsKindParam)) == fmi2CoSimulation;
}

//static fmikit::Kind runAsKind(SimStruct *S) { return static_cast<fmikit::Kind>(static_cast<int>( mxGetScalar( ssGetSFcnParam(S, runAsKindParam) ) ) ); }
//
//static string guid(SimStruct *S) {
//	return getStringParam(S, guidParam);
//}
//
//static string modelIdentifier(SimStruct *S) {
//	return getStringParam(S, modelIdentifierParam);
//}
//
//static string unzipDirectory(SimStruct *S) {
//	return getStringParam(S, unzipDirectoryParam);
//}

static bool debugLogging(SimStruct *S) {
    return mxGetScalar(ssGetSFcnParam(S, debugLoggingParam));
}

static bool logFMICalls(SimStruct *S) {
    return mxGetScalar(ssGetSFcnParam(S, logFMICallsParam));
}

//static fmikit::LogLevel logLevel(SimStruct *S) {
//    int level = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, logLevelParam)));
//    return static_cast<fmikit::LogLevel>(level);
//}

//static const char *logFile(SimStruct *S) {
//    return getStringParam(S, logFileParam);
//}

static double relativeTolerance(SimStruct *S) {
    return mxGetScalar(ssGetSFcnParam(S, relativeToleranceParam));
}

static double sampleTime(SimStruct *S) {
    return mxGetScalar(ssGetSFcnParam(S, sampleTimeParam));
}

static double offsetTime(SimStruct *S) {
    return mxGetScalar(ssGetSFcnParam(S, offsetTimeParam));
}

// number of continuous states
static int nx(SimStruct *S) {
    return (int)mxGetScalar(ssGetSFcnParam(S, nxParam));
}

// number of zero-crossings
static int nz(SimStruct *S) {
    return (int)mxGetScalar(ssGetSFcnParam(S, nzParam));
}

static int nScalarStartValues(SimStruct *S) {
    return mxGetNumberOfElements(ssGetSFcnParam(S, scalarStartVRsParam));
}

static int inputPortWidth(SimStruct *S, int index) {
	real_T * portWidths = (real_T *)mxGetData(ssGetSFcnParam(S, inputPortWidthsParam));
	return portWidths[index];
}

static bool inputPortDirectFeedThrough(SimStruct *S, int index) {
	return ((real_T *)mxGetData(ssGetSFcnParam(S, inputPortDirectFeedThroughParam)))[index] != 0;
}

// number of input ports
inline size_t nu(SimStruct *S) { return mxGetNumberOfElements(ssGetSFcnParam(S, inputPortWidthsParam)); }

// number of input variables
inline size_t nuv(SimStruct *S) { return mxGetNumberOfElements(ssGetSFcnParam(S, inputPortVariableVRsParam)); }

inline FMIValueReference valueReference(SimStruct *S, Parameter parameter, int index) {
	const mxArray * param = ssGetSFcnParam(S, parameter);
	real_T realValue = ((real_T *)mxGetData(param))[index];
	return (FMIValueReference)realValue;
}

inline FMIVariableType variableType(SimStruct *S, Parameter parameter, int index) {
	const mxArray * param = ssGetSFcnParam(S, parameter);
	real_T realValue = ((real_T *)mxGetData(param))[index];
	int intValue = (int)realValue;
	return (FMIVariableType)intValue;
}

inline DTypeId simulinkVariableType(SimStruct *S, Parameter parameter, int index) {

	const mxArray *param = ssGetSFcnParam(S, parameter);
	real_T realValue = ((real_T *)mxGetData(param))[index];
	int intValue = (int)realValue;
	FMIVariableType type = (FMIVariableType)intValue;

	switch (type) {
	case FMIRealType:    return SS_DOUBLE;
	case FMIIntegerType: return SS_INT32;
	case FMIBooleanType: return SS_BOOLEAN;
	default:      return -1; // error
	}
}

inline real_T scalarValue(SimStruct *S, Parameter parameter, int index) {
	const mxArray *param = ssGetSFcnParam(S, parameter);
	return ((real_T *)mxGetData(param))[index];
}

static int outputPortWidth(SimStruct *S, int index) {
	real_T *portWidths = (real_T *)mxGetData(ssGetSFcnParam(S, outputPortWidthsParam));
	return (int)portWidths[index];
}

inline size_t ny(SimStruct *S) { return mxGetNumberOfElements(ssGetSFcnParam(S, outputPortWidthsParam)); }

//template<typename T> T *component(SimStruct *S) {
//	auto fmu = static_cast<FMU *>(ssGetPWork(S)[0]);
//	return dynamic_cast<T *>(fmu);
//}

static void logCall(SimStruct *S, const char* message) {

    FILE *logfile = NULL;

	void **p = ssGetPWork(S);

    if (p) {
        logfile = (FILE *)p[1];
    }

    if (logfile) {
        fputs(message, logfile);
        fputs("\n", logfile);
        fflush(logfile);
    } else {
        ssPrintf(message);
        ssPrintf("\n");
    }
}

static void cb_logMessage(fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message) {
	ssPrintf(message);
	ssPrintf("\n");
}

//static void logFMUMessage(FMU *instance, LogLevel level, const char* category, const char* message) {
//
//    if (instance && instance->m_userData) {
//        SimStruct *S = static_cast<SimStruct *>(instance->m_userData);
//        logCall(S, message);
//    }
//}
//
//static void logFMICall(FMU *instance, const char* message) {
//
//	if (instance && instance->m_userData) {
//		SimStruct *S = static_cast<SimStruct *>(instance->m_userData);
//		logCall(S, message);
//	}
//}

//static void cb_logMessage(fmi2ComponentEnvironment componentEnvironment, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...) {
//	ssPrintf(message);
//	ssPrintf("\n");
//}

static void cb_logFunctionCall(fmi2Status status, const char *instanceName, const char *message, ...) {
	char s[MAX_MESSAGE_SIZE];
	va_list args;
	size_t len = snprintf(s, MAX_MESSAGE_SIZE, "[%s] ", instanceName);
	va_start(args, message);
	len += vsnprintf(&s[len], MAX_MESSAGE_SIZE - len, message, args);
	va_end(args);
	len += snprintf(&s[len], MAX_MESSAGE_SIZE - len, " -> %d\n", status);
	ssPrintf(s);
	ssPrintf("\n");
}


#define CHECK_STATUS(s) if (s > fmi2Warning) { ssSetErrorStatus(S, "The FMU encountered an error."); return; }


/* log mdl*() and fmi*() calls */
static void logDebug(SimStruct *S, const char* message, ...) {

    if (logFMICalls(S)) {
		
		char buf[MAX_MESSAGE_SIZE];
		
		size_t len = snprintf(buf, MAX_MESSAGE_SIZE, "[%s] ", ssGetPath(S));
		
		va_list args;
		va_start(args, message);
        vsnprintf(&buf[len], MAX_MESSAGE_SIZE - len, message, args);
        va_end(args);

        logCall(S, buf);
    }
}

static void setErrorStatus(SimStruct *S, const char *message, ...) {
	va_list args;
	va_start(args, message);
	static char msg[1024];
	vsnprintf(msg, 1024, message, args);
	ssSetErrorStatus(S, msg);
	va_end(args);
}

static void setInput(SimStruct *S, bool direct) {

	void **p = ssGetPWork(S);

	FMI2Instance *instance = (FMI2Instance *)p[0];

	int iu = 0;

	for (int i = 0; i < nu(S); i++) {

		const int w = inputPortWidth(S, i);

		if (direct && !inputPortDirectFeedThrough(S, i)) {
			iu += w;
			continue;
		}

		FMIVariableType type = variableType(S, inputPortTypesParam, i);

		const void *y = ssGetInputPortSignal(S, i);

		for (int j = 0; j < w; j++) {

			const FMIValueReference vr = valueReference(S, inputPortVariableVRsParam, iu);

			// set the input
			if (isFMI1(S)) {

				switch (type) {
				case FMIRealType:
					CHECK_STATUS(FMI1SetReal(instance, &vr, 1, &((const real_T *)y)[j]))
						break;
				case FMIIntegerType:
					CHECK_STATUS(FMI1SetInteger(instance, &vr, 1, &((const int32_T *)y)[j]))
						break;
				case FMIBooleanType:
					CHECK_STATUS(FMI1SetBoolean(instance, &vr, 1, &((const boolean_T *)y)[j]))
					break;
				default:
					break;
				}

			} else {
				
				switch (type) {
				case FMIRealType:
					CHECK_STATUS(FMI2SetReal(instance, &vr, 1, &((const real_T *)y)[j]))
					break;
				case FMIIntegerType:
					CHECK_STATUS(FMI2SetInteger(instance, &vr, 1, &((const int32_T *)y)[j]))
					break;
				case FMIBooleanType: {
					const fmi2Boolean booleanValue = ((const boolean_T *)y)[j];
					CHECK_STATUS(FMI2SetBoolean(instance, &vr, 1, &booleanValue))
					break;
				}
				default:
					break;
				}
			}



			iu++;
		}
	}
}

static void setOutput(SimStruct *S) {

	void **p = ssGetPWork(S);

	FMI2Instance *instance = (FMI2Instance *)p[0];

	int iy = 0;

	for (int i = 0; i < ny(S); i++) {

		FMIVariableType type = variableType(S, outputPortTypesParam, i);

		void *y = ssGetOutputPortSignal(S, i);

		for (int j = 0; j < outputPortWidth(S, i); j++) {

			auto vr = valueReference(S, outputPortVariableVRsParam, iy);

			if (isFMI1(S)) {

				switch (type) {
				case FMIRealType:
					CHECK_STATUS(FMI1GetReal(instance, &vr, 1, &((real_T *)y)[j]))
						break;
				case FMIIntegerType:
					CHECK_STATUS(FMI1GetInteger(instance, &vr, 1, &((int32_T *)y)[j]))
						break;
				case FMIBooleanType:
					CHECK_STATUS(FMI1GetBoolean(instance, &vr, 1, &((boolean_T *)y)[j]))
						break;
				default:
					break;
				}

			} else {

				switch (type) {
				case FMIRealType:
					CHECK_STATUS(FMI2GetReal(instance, &vr, 1, &((real_T *)y)[j]))
						break;
				case FMIIntegerType:
					CHECK_STATUS(FMI2GetInteger(instance, &vr, 1, &((int32_T *)y)[j]))
						break;
				case FMIBooleanType: {
					fmi2Boolean booleanValue;
					CHECK_STATUS(FMI2GetBoolean(instance, &vr, 1, &booleanValue))
					((boolean_T *)y)[j] = booleanValue;
					break;
				}
				default:
					break;
				}

			}

			iy++;
		}
	}
}

//static void getLibraryPath(SimStruct *S, char *path) {
//
//#ifdef GRTFMI
//	auto unzipdir = FMU_RESOURCES_DIR + string("/") + modelIdentifier(S);
//#else
//	auto unzipdir = unzipDirectory(S);
//#endif
//
//#ifdef _WIN32
//	strcpy(path, unzipdir.c_str());
//	PathAppend(path, "binaries");
//
//#ifdef _WIN64
//	PathAppend(path, "win64");
//#else
//	PathAppend(path, "win32");
//#endif
//
//	PathAppend(path, modelIdentifier(S).c_str());
//	PathAddExtension(path, ".dll");
//#else
//	// TODO
//#endif
//}

static void setStartValues(SimStruct *S) {

	void **p = ssGetPWork(S);

	FMI2Instance *instance = (FMI2Instance *)p[0];

    // scalar start values
	for (int i = 0; i < nScalarStartValues(S); i++) {
		FMIValueReference vr = valueReference(S, scalarStartVRsParam, i);
		FMIVariableType type = variableType(S, scalarStartTypesParam, i);
		real_T realValue = scalarValue(S, scalarStartValuesParam, i);
				
		if (isFMI1(S)) {

			fmi1Integer intValue = realValue;
			fmi1Boolean boolValue = realValue;

			switch (type) {
			case FMIRealType:    FMI1SetReal    (instance, &vr, 1, &realValue); break;
			case FMIIntegerType: FMI1SetInteger (instance, &vr, 1, &intValue);  break;
			case FMIBooleanType: FMI1SetBoolean (instance, &vr, 1, &boolValue); break;
			default: break;
			}

		} else {

			fmi2Integer intValue = realValue;
			fmi2Boolean boolValue = realValue;

			switch (type) {
			case FMIRealType:    FMI2SetReal    (instance, &vr, 1, &realValue); break;
			case FMIIntegerType: FMI2SetInteger (instance, &vr, 1, &intValue);  break;
			case FMIBooleanType: FMI2SetBoolean (instance, &vr, 1, &boolValue); break;
			default: break;
			}
		}
    }

	// string start values
	const mxArray * pa     = ssGetSFcnParam(S, stringStartValuesParam);
	size_t size   = mxGetNumberOfElements(pa) + 1;
	size_t m      = mxGetM(pa);
	size_t n      = mxGetN(pa);
	char *buffer = (char *)calloc(size, sizeof(char));
	char *value  = (char *)calloc(n + 1, sizeof(char));

	//if (mxGetString(pa, buffer, size) != 0) {
	//	ssSetErrorStatus(S, "Failed to convert string parameters");
	//	return;
	//}

	for (int i = 0; i < m; i++) {

		// copy the row
		for (int j = 0; j < n; j++) value[j] = buffer[j * m + i];

		// remove the trailing blanks
		for (int j = n - 1; j >= 0; j--) {
			if (value[j] != ' ') break;
			value[j] = '\0';
		}

		FMIValueReference vr = valueReference(S, stringStartVRsParam, i);

		if (isFMI1(S)) {
			FMI2SetString(instance, &vr, 1, &value);
		} else {
			FMI1SetString(instance, &vr, 1, &value);
		}
	}

	free(buffer);
	free(value);
}

static void update(SimStruct *S) {

	if (isCS(S)) {
		return;  // nothing to do
	}

	FMI2Instance *instance = (FMI2Instance *)ssGetPWork(S)[0];

	double time = instance->time;
	bool upcomingTimeEvent;
	double nextEventTime;

	if (isFMI1(S)) {
		upcomingTimeEvent = instance->eventInfo1.upcomingTimeEvent;
		nextEventTime = instance->eventInfo1.nextEventTime;
	} else {
		upcomingTimeEvent = instance->eventInfo.nextEventTimeDefined;
		nextEventTime = instance->eventInfo.nextEventTime;
	}

	// Work around for the event handling in Dymola FMUs:
	bool timeEvent = upcomingTimeEvent && time >= nextEventTime;

	if (timeEvent/* && logLevel(S) <= DEBUG*/) {
		logDebug(S, "Time event at t=%.16g", time);
		//ssPrintf("Time event at t=%.16g\n", time);
	}

	bool stepEvent;

	if (isFMI1(S)) {
		fmi1Boolean callEventUpdate = fmi1False;
		CHECK_STATUS(FMI1CompletedIntegratorStep(instance, &callEventUpdate))
		stepEvent = callEventUpdate;
	} else {
		fmi2Boolean enterEventMode = fmi2False;
		fmi2Boolean terminateSimulation = fmi2False;
		CHECK_STATUS(FMI2CompletedIntegratorStep(instance, fmi2True, &enterEventMode, &terminateSimulation))
		if (terminateSimulation) {
			setErrorStatus(S, "The FMU requested to terminate the simulation.");
			return;
		}
		stepEvent = enterEventMode;
	}

	if (stepEvent) {
		logDebug(S, "Step event at t=%.16g\n", time);
	}

	bool stateEvent = false;

	if (nz(S) > 0) {

		real_T *prez = ssGetRWork(S);
		real_T *z = prez + nz(S);

		if (isFMI1(S)) {
			CHECK_STATUS(FMI1GetEventIndicators(instance, z, nz(S)))
		} else {
			CHECK_STATUS(FMI2GetEventIndicators(instance, z, nz(S)))
		}

		// check for state events
		for (int i = 0; i < nz(S); i++) {

			bool rising  = (prez[i] < 0 && z[i] >= 0) || (prez[i] == 0 && z[i] > 0);
			bool falling = (prez[i] > 0 && z[i] <= 0) || (prez[i] == 0 && z[i] < 0);

			if (rising || falling) {
				logDebug(S, "State event %s z[%d] at t=%.16g\n", rising ? "-\\+" : "+/-", i, instance->time);
				stateEvent = true;
				// TODO: break?
			}
		}

		// remember the current event indicators
		for (int i = 0; i < nz(S); i++) prez[i] = z[i];
	}

	if (timeEvent || stepEvent || stateEvent) {

		if (isFMI1(S)) {

			CHECK_STATUS(FMI1EventUpdate(instance, fmi1False, &instance->eventInfo1));
		
		} else {

			CHECK_STATUS(FMI2EnterEventMode(instance))

			do {
				CHECK_STATUS(FMI2NewDiscreteStates(instance, &instance->eventInfo))
				if (instance->eventInfo.terminateSimulation) {
					setErrorStatus(S, "The FMU requested to terminate the simulation.");
					return;
				}
			} while (instance->eventInfo.newDiscreteStatesNeeded);

			CHECK_STATUS(FMI2EnterContinuousTimeMode(instance))
		}

		if (nx(S) > 0) {

			real_T *x = ssGetContStates(S);

			if (isFMI1(S)) {
				CHECK_STATUS(FMI1GetContinuousStates(instance, x, nx(S)))
			} else {
				CHECK_STATUS(FMI2GetContinuousStates(instance, x, nx(S)))
			}
		}

		if (nz(S) > 0) {

			real_T *prez = ssGetRWork(S);

			if (isFMI1(S)) {
				CHECK_STATUS(FMI1GetEventIndicators(instance, prez, nz(S)))
			} else {
				CHECK_STATUS(FMI2GetEventIndicators(instance, prez, nz(S)))
			}
		}

		ssSetSolverNeedsReset(S);
	}
}

#define MDL_CHECK_PARAMETERS
#if defined(MDL_CHECK_PARAMETERS) && defined(MATLAB_MEX_FILE)
static void mdlCheckParameters(SimStruct *S) {

	logDebug(S, "mdlCheckParameters()");

	if (!mxIsChar(ssGetSFcnParam(S, fmiVersionParam)) || (!isFMI1(S) && !isFMI2(S))) {
        setErrorStatus(S, "Parameter %d (FMI version) must be one of '1.0' or '2.0'", fmiVersionParam + 1);
        return;
    }

	if (!mxIsNumeric(ssGetSFcnParam(S, runAsKindParam)) || mxGetNumberOfElements(ssGetSFcnParam(S, runAsKindParam)) != 1 || (!isME(S) && !isCS(S))) {
        setErrorStatus(S, "Parameter %d (run as kind) must be one of 0 (= MODEL_EXCHANGE) or 1 (= CO_SIMULATION)", runAsKindParam + 1);
        return;
    }

	if (!mxIsChar(ssGetSFcnParam(S, guidParam))) {
        setErrorStatus(S, "Parameter %d (GUID) must be a string", guidParam + 1);
        return;
    }

	if (!mxIsChar(ssGetSFcnParam(S, modelIdentifierParam))) {
        setErrorStatus(S, "Parameter %d (model identifier) must be a string", modelIdentifierParam + 1);
        return;
    }

	if (!mxIsChar(ssGetSFcnParam(S, unzipDirectoryParam))) {
		setErrorStatus(S, "Parameter %d (unzip directory) must be a string", unzipDirectoryParam + 1);
		return;
	}

    if (!mxIsNumeric(ssGetSFcnParam(S, debugLoggingParam)) || mxGetNumberOfElements(ssGetSFcnParam(S, debugLoggingParam)) != 1) {
        setErrorStatus(S, "Parameter %d (debug logging) must be a scalar", debugLoggingParam + 1);
        return;
    }

	// TODO: check VRS values!

}
#endif /* MDL_CHECK_PARAMETERS */


static void mdlInitializeSizes(SimStruct *S) {

	logDebug(S, "mdlInitializeSizes()");

	ssSetNumSFcnParams(S, numParams);

#if defined(MATLAB_MEX_FILE)
	if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S)) {
		mdlCheckParameters(S);
		if (ssGetErrorStatus(S) != NULL) {
			return;
		}
	} else {
		return; // parameter mismatch will be reported by Simulink
	}
#endif

	ssSetNumContStates(S, (isME(S)) ? nx(S) : 0);
	ssSetNumDiscStates(S, 0);

	if (!ssSetNumInputPorts(S, nu(S))) return;

	for (int i = 0; i < nu(S); i++) {
		ssSetInputPortWidth(S, i, inputPortWidth(S, i));
		ssSetInputPortRequiredContiguous(S, i, 1); // direct input signal access
		DTypeId type = simulinkVariableType(S, inputPortTypesParam, i);
		ssSetInputPortDataType(S, i, type);
		bool dirFeed = inputPortDirectFeedThrough(S, i);
		ssSetInputPortDirectFeedThrough(S, i, dirFeed); // direct feed through
		logDebug(S, "ssSetInputPortDirectFeedThrough(path=\"%s\", port=%d, dirFeed=%d)", ssGetPath(S), i, dirFeed);
	}

	if (!ssSetNumOutputPorts(S, ny(S))) return;

	for (int i = 0; i < ny(S); i++) {
		ssSetOutputPortWidth(S, i, outputPortWidth(S, i));
		DTypeId type = simulinkVariableType(S, outputPortTypesParam, i);
		ssSetOutputPortDataType(S, i, type);
	}

	ssSetNumSampleTimes(S, 1);
	ssSetNumRWork(S, 2 * nz(S) + nuv(S)); // prez & z, preu
	ssSetNumIWork(S, 0);
	ssSetNumPWork(S, 2); // [FMU, logfile]
	ssSetNumModes(S, 3); // [stateEvent, timeEvent, stepEvent]
	ssSetNumNonsampledZCs(S, (isME(S)) ? nz(S) + 1 : 0);

	// specify the sim state compliance to be same as a built-in block
	//ssSetSimStateCompliance(S, USE_DEFAULT_SIM_STATE);

	ssSetOptions(S, 0);
}


static void mdlInitializeSampleTimes(SimStruct *S) {

	logDebug(S, "mdlInitializeSampleTimes()");

	if (isCS(S)) {
		ssSetSampleTime(S, 0, sampleTime(S));
		ssSetOffsetTime(S, 0, offsetTime(S));
	} else {
		ssSetSampleTime(S, 0, CONTINUOUS_SAMPLE_TIME);
		ssSetOffsetTime(S, 0, offsetTime(S));
	}

}


#define MDL_START
#if defined(MDL_START)
static void mdlStart(SimStruct *S) {

    void **p = ssGetPWork(S);

    if (p[1]) {
        fclose((FILE *)p[1]);
        p[1] = NULL;
    }

    //auto logfile = logFile(S);

    //if (!logfile.empty()) {
    //    p[1] = fopen(logfile.c_str(), "w");
    //}

	logDebug(S, "mdlStart()");

	const char_T* instanceName = ssGetPath(S);
	time_T time = ssGetT(S);

	//FMU::m_messageLogger = logFMUMessage;

	//char libraryFile[1000];
	//getLibraryPath(S, libraryFile);
//
//#ifdef _WIN32
//	if (!PathFileExists(libraryFile)) {
//		static char errorMessage[1024];
//		snprintf(errorMessage, 1024, "Cannot find the FMU's platform binary %s for %s.", libraryFile, instanceName);
//		ssSetErrorStatus(S, errorMessage);
//		return;
//	}
//#endif

	bool toleranceDefined = relativeTolerance(S) > 0;

    bool loggingOn = debugLogging(S);

	const char *modelIdentifier = getStringParam(S, modelIdentifierParam);
	const char *fmuResourceLocation = "";

#ifdef GRTFMI
	auto unzipdir = FMU_RESOURCES_DIR + string("/") + modelIdentifier(S);
#else
	const char *unzipdir = getStringParam(S, unzipDirectoryParam);
#endif

#ifdef _WIN32
	char libraryPath[MAX_PATH];
	WCHAR dllDirectory[MAX_PATH];

	strncpy(libraryPath, unzipdir, MAX_PATH);

	PathAppend(libraryPath, "binaries");
#ifdef _WIN64
	PathAppend(libraryPath, "win64");
#else
	PathAppend(libraryPath, "win32");
#endif

	PathAppend(libraryPath, modelIdentifier);
	strncat(libraryPath, ".dll", MAX_PATH);
#endif

	FMI2Instance *instance = FMICreateInstance(instanceName, libraryPath, cb_logMessage, logFMICalls(S) ? cb_logFunctionCall : NULL);

	p[0] = instance;

	const char *guid = getStringParam(S, guidParam);

	time_T stopTime = ssGetTFinal(S);  // can be -1

	if (isFMI1(S)) {

		if (isCS(S)) {
			CHECK_STATUS(FMI1InstantiateSlave(instance, modelIdentifier, guid, fmuResourceLocation, "application/x-fmu-sharedlibrary", 0, fmi1False, fmi1False, loggingOn))
			setStartValues(S);
			CHECK_STATUS(FMI1InitializeSlave(instance, time, stopTime > time, stopTime))
		} else {
			CHECK_STATUS(FMI1InstantiateModel(instance, modelIdentifier, guid, loggingOn))
			setStartValues(S);
			CHECK_STATUS(FMI1SetTime(instance, time))
			CHECK_STATUS(FMI1Initialize(instance, toleranceDefined, relativeTolerance(S)))
			if (instance->eventInfo1.terminateSimulation) {
				setErrorStatus(S, "Model requested termination at init");
				return;
			}
		}

	} else {

		CHECK_STATUS(FMI2Instantiate(instance, fmuResourceLocation, isCS(S) ? fmi2CoSimulation : fmi2ModelExchange, guid, fmi2False, loggingOn))

		if (!instance) {
			ssSetErrorStatus(S, "Failed to instantiate FMU.");
			return;
		}

		setStartValues(S);

		if (ssGetErrorStatus(S)) return;

		CHECK_STATUS(FMI2SetupExperiment(instance, toleranceDefined, relativeTolerance(S), time, stopTime > time, stopTime))

		CHECK_STATUS(FMI2EnterInitializationMode(instance))
		CHECK_STATUS(FMI2ExitInitializationMode(instance))
	}

	mxFree((void *)modelIdentifier);
	mxFree((void *)guid);
}
#endif /* MDL_START */


#define MDL_INITIALIZE_CONDITIONS
#if defined(MDL_INITIALIZE_CONDITIONS)
static void mdlInitializeConditions(SimStruct *S) {

	logDebug(S, "mdlInitializeConditions()");

	if (isCS(S)) {
		return;  // nothing to do
	}


	void **p = ssGetPWork(S);

	FMI2Instance *instance = (FMI2Instance *)p[0];

	// initialize the continuous states
	real_T *x = ssGetContStates(S);

	if (isFMI1(S)) {
		CHECK_STATUS(FMI1GetContinuousStates(instance, x, nx(S)))
	} else {
		CHECK_STATUS(FMI2GetContinuousStates(instance, x, nx(S)))
	}

	// initialize the event indicators
	if (nz(S) > 0) {

		real_T *prez = ssGetRWork(S);
		real_T *z = prez + nz(S);

		if (isFMI1(S)) {
			CHECK_STATUS(FMI1GetEventIndicators(instance, prez, nz(S)))
			CHECK_STATUS(FMI1GetEventIndicators(instance, z, nz(S)))
		}
		else {
			CHECK_STATUS(FMI2GetEventIndicators(instance, prez, nz(S)))
			CHECK_STATUS(FMI2GetEventIndicators(instance, z, nz(S)))
		}

	}
}
#endif


static void mdlOutputs(SimStruct *S, int_T tid) {

	logDebug(S, "mdlOutputs(tid=%d, time=%.16g, majorTimeStep=%d)", tid, ssGetT(S), ssIsMajorTimeStep(S));

	void **p = ssGetPWork(S);

	FMI2Instance *instance = (FMI2Instance *)p[0];

	if (isME(S)) {

		real_T *x = ssGetContStates(S);

		if (isFMI2(S)) {

			if (instance->state == FMI2EventModeState) {


				setInput(S, true);

				if (ssGetErrorStatus(S)) return;

				do {
					CHECK_STATUS(FMI2NewDiscreteStates(instance, &instance->eventInfo))
					if (instance->eventInfo.terminateSimulation) {
						setErrorStatus(S, "The FMU requested to terminate the simulation.");
						return;
					}
				} while (instance->eventInfo.newDiscreteStatesNeeded);

				CHECK_STATUS(FMI2EnterContinuousTimeMode(instance))
			}

			if (instance->state != FMI2ContinuousTimeModeState) {
				CHECK_STATUS(FMI2EnterContinuousTimeMode(instance))
			}

			CHECK_STATUS(FMI2SetTime(instance, ssGetT(S)))
			CHECK_STATUS(FMI2SetContinuousStates(instance, x, nx(S)))

		} else {
			
			CHECK_STATUS(FMI1SetTime(instance, ssGetT(S)))
			CHECK_STATUS(FMI1SetContinuousStates(instance, x, nx(S)))
		
		}
			   		
		setInput(S, true);

		if (ssGetErrorStatus(S)) return;

		if (ssIsMajorTimeStep(S)) {
			update(S);
			if (ssGetErrorStatus(S)) return;
		}

	} else {

		time_T h = ssGetT(S) - instance->time;

		if (h > 0) {
			if (isFMI1(S)) {
				CHECK_STATUS(FMI1DoStep(instance, ssGetT(S), h, fmi1True))
			} else {
				CHECK_STATUS(FMI2DoStep(instance, ssGetT(S), h, fmi2False))
			}
		}
	}

	setOutput(S);
}

#define MDL_UPDATE
#if defined(MDL_UPDATE)
static void mdlUpdate(SimStruct *S, int_T tid) {

	logDebug(S, "mdlUpdate(tid=%d, time=%.16g, majorTimeStep=%d)", tid, ssGetT(S), ssIsMajorTimeStep(S));

	setInput(S, false);
}
#endif // MDL_UPDATE


#define MDL_ZERO_CROSSINGS
#if defined(MDL_ZERO_CROSSINGS) && (defined(MATLAB_MEX_FILE) || defined(NRT))
static void mdlZeroCrossings(SimStruct *S) {

	logDebug(S, "mdlZeroCrossings(time=%.16g, majorTimeStep=%d)", ssGetT(S), ssIsMajorTimeStep(S));

	if (isME(S)) {

		setInput(S, true);

		if (ssGetErrorStatus(S)) return;

		real_T *z = ssGetNonsampledZCs(S);

		void **p = ssGetPWork(S);

		FMI2Instance *instance = (FMI2Instance *)p[0];

		if (nz(S) > 0) {
			if (isFMI1(S)) {
				CHECK_STATUS(FMI1GetEventIndicators(instance, z, nz(S)))
			} else {
				CHECK_STATUS(FMI2GetEventIndicators(instance, z, nz(S)))
			}
		}

		real_T nextEventTime = instance->eventInfo1.nextEventTime ? isFMI1(S) : instance->eventInfo.nextEventTime;

		z[nz(S)] = nextEventTime - ssGetT(S);
	}
}
#endif


#define MDL_DERIVATIVES
#if defined(MDL_DERIVATIVES)
static void mdlDerivatives(SimStruct *S) {

	logDebug(S, "mdlDerivatives(time=%.16g, majorTimeStep=%d)", ssGetT(S), ssIsMajorTimeStep(S));

	if (isCS(S)) {
		return;  // nothing to do
	}
		
	void **p = ssGetPWork(S);

	FMI2Instance *instance = (FMI2Instance *)p[0];

	setInput(S, true);

	if (ssGetErrorStatus(S)) return;

	real_T *x = ssGetContStates(S);
	real_T *dx = ssGetdX(S);

	if (isFMI1(S)) {
		FMI1GetContinuousStates(instance, x, nx(S));
		FMI1GetDerivatives(instance, dx, nx(S));
	} else {
		FMI2GetContinuousStates(instance, x, nx(S));
		FMI2GetDerivatives(instance, dx, nx(S));
	}
}
#endif


static void mdlTerminate(SimStruct *S) {

	logDebug(S, "mdlTerminate()");

	FMI2Instance *instance = (FMI2Instance *)ssGetPWork(S)[0];

	if (isFMI1(S)) {

		if (isME(S)) {
			CHECK_STATUS(FMI1Terminate(instance))
			FMI1FreeModelInstance(instance);
		} else {
			CHECK_STATUS(FMI1TerminateSlave(instance))
			FMI1FreeSlaveInstance(instance);
		}

	} else {
		
		CHECK_STATUS(FMI2Terminate(instance))
		
		FMI2FreeInstance(instance);
	}

	FMIFreeInstance(instance);
}

/*=============================*
* Required S-function trailer *
*=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
