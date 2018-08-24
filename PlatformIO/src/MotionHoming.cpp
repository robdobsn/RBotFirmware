// RBotFirmware
// Rob Dobson 2016-18

#include "MotionHelper.h"
#include "MotionHoming.h"

void MotionHoming::moveTo(RobotCommandArgs& args)
{
  _pMotionHelper->moveTo(args);
}

int MotionHoming::getLastCompletedNumberedCmdIdx()
{
  return _pMotionHelper->getLastCompletedNumberedCmdIdx();
}

void MotionHoming::setAtHomePos(int axisIdx)
{
  _pMotionHelper->setCurPositionAsHome(axisIdx);
}
