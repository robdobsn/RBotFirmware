// TestPipelinePlannerCLRCPP.cpp : main project file.

#include "stdafx.h"
#include "conio.h"

using namespace System;

#include "RobotController.h"

int main()
{
    Console::WriteLine(L"Hello World");

	RobotController robotController;
	RobotCommandArgs rcArgs;
	
	rcArgs.setAxisValue(0, 1, 0);
	rcArgs.setAxisValue(1, 0, 0);
	robotController.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 2, 0);
	rcArgs.setAxisValue(1, 0, 0);
	robotController.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 3, 0);
	rcArgs.setAxisValue(1, 0, 0);
	robotController.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 3.5, 0);
	rcArgs.setAxisValue(1, 0.5, 0);
	robotController.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 4, 0);
	rcArgs.setAxisValue(1, 1.5, 0);
	robotController.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 5, 0);
	rcArgs.setAxisValue(1, 3, 0);
	robotController.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 6, 0);
	rcArgs.setAxisValue(1, 5, 0);
	robotController.moveTo(rcArgs);
	rcArgs.setAxisValue(0, 7, 0);
	rcArgs.setAxisValue(1, 7, 0);
	robotController.moveTo(rcArgs);

	//rcArgs.setAxisValue(0, 1, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//robotController.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 2, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//robotController.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 3, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//robotController.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 4, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//robotController.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 5, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//robotController.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 6, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//robotController.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 7, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//robotController.moveTo(rcArgs);
	//rcArgs.setAxisValue(0, 8, 0);
	//rcArgs.setAxisValue(1, 0, 0);
	//robotController.moveTo(rcArgs);


	robotController.debugIt();
	while (true)
	{
		if (_kbhit())
			break;
		robotController.service();
	}
    return 0;
}
