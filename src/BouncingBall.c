#include <stdio.h>
#include <stdarg.h>

//// no runtime resources
//#define RESOURCE_LOCATION ""

#include "FMI2.h"

// callback functions
static void cb_logMessage(fmi2ComponentEnvironment componentEnvironment, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...) {
	printf("%s\n", message);
}

static void cb_logFunctionCall(fmi2Status status, const char *instanceName, const char *message, ...) {
	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);
	printf(" -> %d\n", status);
}

#define CHECK_STATUS(S) { status = S; if (status != fmi2OK) goto TERMINATE; }

// #define ASSERT_NO_ERROR(F, M) __try { assertNoError(F, M); } __except (EXCEPTION_EXECUTE_HANDLER) { error("%s. The FMU crashed (exception code: %s).", M, exceptionCodeToString(GetExceptionCode())); }


int main(int argc, char *argv[]) {

	fmi2Status status = fmi2OK;

	FMI2Instance *instance = FMI2Instantiate("E:\\Development\\FMIKit-Simulink\\examples\\BouncingBall", 
		"BouncingBall", "bouncingBall", fmi2CoSimulation, "{8c4e810f-3df3-4a00-8276-176fa3c9f003}", fmi2False, fmi2False);

	instance->logFunctionCall = cb_logFunctionCall;

	if (!instance) return 1;

	fmi2Real time = 0;
	fmi2Real stepSize = 0.01;

	__try {
		//volatile int *pInt = 0x00000000;
		//*pInt = 20;
		CHECK_STATUS(FMI2SetupExperiment(instance, fmi2False, 0, time, fmi2False, 0))
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		const char *code = exceptionCodeToString(GetExceptionCode());
		printf("The FMU instance %s caused a %s (exception code 0x%lX).\n", instance->name, code, GetExceptionCode());
		status = fmi2Fatal;
		goto TERMINATE;
	}

	CHECK_STATUS(FMI2EnterInitializationMode(instance))

	CHECK_STATUS(FMI2ExitInitializationMode(instance))
	
	printf("time, h\n");

	for (int nSteps = 0; nSteps <= 100; nSteps++) {

		time = nSteps * stepSize;

		fmi2ValueReference vr_h = 0;
		fmi2Real h;

		// get an output
		CHECK_STATUS(FMI2GetReal(instance, &vr_h, 1, &h));

		// perform a simulation step
		CHECK_STATUS(FMI2DoStep(instance, time, stepSize, fmi2True));

		printf("%.2f, %.2f\n", time, h);
	}
//
TERMINATE:

	// clean up
	if (status < fmi2Fatal) {
		FMI2FreeInstance(instance);
	}

	return status;
}