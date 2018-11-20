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
    static bool ptToActuator(AxisFloats& targetPt, AxisFloats& outActuator, AxisPosition& curAxisPositions, AxesParams& axesParams, bool allowOutOfBounds);
    static void actuatorToPt(AxisFloats& actuatorPos, AxisFloats& outPt, AxisPosition& curPos, AxesParams& axesParams);
    static void correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams);
    static ROTATION_TYPE cartesianToActuator(AxisFloats& targetPt, AxisPosition& curAxisPositions, AxisFloats& outActuator, AxesParams& axesParams);
    static float minStepsForMove(float absStepTarget, float absCurSteps, float stepsPerRotation);
    static float absStepForMove(float absStepTarget, float absCurSteps, float stepsPerRotation);
    static void rotationsToPoint(AxisFloats& rotDegrees, AxisFloats& pt, AxesParams& axesParams);
    static void rotationToActuator(float alpha, float beta, AxisFloats& actuatorCoords,
              AxesParams& axesParams);
    static void actuatorToRotation(AxisInt32s& actuatorCoords, AxisFloats& rotationDegrees, AxesParams& axesParams);
    static double cosineRule(double a, double b, double c);
    static inline double wrapRadians(double angle);
    static inline double wrapDegrees(double angle);
    static inline double r2d(double angleRadians);
    static inline double d2r(double angleDegrees);
    static bool isApprox(double v1, double v2, double withinRng = 0.0001);
    static bool isApproxWrap(double v1, double v2, double wrapSize=360.0, double withinRng = 0.0001);
};
