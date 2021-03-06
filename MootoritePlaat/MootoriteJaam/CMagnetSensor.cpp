#include <WProgram.h>
#include "CMagnetSensor.h"
#include "ArduinoPins.h"

MagnetSensor::MagnetSensor(int slaveSelect, int sck, int miso) {
	pinMode(slaveSelect, OUTPUT);
	digitalWrite(slaveSelect, HIGH); // HIGH - disable device

	sensor = MLX90316();
	sensor.attach(slaveSelect, sck, miso);
}

void MagnetSensor::update() {
	currentAngle = sensor.readAngle();

	calculateNewPosition(currentAngle);
}

void MagnetSensor::calculateNewPosition(int angle) {
	if (angle < 0) // angle should be between 0..3600, otherwise an error occurred
		return;

	int delta = angle - anglePrevious;

	if (abs(delta) < 10) // only count changes larger than 1 degree
		return;

	anglePrevious = angle;

	if (delta <= -1800) {
		knownAngle += 3600;
	}
	else if (delta >= 1800) {
		knownAngle -= 3600;
	}

	positionTotal = -(knownAngle + angle - angleInitial);
}

void MagnetSensor::reset() {
	angleInitial = sensor.readAngle();
	anglePrevious = angleInitial;
	knownAngle = 0;
	positionTotal = 0;
}

long MagnetSensor::getPositionTotal() {
	return positionTotal;
}

long MagnetSensor::getCurrentDelta() {
	return getPositionTotal() - positionPrevious;
}

void MagnetSensor::resetCurrentDelta() {
	positionPrevious = getPositionTotal();
}

