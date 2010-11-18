#include <WProgram.h>
#include "CPid.h"

Pid::Pid(double p, double i, double d) {
	outputMin = -255;
	outputMax = 255;

	kp = p;
	ki = i;
	kd = d;
}

void Pid::setInput(int newInput) {
	input = newInput;
}

void Pid::setSetpoint(int newSetpoint) {
	setpoint = newSetpoint;
}

void Pid::setOutputLimits(int min, int max) {
	outputMin = min;
	outputMax = max;
}

void Pid::compute(double dt) {
	int error = setpoint - input;
	integral = integral + (error * dt);
	double derivative = (error - errorPrevious) / dt;
	output += (kp * error) + (ki * integral) + (kd * derivative);
	errorPrevious = error;

	output = constrain(output, outputMin, outputMax);
}