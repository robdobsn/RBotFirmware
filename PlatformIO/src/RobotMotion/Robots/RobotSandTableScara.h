// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "RobotBase.h"

class AxisFloats;
class AxisPosition;
class MotionHelper;
class AxesParams;
class AxisInt32s;

class RobotSandTableScara : public RobotBase
{
public:
    static const int NUM_ROBOT_AXES = 2;
    enum ROTATION_TYPE {
        ROTATION_NORMAL,
        ROTATION_OUT_OF_BOUNDS,
        ROTATION_IS_NEAR_CENTRE,
    };

public:
    // Notes for SandTableScara
    // Positive stepping direction for axis 0 is clockwise movement of the upper arm when viewed from top of robot
    // Positive stepping direction for axis 1 is anticlockwise movement of the lower arm when viewed from top of robot
    // Home position has elbow joint next to elbow position detector and magnet in centre
    // In the following the elbow joint at home position is at X=100, Y=0
    // Angles of upper arm are calculated clockwise from North
    // Angles of lower arm are calculated clockwise from North
    RobotSandTableScara(const char* pRobotTypeName, MotionHelper& motionHelper);
    ~RobotSandTableScara();

private:

    // Convert a cartesian point to actuator coordinates
    static bool ptToActuator(AxisFloats& targetPt, AxisFloats& outActuator, 
                AxisPosition& curPos, AxesParams& axesParams, bool allowOutOfBounds);

    // Convert actuator values to cartesian point
    static void actuatorToPt(AxisInt32s& targetActuator, AxisFloats& outPt,
                AxisPosition& curPos, AxesParams& axesParams);

    // Correct overflow (necessary for continuous rotation robots)
    static void correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams);

    // Convert coordinates in place
    static void convertCoords(RobotCommandArgs& cmdArgs, AxesParams& axesParams);

    // Set robot attributes
    static void setRobotAttributes(AxesParams& axesParams, String& robotAttributes);

private:
    static bool cartesianToPolar(AxisFloats& targetPt, AxisFloats& targetSoln1, 
                    AxisFloats& targetSoln2, AxesParams& axesParams);
    static void stepsToPolar(AxisInt32s& actuatorCoords, AxisFloats& rotationDegrees, AxesParams& axesParams);
    static float calcRelativePolar(float targetRotation, float curRotation);
    static void relativePolarToSteps(AxisFloats& relativePolar, AxisPosition& curAxisPositions, 
            AxisFloats& outActuator, AxesParams& axesParams);

};
