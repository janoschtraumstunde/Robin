#pragma once

#include <WProgram.h>

#define ADCC_RATE 0b10010100 // conversion command for angular rate
#define ADCC_TEMP 0b10011100 // conversion command for temperature
#define ADCR 0b10000000 // result reading command

class Gyroscope {
	bool isCalibrating;
	bool enabled;
	int currentAngle;
	int pinSS;
	int pinSCK;
	int pinMOSI;
	int pinMISO;
	int calibrationCount;
	int calibrationAdcSum;
	int calibrationIndex;
	int calibrationAdcMin;
	int calibrationAdcMax;
	int calibrationAdcAvg;
	double calibrationRateAvg;

public:
	Gyroscope(int slaveSelect, int sck, int mosi, int miso);

	void enable();
	void update(unsigned long deltaInMilliseconds);
	int getCurrentAngle();
	void resetAngle(int angle);
	void calibrate(int count);

private:
	bool tryReadAdc(unsigned int &result);
	double adcToAngularRate(unsigned int adcValue);
	byte spiTransfer(byte tx);
};
