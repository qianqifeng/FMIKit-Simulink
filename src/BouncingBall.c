#include <stdio.h>

//// no runtime resources
//#define RESOURCE_LOCATION ""

#include "FMI2.h"

// callback functions
static void cb_logMessage(fmi2ComponentEnvironment componentEnvironment, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...) {
	printf("%s\n", message);
}

//static void* cb_allocateMemory(size_t nobj, size_t size) {
//	return calloc(nobj, size);
//}
//
//static void cb_freeMemory(void* obj) {
//	free(obj);
//}

#define CHECK_STATUS(S) { status = S; if (status != fmi2OK) goto TERMINATE; }

int main(int argc, char *argv[]) {

	fmi2Status status = fmi2OK;

	FMI2Instance *instance = FMI2Instantiate("E:\\Development\\FMIKit-Simulink\\examples\\BouncingBall", "bouncingBall", fmi2CoSimulation, "{8c4e810f-3df3-4a00-8276-176fa3c9f003}", NULL, fmi2False, fmi2False);

	if (!instance) return 1;

	fmi2Real time = 0;
	fmi2Real stepSize = 0.01;

	CHECK_STATUS(FMI2SetupExperiment(instance, fmi2False, 0, time, fmi2False, 0))

	CHECK_STATUS(FMI2EnterInitializationMode(instance))

	CHECK_STATUS(FMI2ExitInitializationMode(instance))
	
	printf("time, h\n");

	for (int nSteps = 0; nSteps <= 100; nSteps++) {

		time = nSteps * stepSize;

		fmi2ValueReference vr_h = 0;
		fmi2Real h;
//
		// get an output
		CHECK_STATUS(FMI2GetReal(instance, &vr_h, 1, &h));

		// perform a simulation step
		CHECK_STATUS(FMI2DoStep(instance, time, stepSize, fmi2True));

		printf("%.2f, %.2f\n", time, h);
	}
//
TERMINATE:
//
//	// clean up
//	if (status < fmi2Fatal) {
//		Heater_fmi2FreeInstance(c);
//	}

	return status;
}