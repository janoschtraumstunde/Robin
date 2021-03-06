#include <WProgram.h>
#include <Wire.h>

#include "CommandData.h"

class MotorBoard {
	static CommandData cmdData;
	static union CommandUnion cmdUnion;

public:
	static void sendCommand(char command) {
		sendCommand(command, 0, 0, 0, 1);
	}

	static void sendCommand(char command, int first) {
		sendCommand(command, first, 0, 0, 3);
	}

	static void sendCommand(char command, int first, int second) {
		sendCommand(command, first, second, 0, 5);
	}

	static void sendCommand(char command, int first, int second, int third) {
		sendCommand(command, first, second, third, 7);
	}

private:
	static void sendCommand(char command, int first, int second, int third, int byteCount) {
		cmdData.command = command;
		cmdData.first = first;
		cmdData.second = second;
		cmdData.third = third;
		sendCommand(cmdData, byteCount);
	}

	static void sendCommand(struct CommandData &cmd, int byteCount) {
		Wire.beginTransmission(1);
		cmdUnion.command = cmd;
		for (int i = 0; i < byteCount; i++) {
			Wire.send(cmdUnion.bytes[i]);
		}
		Wire.endTransmission();	
	}
};

CommandData MotorBoard::cmdData;
CommandUnion MotorBoard::cmdUnion;