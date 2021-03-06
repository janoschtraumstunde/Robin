#include <WProgram.h>
#include "CMagnetSensor.h"
#include "CWheels.h"
#include "CPid.h"
#include "SinLookupTable.h"

void Wheels::moveDistance(int localDirection, int distance) {
	moveAndTurnDistance(localDirection, distance, 0);
}

void Wheels::turnDistance(int localRotation) {
	moveAndTurnDistance(0, 0, localRotation);
}

// decidegrees, millimeters, decidegrees
void Wheels::moveAndTurnDistance(int localDirectionInDeciDegrees, int distance, int localRotation) {
	int localDirectionInRadians = M_PI * localDirectionInDeciDegrees / 1.8; // 180 * 10 / 1000
	
	long sinRotation = SinLookupTable::getSinFromTenthDegrees(globalCurrentTheta + localDirectionInDeciDegrees);
	long cosRotation = SinLookupTable::getCosFromTenthDegrees(globalCurrentTheta + localDirectionInDeciDegrees);

//	int localX = SinLookupTable::getSin(localDirectionInRadians);
//	int localY = SinLookupTable::getCos(localDirectionInRadians);

//	long worldX = (localX * cosRotation) - (localY * sinRotation);
//	long worldY = (localX * sinRotation) + (localY * cosRotation);
        long worldX = -sinRotation;
        long worldY = cosRotation;

	//worldX /= LOOKUP_SCALE;
	//worldY /= LOOKUP_SCALE;

	localCurrentX = 0;
	localCurrentY = 0;

	localFinalX = (worldX * distance) / LOOKUP_SCALE;
	localFinalY = (worldY * distance) / LOOKUP_SCALE;
	globalFinalTheta = globalCurrentTheta + localRotation;

	Serial.print("calculated distance (divide by 1024.0!): ");
	Serial.print(worldX * distance);
	Serial.print(", ");
	Serial.print(worldY * distance);

	Serial.print(", current angle: ");
	Serial.println(globalCurrentTheta);
}

void Wheels::stop() {
  	localCurrentX = 0;
	localCurrentY = 0;
  
	localFinalX = 0;
	localFinalY = 0;
	globalFinalTheta = globalCurrentTheta;
}

void Wheels::resetGlobalPosition(int gyroAngle) {
	worldCurrentX = 0;
	worldCurrentY = 0;
	globalFinalTheta = gyroAngle * 10;
	globalCurrentTheta = gyroAngle * 10;
}

void Wheels::updateGlobalPosition(long leftWheel, long rightWheel, long backWheel, double gyroAngle) {
	double localX, localY, localTheta;
	forwardKinematics(leftWheel, rightWheel, backWheel, localX, localY, localTheta);

	long cosRotation = SinLookupTable::getCosFromTenthDegrees(globalCurrentTheta);
	long sinRotation = SinLookupTable::getSinFromTenthDegrees(globalCurrentTheta);

	//double worldX = (localX * sinRotation) - (localY * cosRotation);
	//double worldY = (localX * cosRotation) + (localY * sinRotation);

	long worldX = (localX * cosRotation) - (localY * sinRotation);
	long worldY = (localX * sinRotation) + (localY * cosRotation);

	worldX /= LOOKUP_SCALE; // rotated from gyroscope
	worldY /= LOOKUP_SCALE;
	
	localCurrentX += worldX;
	localCurrentY += worldY;
	globalCurrentTheta = gyroAngle * 10;

	worldCurrentX += worldX;
	worldCurrentY += worldY;
}

void Wheels::getDesiredWheelPositions(long &desiredLeft, long &desiredRight, long &desiredBack) {
	int diffX = localCurrentX - localFinalX;
	int diffY = localCurrentY - localFinalY;
	int diffTheta = globalCurrentTheta - globalFinalTheta;
	Serial.print("diff x: ");
	Serial.print(diffX);

	Serial.print(", diff y: ");
	Serial.print(diffY);

	Serial.print(", theta diff: ");
	Serial.println(diffTheta);

	inverseKinematics(diffX, diffY, diffTheta, desiredLeft, desiredRight, desiredBack);
}

void Wheels::forwardKinematics(long leftDegrees, long rightDegrees, long backDegrees, double &x, double &y, double &theta) {
	// x = ((sqrt(3) * vRight) / 3) - ((sqrt(3) * vLeft) / 3)
	// y = (vRight / 3) + (vLeft / 3) - ((2 * vBack) / 3)
	// theta = (vRight / 3) + (vLeft / 3) + (vBack / 3)

	const double sqrt3 = 1.732; // sqrt(3) = 1.73205081 

	double left = leftDegrees / WHEEL_DECIDEGREES_TO_MILLIMETERS_DIVISOR; //22.55
	double right = rightDegrees / WHEEL_DECIDEGREES_TO_MILLIMETERS_DIVISOR;
	double back = backDegrees / WHEEL_DECIDEGREES_TO_MILLIMETERS_DIVISOR;

	x = (right + left - (2 * back)) / 3.0;
	y = ((sqrt3 * left) - (sqrt3 * right)) / 3.0; // * 1000, because sqrt3 is * 1000
	theta = (left + right + back) / -3.0;

	theta = (theta * 180) / (11 * M_PI);
}

void Wheels::inverseKinematics(long x, long y, long theta, long &left, long &right, long &back) {
	// vLeft = -vx * sin(150deg) + vy * cos(150deg) + Rw
	// vRight = -vx * sin(30deg) + vy * cos(30deg) + Rw
	// vBack = -vx * sin(270deg) + vy * cos(270deg) + Rw

	x *= DECIDEGREES_TO_MILLIMETERS_DIVISOR; // 22.5
	y *= DECIDEGREES_TO_MILLIMETERS_DIVISOR; // 22.5
	theta /= WHEEL_TO_ROBOT_ROTATION_MULTIPLIER; // 0.21175225

	long temp1 = -0.5 * x;
	long temp2 = (0.866025404 * y);

	right = temp1 + temp2 + theta;
	left = (temp1 - temp2) + theta;
	back = x + theta;
}

// Coordinate system:
//    ^ +Y
//    |
// <--+--> +X
//    |        |
//    V      <- +Theta
