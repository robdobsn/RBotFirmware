// RBotFirmware
// Rob Dobson 2016-18

#include <Arduino.h>
#include "RobotSandTableScara.h"
#include "../MotionControl/MotionHelper.h"
#include "Utils.h"
#include "math.h"

// Notes for SandTableScara
// Positive stepping direction for axis 0 is clockwise movement of the upper arm when viewed from top of robot
// Positive stepping direction for axis 1 is anticlockwise movement of the lower arm when viewed from top of robot
// Home position has elbow joint next to elbow position detector and magnet in centre
// In the following the elbow joint at home position is at X=100, Y=0
// Angles of upper arm are calculated clockwise from North
// Angles of lower arm are calculated clockwise from North


RobotSandTableScara::RobotSandTableScara(const char* pRobotTypeName, MotionHelper& motionHelper) :
    RobotBase(pRobotTypeName, motionHelper)
{
    // Set transforms
    _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);

    // Light
    pinMode(A0, OUTPUT);
    digitalWrite(A0, 1);
}

RobotSandTableScara::~RobotSandTableScara()
{
    pinMode(A0, INPUT);
}

// Convert a cartesian point to actuator coordinates
bool RobotSandTableScara::ptToActuator(AxisFloats& targetPt, AxisFloats& outActuator, AxisPosition& curAxisPositions, AxesParams& axesParams, bool allowOutOfBounds)
{
    // Target axis rotations
    AxisFloats prefSolnDegrees;
    ROTATION_TYPE rotationResult = cartesianToActuator(targetPt, curAxisPositions, outActuator, axesParams);
    if ((rotationResult == ROTATION_OUT_OF_BOUNDS) && (!allowOutOfBounds))
        return false;

    //

    // // Get current rotation
    // AxisFloats curRotation;
    // getCurrentRotation(curRotation, axesParams);
    //
    // // Find best geometry solution for movement from current position to new one
    // AxisFloats reqdRotation;
    // getBestMovement(prefSolnDegrees, curRotation, rotationResult, reqdRotation);

    // Convert to actuatorCoords
    // rotationToActuator(prefSolnDegrees, outActuator, curPos, axesParams);

//        testCoordTransforms(axisParams);

    // Find least turning option
    // if ((targetPolarCoords._pt[0] - currentPolarCoords._pt[0]) > M_PI)
    //     targetPolarCoords._pt[0] -= 2 * M_PI;
    // else if ((targetPolarCoords._pt[0] - currentPolarCoords._pt[0]) < -M_PI)
    //     targetPolarCoords._pt[0] += 2 * M_PI;
    // // double curCorrectedBeta = currentPolarCoords._pt[1] + M_PI;
    // if ((targetPolarCoords._pt[1] - currentPolarCoords._pt[1]) > M_PI)
    //     targetPolarCoords._pt[1] -= 2 * M_PI;
    // // else if ((targetPolarCoords._pt[1] - currentPolarCoords._pt[1]) < -M_PI)
    // //     targetPolarCoords._pt[1] += 2 * M_PI;

    // Log.verbose("ptToActuator final targetPolarCoords a %F b %F\n",
    //        targetPolarCoords._pt[0], targetPolarCoords._pt[1]);

    // // Coordinates for axis0
    // actuatorCoords._pt[0] = axisParams[0]._stepsPerRot * targetPolarCoords._pt[0] / M_PI / 2;
    // // Check for very close to origin - in this case don't move the shoulder-elbow as it is unnecessary
    // if (pt._pt[0]*pt._pt[0]+pt._pt[1]*pt._pt[1] < 1.0)
    //     actuatorCoords._pt[0] = axisParams[0]._stepsFromHome;
    //
    // // Convert to steps - note that axis 1 motor is mounted upside down from each other so axis 1 is inverted
    // actuatorCoords._pt[1] = - axisParams[1]._stepsPerRot * targetPolarCoords._pt[1] / M_PI / 2;

    return true;
}

void RobotSandTableScara::actuatorToPt(AxisFloats& actuatorPos, AxisFloats& outPt, AxisPosition& curPos, AxesParams& axesParams)
{
    // Convert to rotations
    AxisFloats rotDegrees;
    AxisInt32s actuatorInt32s;
    actuatorInt32s.set(int32_t(actuatorPos.getVal(0)), int32_t(actuatorPos.getVal(1)), int32_t(actuatorPos.getVal(2)));
    actuatorToRotation(actuatorInt32s, rotDegrees, axesParams);

    // Convert rotations to point
    rotationsToPoint(rotDegrees, outPt, axesParams);

}

void RobotSandTableScara::correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams)
{
    // AxisInt32s cpy = curPos._stepsFromHome;
    // for (int axisIdx = 0; axisIdx < 2; axisIdx++)
    // {
    //     if (curPos._stepsFromHome.getVal(axisIdx) < 0)
    //     {
    //         int32_t intProp = 1 - curPos._stepsFromHome.getVal(axisIdx) / axesParams.getStepsPerRot(axisIdx);
    //         curPos._stepsFromHome.setVal(axisIdx, curPos._stepsFromHome.getVal(axisIdx) + intProp * axesParams.getStepsPerRot(axisIdx));
    //     }
    //     if (curPos._stepsFromHome.getVal(axisIdx) >= axesParams.getStepsPerRot(axisIdx))
    //     {
    //       int32_t intProp = curPos._stepsFromHome.getVal(axisIdx) / axesParams.getStepsPerRot(axisIdx);
    //       curPos._stepsFromHome.setVal(axisIdx, curPos._stepsFromHome.getVal(axisIdx) - intProp * axesParams.getStepsPerRot(axisIdx));
    //     }
    // }
    // Log.verbose("SandTableScara::correctStepOverflow: %d %d -> %d %d\n", cpy.getVal(0), cpy.getVal(1), curPos._stepsFromHome.getVal(0), curPos._stepsFromHome.getVal(1));
}

RobotSandTableScara::ROTATION_TYPE RobotSandTableScara::cartesianToActuator(AxisFloats& targetPt, AxisPosition& curAxisPositions, AxisFloats& outActuator, AxesParams& axesParams)
{
    // Calculate arm lengths
    // The _unitsPerRot values in the axisParams indicate the circumference of the circle
    // formed by moving each arm through 360 degrees
    double shoulderElbowMM = axesParams.getunitsPerRot(0) / M_PI / 2;
    double elbowHandMM = axesParams.getunitsPerRot(1) / M_PI / 2;

    // Calculate the two different configurations of upper and lower arm that can by used to reach any point
    // with the exception of the centre of the machine (many solutions to this) and points on perimeter of maximum
    // circle (only one solution)
    // All angles are calculated clockwise from North

    // Check for 0,0 point as this is a special case for this robot
    if (isApprox(targetPt._pt[0],0,0.5) && (isApprox(targetPt._pt[1],0,0.5)))
    {
        // Special case
        Log.verbose("SandTableScara::cartesianToActuator x %F y %F close to origin\n", targetPt._pt[0], targetPt._pt[1]);

        // Keep the current position for alpha, set beta to alpha+180 (i.e. doubled-back so eng-effector is in centre)
        outActuator.setVal(0, 0);
        outActuator.setVal(1, 0);
        return ROTATION_IS_NEAR_CENTRE;
    }

    // Calculate distance from origin to pt (forms one side of triangle where arm segments form other sides)
    double thirdSideMM = sqrt(pow(targetPt._pt[0],2) + pow(targetPt._pt[1], 2));

    // Check validity of position (use max val for X axis as the robot is circular X and Y max will be the same)
    float maxValForXAxis = 0;
    bool maxValid = axesParams.getMaxVal(0, maxValForXAxis);
    bool posValid = thirdSideMM <= shoulderElbowMM + elbowHandMM;
    if (maxValid)
        posValid = posValid && (thirdSideMM <= maxValForXAxis);

    // Calculate angle from North to the point (note in atan2 X and Y are flipped from normal as angles are clockwise)
    double delta1 = atan2(targetPt._pt[0], targetPt._pt[1]);
    if (delta1 < 0)
        delta1 += M_PI * 2;

    // Calculate angle of triangle opposite elbow-hand side
    double delta2 = cosineRule(thirdSideMM, shoulderElbowMM, elbowHandMM);

    // Calculate angle of triangle opposite third side
    double innerAngleOppThird = cosineRule(shoulderElbowMM, elbowHandMM, thirdSideMM);

    // The two pairs of angles that solve these equations
    // alpha is the angle from shoulder to elbow
    // beta is angle from elbow to hand
    double alpha1rads = delta1 - delta2;
    double beta1rads = alpha1rads - innerAngleOppThird + M_PI;
    double alpha2rads = delta1 + delta2;
    double beta2rads = alpha2rads + innerAngleOppThird - M_PI;

    // Calculate the alpha and beta angles
    double alpha1 = r2d(wrapRadians(alpha1rads + 2 * M_PI));
    double beta1 = r2d(wrapRadians(beta1rads + 2 * M_PI));
    double alpha2 = r2d(wrapRadians(alpha2rads + 2 * M_PI));
    double beta2 = r2d(wrapRadians(beta2rads + 2 * M_PI));

    // Find step possibilities
    AxisFloats actuator1, actuator2;
    rotationToActuator(alpha1, beta1, actuator1, axesParams);
    rotationToActuator(alpha2, beta2, actuator2, axesParams);

    // Find the option with the minimum steps
    float stepCount1 = minStepsForMove(actuator1.getVal(0), curAxisPositions._stepsFromHome.getVal(0), axesParams.getStepsPerRot(0)) +
                        minStepsForMove(actuator1.getVal(1), curAxisPositions._stepsFromHome.getVal(1), axesParams.getStepsPerRot(1));
    float stepCount2 = minStepsForMove(actuator2.getVal(0), curAxisPositions._stepsFromHome.getVal(0), axesParams.getStepsPerRot(0)) +
                        minStepsForMove(actuator2.getVal(1), curAxisPositions._stepsFromHome.getVal(1), axesParams.getStepsPerRot(1));

    if (stepCount1 < stepCount2)
    {
        outActuator.setVal(0, absStepForMove(actuator1.getVal(0), curAxisPositions._stepsFromHome.getVal(0), axesParams.getStepsPerRot(0)));
        outActuator.setVal(1, absStepForMove(actuator1.getVal(1), curAxisPositions._stepsFromHome.getVal(1), axesParams.getStepsPerRot(1)));
    }
    else
    {
        outActuator.setVal(0, absStepForMove(actuator2.getVal(0), curAxisPositions._stepsFromHome.getVal(0), axesParams.getStepsPerRot(0)));
        outActuator.setVal(1, absStepForMove(actuator2.getVal(1), curAxisPositions._stepsFromHome.getVal(1), axesParams.getStepsPerRot(1)));
    }

    // Debug
    Log.verbose("SandTableScara::ptToRotations %s fromCtr %Fmm D1 %Fd D2 %Fd innerAng %Fd\n",
            posValid ? "ok" : "OUT_OF_BOUNDS",
            thirdSideMM, delta1 * 180 / M_PI, delta2 * 180 / M_PI, innerAngleOppThird * 180 / M_PI);
    Log.verbose("SandTableScara::ptToRotations alpha1 %Fd, beta1 %Fd, steps1 %F, alpha2 %Fd, beta2 %Fd, steps2 %F, prefOption %d\n",
            alpha1, beta1, stepCount1, alpha2, beta2, stepCount2, stepCount1 < stepCount2 ? 1 : 2);
    Log.verbose("SandTableScara::ptToRotations ----------- curA %d curB %d stA %F stB %F\n",
                    curAxisPositions._stepsFromHome.getVal(0), curAxisPositions._stepsFromHome.getVal(1),
                    outActuator.getVal(0), outActuator.getVal(1));

    if (!posValid)
        return ROTATION_OUT_OF_BOUNDS;
    return ROTATION_NORMAL;
}

float RobotSandTableScara::minStepsForMove(float absStepTarget, float absCurSteps, float stepsPerRotation)
{
    float stepsAbsDiff = fabs(absStepTarget-absCurSteps);
    if (stepsAbsDiff > stepsPerRotation/2)
        stepsAbsDiff = stepsPerRotation - stepsAbsDiff;
    return stepsAbsDiff;
}

float RobotSandTableScara::absStepForMove(float absStepTarget, float absCurSteps, float stepsPerRotation)
{
    if (absStepTarget > absCurSteps)
    {
        if (fabs(absStepTarget-absCurSteps) > stepsPerRotation/2)
        {
            return absStepTarget - stepsPerRotation;
        }
    }
    else
    {
        if (fabs(absStepTarget-absCurSteps) > stepsPerRotation/2)
        {
            return absStepTarget + stepsPerRotation;
        }
    }
    return absStepTarget;
}

void RobotSandTableScara::rotationsToPoint(AxisFloats& rotDegrees, AxisFloats& pt, AxesParams& axesParams)
{
    // Calculate arm lengths
    // The _unitsPerRot values in the axisParams indicate the circumference of the circle
    // formed by moving each arm through 360 degrees
    double shoulderElbowMM = axesParams.getunitsPerRot(0) / M_PI / 2;
    double elbowHandMM = axesParams.getunitsPerRot(1) / M_PI / 2;

    // Trig to get elbow location - alpha and beta are clockwise from North
    // But the home position has the lower arm at 180 degrees
    double alpha = d2r(rotDegrees._pt[0]);
    double beta = d2r(wrapDegrees(rotDegrees._pt[1]+180));
    double x1 = shoulderElbowMM * sin(alpha);
    double y1 = shoulderElbowMM * cos(alpha);

    // Trig to get hand location
    double x2 = x1 + elbowHandMM * sin(beta);
    double y2 = y1 + elbowHandMM * cos(beta);

    // Output
    pt._pt[0] = x2;
    pt._pt[1] = y2;

    Log.verbose("SandTableScara::rotationsToPoint: alpha %Fd beta %Fd => X %F Y %F shoulderElbowMM %F elbowHandMM %F\n",
            rotDegrees._pt[0], rotDegrees._pt[1], pt._pt[0], pt._pt[1],
            shoulderElbowMM, elbowHandMM);

}

void RobotSandTableScara::rotationToActuator(float alpha, float beta, AxisFloats& actuatorCoords,
            AxesParams& axesParams)
{
    // Axis 0 positive steps clockwise, axis 1 postive steps are anticlockwise
    // Axis 0 zero steps is at 0 degrees, axis 1 zero steps is at 180 degrees
    float alphaStepTarget = alpha * axesParams.getStepsPerRot(0) / 360;
    actuatorCoords._pt[0] = alphaStepTarget;
    // For beta values the rotation should always be between 0 steps and + 1/2 * stepsPerRotation
    float betaStepTarget = axesParams.getStepsPerRot(1) - wrapDegrees(beta - 180) * axesParams.getStepsPerRot(1) / 360;
    actuatorCoords._pt[1] = betaStepTarget;
    Log.verbose("SandTableScara::rotationToActuator: alpha %Fd beta %Fd ax0Steps %F ax1Steps %F\n",
            alpha, beta, actuatorCoords._pt[0], actuatorCoords._pt[1]);
}

void RobotSandTableScara::actuatorToRotation(AxisInt32s& actuatorCoords, AxisFloats& rotationDegrees, AxesParams& axesParams)
{
    // Axis 0 positive steps clockwise, axis 1 postive steps are anticlockwise
    // Axis 0 zero steps is at 0 degrees, axis 1 zero steps is at 180 degrees
    // All angles returned are in degrees clockwise from North
    double axis0Degrees = wrapDegrees(actuatorCoords.getVal(0) * 360 / axesParams.getStepsPerRot(0));
    double alpha = axis0Degrees;
    double axis1Degrees = wrapDegrees(540 - (actuatorCoords.getVal(1) * 360 / axesParams.getStepsPerRot(1)));
    double beta = axis1Degrees;
    rotationDegrees.set(alpha, beta);
    Log.verbose("SandTableScara::actuatorToRotation: ax0Steps %d ax1Steps %d a %Fd b %Fd\n",
            actuatorCoords.getVal(0), actuatorCoords.getVal(1), rotationDegrees._pt[0], rotationDegrees._pt[1]);
}

// static void getCurrentRotation(AxisFloats& rotationDegrees, AxesParams& axesParams)
// {
//     // Get current steps from home
//     AxisFloats actuatorCoords;
//     actuatorCoords.X(axesParams.gethomeOffSteps(0));
//     actuatorCoords.Y(axesParams.gethomeOffSteps(1));
//
//     actuatorToRotation(actuatorCoords, rotationDegrees, axesParams);
//
//     Log.verbose("getCurrentRotation ax0FromHome %ld ax1FromHome %ld alpha %Fd beta %Fd\n",
//                 axesParams.gethomeOffSteps(0), axesParams.gethomeOffSteps(1),
//                 rotationDegrees._pt[0], rotationDegrees._pt[1]);
// }

// static double calcMinAngleDiff(double target, float& finalAngle, double current)
// {
//     // Wrap angles to 0 <= angle < 360
//     double wrapTarget = wrapDegrees(target);
//     double wrapCurrent = wrapDegrees(current);
//
//     // Minimum difference (based on turning in the right direction)
//     double diff = wrapDegrees(target - current);
//     double minDiff = (diff > 180) ? 360 - diff : diff;
//
//     // Find the angle which involves minimal turning
//     if (wrapCurrent < 180)
//     {
//         if ((wrapTarget < wrapCurrent + 180) && (wrapTarget > wrapCurrent))
//             finalAngle = current + minDiff;
//         else
//             finalAngle = current - minDiff;
//     }
//     else
//     {
//         if ((wrapTarget > wrapCurrent - 180) && (wrapTarget < wrapCurrent))
//             finalAngle = current - minDiff;
//         else
//             finalAngle = current + minDiff;
//     }
//
//     // Check valid
//     bool checkDiff = finalAngle > current ? finalAngle - current : current - finalAngle;
//     bool checkDest = isApprox(wrapDegrees(finalAngle), wrapTarget, 0.01);
//     Log.verbose("calcMinAngleDiff %s %s tgt %F cur %F mindiff %F (%F) final %F, wrapFinal %F, wrapTarget %F\n",
//                 checkDiff ? "OK" : "DIFF_WRONG *********",
//                 checkDest ? "OK" : "DEST_WRONG *********",
//                 target, current, minDiff, diff, finalAngle, wrapDegrees(finalAngle), wrapTarget);
//     return minDiff;
// }

// static void getBestMovement(AxisFloats& prefSolnDegrees, AxisFloats& curRotation,
//                 ROTATION_TYPE rotType, AxisFloats& outSolution)
// {
    // Select the solution which keeps the difference between 0 and 180 degrees
    // double betweenArmsOpt1 = wrapDegrees(option1._pt[1] - option1._pt[0]);
    // double betweenArmsOpt2 = wrapDegrees(option2._pt[1] - option2._pt[0]);
    // Log.verbose("getBestMovement: opt1pt0 %F opt1pt1 %F bet1 %F opt2pt0 %F opt1pt1 %F bet2 %F\n",
    //             option1._pt[0], option1._pt[1], betweenArmsOpt1, option2._pt[0], option2._pt[1], betweenArmsOpt2);
    // if (betweenArmsOpt1 >= 0 && betweenArmsOpt1 <= 180)
    //     outSolution = option1;
    // else
    //     outSolution = option2;

    // // Check for point near centre
    // if (rotType == ROTATION_IS_NEAR_CENTRE)
    // {
    //     // In this case leave the upper arm where it is
    //     outSolution._pt[0] = curRotation._pt[0];
    //
    //     // Move lower arm to upper arm angle +/- 180
    //     double angleDiff = curRotation._pt[0] + 180 - curRotation._pt[1];
    //     if (angleDiff > 180)
    //         outSolution._pt[1] = curRotation._pt[0] - 180;
    //     else
    //         outSolution._pt[1] = curRotation._pt[0] + 180;
    //     Log.verbose("getBestMovement 0,0 axis0Cur %F axis1Cur %F angleDiff %FD alpha %FD beta %FD\n",
    //                 curRotation._pt[0], curRotation._pt[1], angleDiff,
    //                 outSolution._pt[0], outSolution._pt[1]);
    //     return;
    // }
    //
    // Option 1 and Option 2 are two solutions to the position required
    // Calculate the total angle differences for each axis in each case
    // AxisFloats newOption1, newOption2;
    // double ax0Opt1Diff = calcMinAngleDiff(option1._pt[0], newOption1._pt[0], curRotation._pt[0]);
    // double ax1Opt1Diff = calcMinAngleDiff(option1._pt[1], newOption1._pt[1], curRotation._pt[1]);
    // double ax0Opt2Diff = calcMinAngleDiff(option2._pt[0], newOption2._pt[0], curRotation._pt[0]);
    // double ax1Opt2Diff = calcMinAngleDiff(option2._pt[1], newOption2._pt[1], curRotation._pt[1]);
    // Log.verbose("getBestMovement ax0 cur %F newTgtOpt1 %F newTgtOpt2 %F diff1 %F diff2 %FD \n",
    //             curRotation._pt[0], newOption1._pt[0], newOption2._pt[0], ax0Opt1Diff, ax0Opt2Diff);
    // Log.verbose("getBestMovement ax1 cur %F newTgtOpt1 %F newTgtOpt2 %F diff1 %F diff2 %FD \n",
    //             curRotation._pt[1], newOption1._pt[1], newOption2._pt[1], ax1Opt1Diff, ax1Opt2Diff);

    // // Weight the turning of ax0 more than ax1
    // double AX0_WEIGHTING_FACTOR = 1.3;
    // double opt1TotalDiff = fabs(ax0Opt1Diff) * AX0_WEIGHTING_FACTOR + fabs(ax1Opt1Diff);
    // double opt2TotalDiff = fabs(ax0Opt2Diff) * AX0_WEIGHTING_FACTOR + fabs(ax1Opt2Diff);
    // Log.verbose("getBestMovement option1Weighted %F option2Weighted %F \n",
    //             opt1TotalDiff, opt2TotalDiff);
    //
    // // Choose the option with the least total turning
    // if (opt1TotalDiff < opt2TotalDiff)
    //     outSolution = newOption1;
    // else
    //     outSolution = newOption2;
//
//     outSolution = prefSolnDegrees;
// }

double RobotSandTableScara::cosineRule(double a, double b, double c)
{
    // Calculate angle C of a triangle using the cosine rule
    // Log.verbose("cosineRule a %F b %F c %F acos %F = %F\n",
    //         a, b, c, (a*a + b*b - c*c) / (2 * a * b), acos((a*a + b*b - c*c) / (2 * a * b)));
    double val = (a*a + b*b - c*c) / (2 * a * b);
    if (val > 1) val = 1;
    if (val < -1) val = -1;
    return acos(val);
}

inline double RobotSandTableScara::wrapRadians( double angle )
{
    static const double twoPi = 2.0 * M_PI;
    // Log.verbose("wrapRadians %F %F\n", angle, angle - twoPi * floor( angle / twoPi ));
    return angle - twoPi * floor( angle / twoPi );
}
inline double RobotSandTableScara::wrapDegrees( double angle )
{
    // Log.verbose("wrapDegrees %F %F\n", angle, angle - 360 * floor( angle / 360 ));
    return angle - 360.0 * floor( angle / 360.0 );
}
inline double RobotSandTableScara::r2d(double angleRadians)
{
    // Log.verbose("r2d %F %F\n", angleRadians, angleRadians * 180.0 / M_PI);
    return angleRadians * 180.0 / M_PI;
}
inline double RobotSandTableScara::d2r(double angleDegrees)
{
    return angleDegrees * M_PI / 180.0;
}
bool RobotSandTableScara::isApprox(double v1, double v2, double withinRng)
{
    // Log.verbose("isApprox %F %F = %d\n", v1, v2, fabs(v1 - v2) < withinRng);
    return fabs(v1 - v2) < withinRng;
}
bool RobotSandTableScara::isApproxWrap(double v1, double v2, double wrapSize, double withinRng)
{
    // Log.verbose("isApprox %F %F = %d\n", v1, v2, fabs(v1 - v2) < withinRng);
    double t1 = v1 - wrapSize * floor(v1 / wrapSize);
    double t2 = v2 - wrapSize * floor(v2 / wrapSize);
    return (fabs(t1 - t2) < withinRng) || (fabs(t1 - wrapSize - t2) < withinRng) || (fabs(t1 + wrapSize - t2) < withinRng);
}

/*
static void testCoordTransforms(AxisParams axisParams[])
{

    double testPts [] [2] =
    {
        { 0, 0 },
        { 100, 100 },
        { 100, -100 },
        { -100, 100 },
        { -100, -100 },
        { 200, 0 },
        { 0, 200 },
        { -200, 0 },
        { 0, -200 },
        { 0, 100 },
        { 100, 0 },
        { 0, -100 },
        { -100, 0 },
    };

    double checkOutPts [] [2] [2] =
    {
        { { 0, 180}, { 180 , 0} },
        { { 90, 0}, { 0, 90} },
        { { 0, 270}, { 270, 0} },
        { { 180, 90}, { 90, 180} },
        { { 180, 270}, { 270, 180} },
        { { 0, 0}, { 0, 0} },
        { { 90, 90}, { 90, 90} },
        { { 180, 180}, { 180, 180} },
        { { 270, 270}, { 270, 270} },
        { { 30, 150}, { 150, 30} },
        { { 60, 300}, { 300, 60} },
        { { 210, 330}, { 330, 210} },
        { { 240, 120}, { 120, 240} },
    };

    for (int testIdx = 0; testIdx < (sizeof(testPts) / sizeof(testPts[0])); testIdx++)
    {
        // Check forward conversion
        PointND newPt(testPts[testIdx][0], testPts[testIdx][1]);
        PointND outPt1, outPt2;
        Log.verbose("TestPoint x %F y %F\n", newPt._pt[0], newPt._pt[1]);
        bool valid = ptToRotations(newPt, outPt1, outPt2, axisParams);
        bool outPt1Valid = ((isApproxWrap(outPt1._pt[0], checkOutPts[testIdx][0][0]) && isApproxWrap(outPt1._pt[1], checkOutPts[testIdx][0][1])) ||
            (isApproxWrap(outPt1._pt[0], checkOutPts[testIdx][1][0]) && isApproxWrap(outPt1._pt[1], checkOutPts[testIdx][1][1])));
        bool outPt2Valid = ((isApproxWrap(outPt2._pt[0], checkOutPts[testIdx][0][0]) && isApproxWrap(outPt2._pt[1], checkOutPts[testIdx][0][1])) ||
            (isApproxWrap(outPt2._pt[0], checkOutPts[testIdx][1][0]) && isApproxWrap(outPt2._pt[1], checkOutPts[testIdx][1][1])));
        if (outPt1Valid && outPt2Valid)
            Log.verbose("outPts Valid\n");
        else
            Log.verbose("outPt1 or outPt2 INVALID a1 %F b1 %F a2 %F b2 %F ****************************************************************\n",
                    checkOutPts[testIdx][0][0], checkOutPts[testIdx][0][1], checkOutPts[testIdx][1][0], checkOutPts[testIdx][1][1]);
        // Reverse process
        PointND actuatorCoords;
        rotationToActuator(outPt1, actuatorCoords, axisParams);
        Log.verbose("Actuator ax0Steps %ld ax1Steps %ld", long(actuatorCoords._pt[0]), long(actuatorCoords._pt[1]));
        PointND rotOut;
        actuatorToRotation(actuatorCoords, rotOut, axisParams);
        bool rotOutValid = isApproxWrap(rotOut._pt[0], outPt1._pt[0]) && isApproxWrap(rotOut._pt[1], outPt1._pt[1]);
        if (rotOutValid)
            Log.verbose("rotOutValid\n");
        else
            Log.verbose("rotOut INVALID ****************************************************************\n");

        //
        PointND finalPoint;
        rotationsToPoint(rotOut, finalPoint, axisParams);
        bool finalPtValid = isApprox(newPt._pt[0], finalPoint._pt[0],0.1) && isApprox(newPt._pt[1], finalPoint._pt[1],0.1);
        if (finalPtValid)
            Log.verbose("finalPtValid\n");
        else
            Log.verbose("finalPoint INVALID x %F y %F ****************************************************************\n",
                    finalPoint._pt[0], finalPoint._pt[1]);
    }
}
*/
