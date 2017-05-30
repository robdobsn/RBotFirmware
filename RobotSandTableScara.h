// RBotFirmware
// Rob Dobson 2016

#pragma once

#include "application.h"
#include "Utils.h"
#include "RobotBase.h"
#include "MotionHelper.h"
#include "math.h"

class RobotSandTableScara : public RobotBase
{
public:
    static const int NUM_ROBOT_AXES = 2;
    typedef enum ROTATION_TYPE {
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
    // Angles of upper arm are calculated anticlockwise from home position (i.e. anticlockwise from East)
    // Angles of lower arm are calculated anticlockwise from home posiiton (i.e. anticlockwise from West)

    // Convert a cartesian point to actuator coordinates
    static bool ptToActuator(MotionPipelineElem& motionElem, PointND& actuatorCoords, AxisParams axisParams[], int numAxes)
    {
        // Target axis rotations
        PointND rot1Degrees, rot2Degrees;
        ROTATION_TYPE rotationResult = ptToRotations(motionElem._pt2MM, rot1Degrees, rot2Degrees, axisParams);
        if (rotationResult == ROTATION_OUT_OF_BOUNDS)
            return false;

        // Get current rotation
        PointND curRotation;
        getCurrentRotation(curRotation, axisParams);

        // Find best geometry solution for movement from current position to new one
        PointND reqdRotation;
        getBestMovement(rot1Degrees, rot2Degrees, curRotation, rotationResult, reqdRotation);

        // Convert to actuatorCoords
        rotationToActuator(reqdRotation, actuatorCoords, axisParams);

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

        // Log.trace("ptToActuator final targetPolarCoords a %0.2f b %0.2f",
        //        targetPolarCoords._pt[0], targetPolarCoords._pt[1]);

        // // Coordinates for axis0
        // actuatorCoords._pt[0] = axisParams[0]._stepsPerRotation * targetPolarCoords._pt[0] / M_PI / 2;
        // // Check for very close to origin - in this case don't move the shoulder-elbow as it is unnecessary
        // if (pt._pt[0]*pt._pt[0]+pt._pt[1]*pt._pt[1] < 1.0)
        //     actuatorCoords._pt[0] = axisParams[0]._stepsFromHome;
        //
        // // Convert to steps - note that axis 1 motor is mounted upside down from each other so axis 1 is inverted
        // actuatorCoords._pt[1] = - axisParams[1]._stepsPerRotation * targetPolarCoords._pt[1] / M_PI / 2;

        return true;
    }

    static void actuatorToPt(PointND& actuatorCoords, PointND& pt, AxisParams axisParams[], int numAxes)
    {
        // Convert to rotations
        PointND rotDegrees;
        actuatorToRotation(actuatorCoords, rotDegrees, axisParams);

        // Convert rotations to point
        rotatonsToPoint(rotDegrees, pt, axisParams);

    }

    static void correctStepOverflow(AxisParams axisParams[], int numAxes)
    {
        Log.trace("correctStepOverflow - currently unimplemented");
    }

private:

    static ROTATION_TYPE ptToRotations(PointND& pt, PointND& rot1Degrees, PointND& rot2Degrees, AxisParams axisParams[])
    {
        // Calculate arm lengths
        // The _unitsPerRotation values in the axisParams indicate the circumference of the circle
        // formed by moving each arm through 360 degrees
        double shoulderElbowMM = axisParams[0]._unitsPerRotation / M_PI / 2;
        double elbowHandMM = axisParams[1]._unitsPerRotation / M_PI / 2;

        // Calculate the two different configurations of upper and lower arm that can by used to reach any point
        // with the exception of the centre of the machine (many solutions to this) and points on perimeter of working
        // circle (only one solution)
        // All angles are calculated anticlockwise from East

        // Check for 0,0 point as this is a special case for this robot
        if (isApprox(pt._pt[0],0,1) && (isApprox(pt._pt[1],0,1)))
        {
            // Special case
            Log.trace("ptToRotations x %0.2f y %0.2f close to zero", pt._pt[0], pt._pt[1]);

            // Return values
            rot1Degrees.set(0.0, 180.0);
            rot2Degrees.set(0.0, 180.0);
            return ROTATION_IS_NEAR_CENTRE;
        }

        // Calculate distance from origin to pt (forms one side of triangle where arm segments form other sides)
        double thirdSideMM = sqrt(pow(pt._pt[0],2) + pow(pt._pt[1], 2));

        // Check validity of position
        bool posValid = thirdSideMM <= shoulderElbowMM + elbowHandMM;

        // Calculate upper arm partial angle
        double delta1 = atan2(pt._pt[1], pt._pt[0]);

        // Calculate inner angle of triangle opposite elbow-hand side
        double delta2 = cosineRule(thirdSideMM, shoulderElbowMM, elbowHandMM);

        // Calculate inner angle of triangle related opposite third side (relative to shoulder-elbow)
        double innerAngleOppThird = cosineRule(shoulderElbowMM, elbowHandMM, thirdSideMM);

        // The two pairs of angles that solve these equations
        // alpha is the angle from shoulder to elbow
        // beta is angle from elbow to hand
        double alpha1 = r2d(wrapRadians(delta1 + delta2));
        double beta1 = r2d(wrapRadians(delta1 + delta2 + M_PI + innerAngleOppThird + 2 * M_PI));
        double alpha2 = r2d(wrapRadians(delta1 - delta2 + 2 * M_PI));
        double beta2 = r2d(wrapRadians(delta1 - delta2 + M_PI - innerAngleOppThird + 2 * M_PI));

        // Debug
        Log.trace("ptToRotationsDebug %s dist %0.2fmm D1 %0.2fD D2 %0.2fD innerAng %0.2fD",
                posValid ? "ok" : "OUT_OF_BOUNDS",
                thirdSideMM, delta1 * 180 / M_PI, delta2 * 180 / M_PI, innerAngleOppThird * 180 / M_PI);
        Log.trace("ptToRotationsDebug alpha1 %0.2fD, beta1 %0.2fD, alpha2 %0.2fD, beta2 %0.2fD",
                alpha1, beta1, alpha2, beta2);

        // Return values
        rot1Degrees.set(alpha1, beta1);
        rot2Degrees.set(alpha2, beta2);

        return ROTATION_NORMAL;
    }

    static void rotatonsToPoint(PointND& rotDegrees, PointND& pt, AxisParams axisParams[])
    {
        // Calculate arm lengths
        // The _unitsPerRotation values in the axisParams indicate the circumference of the circle
        // formed by moving each arm through 360 degrees
        double shoulderElbowMM = axisParams[0]._unitsPerRotation / M_PI / 2;
        double elbowHandMM = axisParams[1]._unitsPerRotation / M_PI / 2;

        // Trig to get elbow location - alpha is clockwise from North
        double alpha = d2r(rotDegrees._pt[0]);
        double beta = d2r(rotDegrees._pt[1]);
        double x1 = shoulderElbowMM * cos(alpha);
        double y1 = shoulderElbowMM * sin(alpha);

        // Trig to get hand location
        double x2 = x1 + elbowHandMM * cos(beta);
        double y2 = y1 + elbowHandMM * sin(beta);

        // Output
        pt._pt[0] = x2;
        pt._pt[1] = y2;

        Log.trace("rotatonsToPoint alpha %0.2fD beta %0.2fD => X %0.2f Y %0.2f shoulderElbowMM %0.2f elbowHandMM %0.2f",
                rotDegrees._pt[0], rotDegrees._pt[1], pt._pt[0], pt._pt[1],
                shoulderElbowMM, elbowHandMM);

    }

    static void rotationToActuator(PointND& rotationDegrees, PointND& actuatorCoords, AxisParams axisParams[])
    {
        // Axis 0 positive steps clockwise, axis 1 postive steps are anticlockwise
        // Axis 0 zero steps is at 0 degrees, axis 1 zero steps is at 180 degrees
        actuatorCoords._pt[0] = - rotationDegrees._pt[0] * axisParams[0]._stepsPerRotation / 360;
        actuatorCoords._pt[1] = (rotationDegrees._pt[1] - 180) * axisParams[1]._stepsPerRotation / 360;
        Log.trace("rotationToActuator a %0.2fD b %0.2fD ax1Steps %0.2f ax1Steps %0.2f",
                rotationDegrees._pt[0], rotationDegrees._pt[1], actuatorCoords._pt[0], actuatorCoords._pt[1]);
    }

    static void actuatorToRotation(PointND& actuatorCoords, PointND& rotationDegrees, AxisParams axisParams[])
    {
        // Axis 0 positive steps clockwise, axis 1 postive steps are anticlockwise
        // Axis 0 zero steps is at 0 degrees, axis 1 zero steps is at 180 degrees
        // All angles returned are in degrees anticlockwise from East
        double axis0Degrees = actuatorCoords._pt[0] * 360 / axisParams[0]._stepsPerRotation;
        double alpha = -axis0Degrees;
        double axis1Degrees = actuatorCoords._pt[1] * 360 / axisParams[1]._stepsPerRotation;
        double beta = 180 + axis1Degrees;
        rotationDegrees.set(alpha, beta);
        Log.trace("actuatorToRotation ax0Steps %0.2f ax1Steps %0.2f a %0.2fD b %0.2fD",
                actuatorCoords._pt[0], actuatorCoords._pt[1], rotationDegrees._pt[0], rotationDegrees._pt[1]);
    }

    static void getCurrentRotation(PointND& rotationDegrees, AxisParams axisParams[])
    {
        // Get current steps from home
        PointND actuatorCoords;
        actuatorCoords._pt[0] = axisParams[0]._stepsFromHome;
        actuatorCoords._pt[1] = axisParams[1]._stepsFromHome;

        actuatorToRotation(actuatorCoords, rotationDegrees, axisParams);

        Log.trace("getCurrentRotation ax0FromHome %ld ax1FromHome %ld alpha %0.2fD beta %02.fD",
                    axisParams[0]._stepsFromHome, axisParams[1]._stepsFromHome,
                    rotationDegrees._pt[0], rotationDegrees._pt[1]);
    }

    static double calcMinAngleDiff(double target, double& finalAngle, double current)
    {
        // Wrap angles to 0 <= angle < 360
        double wrapTarget = wrapDegrees(target);
        double wrapCurrent = wrapDegrees(current);

        // Minimum difference (based on turning in the right direction)
        double diff = wrapDegrees(target - current);
        double minDiff = (diff > 180) ? 360 - diff : diff;

        // Find the angle which involves minimal turning
        if (wrapCurrent < 180)
        {
            if ((wrapTarget < wrapCurrent + 180) && (wrapTarget > wrapCurrent))
                finalAngle = current + minDiff;
            else
                finalAngle = current - minDiff;
        }
        else
        {
            if ((wrapTarget > wrapCurrent - 180) && (wrapTarget < wrapCurrent))
                finalAngle = current - minDiff;
            else
                finalAngle = current + minDiff;
        }

        // Check valid
        bool checkDiff = finalAngle > current ? finalAngle - current : current - finalAngle;
        bool checkDest = wrapDegrees(finalAngle) == wrapTarget;
        Log.trace("calcMinAngleDiff %s %s tgt %0.2f cur %0.2f mindiff %0.2f (%02.f) final %0.2f",
                    checkDiff ? "OK" : "DIFF_WRONG *********",
                    checkDest ? "OK" : "DEST_WRONG *********",
                    target, current, minDiff, diff, finalAngle);
        return minDiff;
    }

    static void getBestMovement(PointND& option1, PointND& option2, PointND& curRotation,
                    ROTATION_TYPE rotType, PointND& outSolution)
    {
        // Check for point near centre
        if (rotType == ROTATION_IS_NEAR_CENTRE)
        {
            // In this case leave the upper arm where it is
            outSolution._pt[0] = curRotation._pt[0];

            // Move lower arm to upper arm angle +/- 180
            double angleDiff = curRotation._pt[0] + 180 - curRotation._pt[1];
            if (angleDiff > 180)
                outSolution._pt[1] = curRotation._pt[0] - 180;
            else
                outSolution._pt[1] = curRotation._pt[0] + 180;
            Log.trace("getBestMovement 0,0 axis0Cur %0.2f axis1Cur %0.2f angleDiff %0.2fD alpha %0.2fD beta %0.2fD",
                        curRotation._pt[0], curRotation._pt[1], angleDiff,
                        outSolution._pt[0], outSolution._pt[1]);
            return;
        }

        // Option 1 and Option 2 are two solutions to the position required
        // Calculate the total angle differences for each axis in each case
        PointND newOption1, newOption2;
        double ax0Opt1Diff = calcMinAngleDiff(option1._pt[0], newOption1._pt[0], curRotation._pt[0]);
        double ax1Opt1Diff = calcMinAngleDiff(option1._pt[1], newOption1._pt[1], curRotation._pt[1]);
        double ax0Opt2Diff = calcMinAngleDiff(option2._pt[0], newOption2._pt[0], curRotation._pt[0]);
        double ax1Opt2Diff = calcMinAngleDiff(option2._pt[1], newOption2._pt[1], curRotation._pt[1]);
        Log.trace("getBestMovement ax0 cur %0.2f newTgtOpt1 %0.2f newTgtOpt2 %0.2f diff1 %0.2f diff2 %0.2fD ",
                    curRotation._pt[0], newOption1._pt[0], newOption2._pt[0], ax0Opt1Diff, ax0Opt2Diff);
        Log.trace("getBestMovement ax1 cur %0.2f newTgtOpt1 %0.2f newTgtOpt2 %0.2f diff1 %0.2f diff2 %0.2fD ",
                    curRotation._pt[1], newOption1._pt[1], newOption2._pt[1], ax1Opt1Diff, ax1Opt2Diff);

        // Weight the turning of ax0 more than ax1
        double AX0_WEIGHTING_FACTOR = 1.3;
        double opt1TotalDiff = fabs(ax0Opt1Diff) * AX0_WEIGHTING_FACTOR + fabs(ax1Opt1Diff);
        double opt2TotalDiff = fabs(ax0Opt2Diff) * AX0_WEIGHTING_FACTOR + fabs(ax1Opt2Diff);
        Log.trace("getBestMovement option1Weighted %0.2f option2Weighted %0.2f ",
                    opt1TotalDiff, opt2TotalDiff);

        // Choose the option with the least total turning
        if (opt1TotalDiff < opt2TotalDiff)
            outSolution = newOption1;
        else
            outSolution = newOption2;
    }

    static double cosineRule(double a, double b, double c)
    {
        // Calculate angle C of a triangle using the cosine rule
        // Log.trace("cosineRule a %0.2f b %0.2f c %0.2f acos %0.6f = %0.6f",
        //         a, b, c, (a*a + b*b - c*c) / (2 * a * b), acos((a*a + b*b - c*c) / (2 * a * b)));
        double val = (a*a + b*b - c*c) / (2 * a * b);
        if (val > 1) val = 1;
        if (val < -1) val = -1;
        return acos(val);
    }

    static inline double wrapRadians( double angle )
    {
        static const double twoPi = 2.0 * M_PI;
        // Log.trace("wrapRadians %0.2f %0.2f", angle, angle - twoPi * floor( angle / twoPi ));
        return angle - twoPi * floor( angle / twoPi );
    }
    static inline double wrapDegrees( double angle )
    {
        // Log.trace("wrapDegrees %0.2f %0.2f", angle, angle - 360 * floor( angle / 360 ));
        return angle - 360.0 * floor( angle / 360.0 );
    }
    static inline double r2d(double angleRadians)
    {
        // Log.trace("r2d %0.2f %0.2f", angleRadians, angleRadians * 180.0 / M_PI);
        return angleRadians * 180.0 / M_PI;
    }
    static inline double d2r(double angleDegrees)
    {
        return angleDegrees * M_PI / 180.0;
    }
    static bool isApprox(double v1, double v2, double withinRng = 0.0001)
    {
        // Log.trace("isApprox %0.6f %0.6f = %d", v1, v2, fabs(v1 - v2) < withinRng);
        return fabs(v1 - v2) < withinRng;
    }
    static bool isApproxWrap(double v1, double v2, double wrapSize=360.0, double withinRng = 0.0001)
    {
        // Log.trace("isApprox %0.6f %0.6f = %d", v1, v2, fabs(v1 - v2) < withinRng);
        double t1 = v1 - wrapSize * floor(v1 / wrapSize);
        double t2 = v2 - wrapSize * floor(v2 / wrapSize);
        return (fabs(t1 - t2) < withinRng) || (fabs(t1 - wrapSize - t2) < withinRng) || (fabs(t1 + wrapSize - t2) < withinRng);
    }

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
            Log.trace("TestPoint x %0.2f y %0.2f", newPt._pt[0], newPt._pt[1]);
            bool valid = ptToRotations(newPt, outPt1, outPt2, axisParams);
            bool outPt1Valid = ((isApproxWrap(outPt1._pt[0], checkOutPts[testIdx][0][0]) && isApproxWrap(outPt1._pt[1], checkOutPts[testIdx][0][1])) ||
                (isApproxWrap(outPt1._pt[0], checkOutPts[testIdx][1][0]) && isApproxWrap(outPt1._pt[1], checkOutPts[testIdx][1][1])));
            bool outPt2Valid = ((isApproxWrap(outPt2._pt[0], checkOutPts[testIdx][0][0]) && isApproxWrap(outPt2._pt[1], checkOutPts[testIdx][0][1])) ||
                (isApproxWrap(outPt2._pt[0], checkOutPts[testIdx][1][0]) && isApproxWrap(outPt2._pt[1], checkOutPts[testIdx][1][1])));
            if (outPt1Valid && outPt2Valid)
                Log.trace("outPts Valid");
            else
                Log.trace("outPt1 or outPt2 INVALID a1 %0.2f b1 %0.2f a2 %0.2f b2 %0.2f ****************************************************************",
                        checkOutPts[testIdx][0][0], checkOutPts[testIdx][0][1], checkOutPts[testIdx][1][0], checkOutPts[testIdx][1][1]);
            // Reverse process
            PointND actuatorCoords;
            rotationToActuator(outPt1, actuatorCoords, axisParams);
            Log.trace("Actuator ax0Steps %ld ax1Steps %ld", long(actuatorCoords._pt[0]), long(actuatorCoords._pt[1]));
            PointND rotOut;
            actuatorToRotation(actuatorCoords, rotOut, axisParams);
            bool rotOutValid = isApproxWrap(rotOut._pt[0], outPt1._pt[0]) && isApproxWrap(rotOut._pt[1], outPt1._pt[1]);
            if (rotOutValid)
                Log.trace("rotOutValid");
            else
                Log.trace("rotOut INVALID ****************************************************************");

            //
            PointND finalPoint;
            rotatonsToPoint(rotOut, finalPoint, axisParams);
            bool finalPtValid = isApprox(newPt._pt[0], finalPoint._pt[0],0.1) && isApprox(newPt._pt[1], finalPoint._pt[1],0.1);
            if (finalPtValid)
                Log.trace("finalPtValid");
            else
                Log.trace("finalPoint INVALID x %0.2f y %0.2f ****************************************************************",
                        finalPoint._pt[0], finalPoint._pt[1]);
        }
    }

    // Homing state
    typedef enum HOMING_STATE
    {
        HOMING_STATE_IDLE,
        HOMING_STATE_INIT,
        AXIS0_TO_ENDSTOP,
        AXIS0_AT_ENDSTOP,
        AXIS0_PAST_ENDSTOP,
        AXIS0_HOMED,
        AXIS1_TO_ENDSTOP,
        AXIS1_AT_ENDSTOP,
        AXIS1_PAST_ENDSTOP,
        AXIS1_HOMED
    } HOMING_STATE;
    HOMING_STATE _homingState;
    HOMING_STATE _homingStateNext;

    // Homing variables
    unsigned long _homeReqMillis;
    int _homingStepsDone;
    int _homingStepsLimit;
    bool _homingApplyStepLimit;
    unsigned long _maxHomingSecs;
    typedef enum HOMING_SEEK_TYPE { HSEEK_NONE, HSEEK_ON, HSEEK_OFF } HOMING_SEEK_TYPE;
    HOMING_SEEK_TYPE _homingSeekAxis0Endstop0;
    HOMING_SEEK_TYPE _homingSeekAxis1Endstop0;
    // the following values determine which stepper moves during the current homing stage
    typedef enum HOMING_STEP_TYPE { HSTEP_NONE, HSTEP_FORWARDS, HSTEP_BACKWARDS } HOMING_STEP_TYPE;
    HOMING_STEP_TYPE _homingAxis0Step;
    HOMING_STEP_TYPE _homingAxis1Step;
    double _timeBetweenHomingStepsUs;

    // MotionHelper for the robot motion
    MotionHelper _motionHelper;

    // Defaults
    static constexpr int homingStepTimeUs_default = 1000;
    static constexpr int maxHomingSecs_default = 1000;

public:
    RobotSandTableScara(const char* pRobotTypeName) :
        RobotBase(pRobotTypeName)
    {
        _homingState = HOMING_STATE_IDLE;
        _homeReqMillis = 0;
        _homingStepsDone = 0;
        _maxHomingSecs = maxHomingSecs_default;
        _timeBetweenHomingStepsUs = homingStepTimeUs_default;
        _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow);
    }

    // Set config
    bool init(const char* robotConfigStr)
    {
        // Info
        // Log.info("Constructing %s from %s", _robotTypeName.c_str(), robotConfigStr);

        // Init motion controller from config
        _motionHelper.setAxisParams(robotConfigStr);

        // Get params specific to this robot
        _maxHomingSecs = ConfigManager::getLong("maxHomingSecs", maxHomingSecs_default, robotConfigStr);

        // Info
        Log.info("%s maxHome %ds",
                _robotTypeName.c_str(), _maxHomingSecs);

        return true;
    }

    void home(RobotCommandArgs& args)
    {
        // Info
        Log.info("%s home x%d, y%d, z%d", _robotTypeName.c_str(), args.valid.X(), args.valid.Y(), args.valid.Z());

        // Set homing state
        homingSetNewState(HOMING_STATE_INIT);
    }

    bool canAcceptCommand()
    {
        // Check if homing
        if (_homingState != HOMING_STATE_IDLE)
            return false;

        // Check if motionHelper can accept a command
        return _motionHelper.canAcceptCommand();
    }

    void moveTo(RobotCommandArgs& args)
    {
//        Log.trace("SandTableScara moveTo");
        _motionHelper.moveTo(args);
    }

    void homingSetNewState(HOMING_STATE newState)
    {
        // Debug
        if (_homingStepsDone != 0)
            Log.trace("Changing state to %d ... HomingSteps %d", newState, _homingStepsDone);

        // Reset homing vars
        int prevHomingSteps = _homingStepsDone;
        _homingStepsDone = 0;
        _homingStepsLimit = 0;
        _homingApplyStepLimit = false;
        _homingSeekAxis0Endstop0 = HSEEK_NONE;
        _homingSeekAxis1Endstop0 = HSEEK_NONE;
        _homingAxis0Step = HSTEP_NONE;
        _homingAxis1Step = HSTEP_NONE;
        HOMING_STATE prevState = _homingState;
        _homingState = newState;

        // Handle the specfics of the new homing state
        switch(newState)
        {
            case HOMING_STATE_IDLE:
            {
                break;
            }
            case HOMING_STATE_INIT:
            {
                _homeReqMillis = millis();
                // If we are at the rotation endstop we need to move away from it first
                _homingStateNext = AXIS0_TO_ENDSTOP;
                _homingSeekAxis0Endstop0 = HSEEK_OFF;
                // Moving axis 0 only
                _homingAxis0Step = HSTEP_BACKWARDS;
                Log.info("Homing started");
                break;
            }
            case AXIS0_TO_ENDSTOP:
            {
                // Rotate to endstop
                _homingStateNext = AXIS0_AT_ENDSTOP;
                _homingSeekAxis0Endstop0 = HSEEK_ON;
                // Axis 0 only
                _homingAxis0Step = HSTEP_FORWARDS;
                Log.trace("Homing - rotating to axis 0 end stop");
                break;
            }
            case AXIS0_AT_ENDSTOP:
            {
                // Rotate past endstop to find other end of endstop range
                _homingStateNext = AXIS0_PAST_ENDSTOP;
                _homingSeekAxis0Endstop0 = HSEEK_OFF;
                // Axis 0 only
                _homingAxis0Step = HSTEP_FORWARDS;
                break;
            }
            case AXIS0_PAST_ENDSTOP:
            {
                // We now know how many steps the endstop is active for - so halve this and move
                // back that amount and we are in the centre of the home range
                int homingAxis0Centre = prevHomingSteps / 2;
                Log.trace("Homing - PastEndstop - axis0CentreSteps = %d", homingAxis0Centre);
                _homingStepsLimit = homingAxis0Centre;
                _homingApplyStepLimit = true;
                _homingStateNext = AXIS0_HOMED;
                // Axis 0 only
                _homingAxis0Step = HSTEP_BACKWARDS;
                break;
            }
            case AXIS0_HOMED:
            {
                _motionHelper.axisIsHome(0);
                // Rotate from endstop if needed
                _homingStateNext = AXIS1_TO_ENDSTOP;
                _homingSeekAxis1Endstop0 = HSEEK_OFF;
                // Axis 1 only
                _homingAxis1Step = HSTEP_BACKWARDS;
                Log.trace("Homing - rotating away from axis 1 endstop if required");
                break;
            }
            case AXIS1_TO_ENDSTOP:
            {
                // Rotate to endstop
                _homingStateNext = AXIS1_AT_ENDSTOP;
                _homingSeekAxis1Endstop0 = HSEEK_ON;
                // Axis 1 only
                _homingAxis1Step = HSTEP_FORWARDS;
                Log.trace("Homing - rotating to axis 1 endstop");
                break;
            }
            case AXIS1_AT_ENDSTOP:
            {
                // Rotate past endstop to find other end of endstop range
                _homingStateNext = AXIS1_PAST_ENDSTOP;
                _homingSeekAxis1Endstop0 = HSEEK_OFF;
                // Axis 1 only
                _homingAxis1Step = HSTEP_FORWARDS;
                break;
            }
            case AXIS1_PAST_ENDSTOP:
            {
                // We now know how many steps the endstop is active for - so halve this and move
                // back that amount and we are in the centre of the home range
                int homingAxis1Centre = prevHomingSteps / 2 + _motionHelper.getHomeOffsetSteps(1);
                Log.trace("Homing - PastEndstop - axis1CentreSteps = %d (offset is %ld)",
                                        homingAxis1Centre, _motionHelper.getHomeOffsetSteps(1));
                _homingStepsLimit = homingAxis1Centre;
                _homingApplyStepLimit = true;
                _homingStateNext = AXIS1_HOMED;
                // Axis 1 only
                _homingAxis1Step = HSTEP_BACKWARDS;
                break;
            }
            case AXIS1_HOMED:
            {
                _motionHelper.axisIsHome(1);
                _homingState = HOMING_STATE_IDLE;
                Log.info("Homing - complete");
                break;
            }
        }
    }

    bool homingService()
    {
        // Check for idle
        if (_homingState == HOMING_STATE_IDLE)
            return false;

        // Check for timeout
        if (millis() > _homeReqMillis + (_maxHomingSecs * 1000))
        {
            Log.info("Homing Timed Out");
            homingSetNewState(HOMING_STATE_IDLE);
        }

        // Check for endstop if seeking them
        bool endstop0Val = _motionHelper.isAtEndStop(0,0);
        bool endstop1Val = _motionHelper.isAtEndStop(1,0);
        if (((_homingSeekAxis0Endstop0 == HSEEK_ON) && endstop0Val) || ((_homingSeekAxis0Endstop0 == HSEEK_OFF) && !endstop0Val))
        {
            homingSetNewState(_homingStateNext);
        }
        if (((_homingSeekAxis1Endstop0 == HSEEK_ON) && endstop1Val) || ((_homingSeekAxis1Endstop0 == HSEEK_OFF) && !endstop1Val))
        {
            homingSetNewState(_homingStateNext);
        }

        // Check if we are ready for the next step
        unsigned long lastStepMicros = _motionHelper.getAxisLastStepMicros(0);
        unsigned long lastStepMicros1 = _motionHelper.getAxisLastStepMicros(1);
        if (lastStepMicros < lastStepMicros1)
            lastStepMicros = lastStepMicros1;
        if (Utils::isTimeout(micros(), lastStepMicros, _timeBetweenHomingStepsUs))
        {
            // Axis 0
            if (_homingAxis0Step != HSTEP_NONE)
                _motionHelper.step(0, _homingAxis0Step == HSTEP_FORWARDS);

            // Axis 1
            if (_homingAxis1Step != HSTEP_NONE)
                _motionHelper.step(1, _homingAxis1Step == HSTEP_FORWARDS);

            // Count homing steps in this stage
            _homingStepsDone++;

            // Check for step limit in this stage
            if (_homingApplyStepLimit && (_homingStepsDone >= _homingStepsLimit))
                homingSetNewState(_homingStateNext);

            // // Debug
            // if (_homingStepsDone % 250 == 0)
            // {
            //     const char* pStr = "ROT_ACW";
            //     if (_homingAxis0Step == HSTEP_NONE)
            //     {
            //         if (_homingAxis1Step == HSTEP_NONE)
            //             pStr = "NONE";
            //         else if (_homingAxis1Step == HSTEP_FORWARDS)
            //             pStr = "LIN_FOR";
            //         else
            //             pStr = "LIN_BAK";
            //     }
            //     else if (_homingAxis0Step == HSTEP_FORWARDS)
            //     {
            //         pStr = "ROT_CW";
            //     }
            //     Log.trace("HomingSteps %d %s", _homingStepsDone, pStr);
            // }
        }

        // // Check for linear endstop if seeking it
        // if (_homingState == LINEAR_TO_ENDSTOP)
        // {
        //     if (_motionHelper.isAtEndStop(1,0))
        //     {
        //         _homingState = OFFSET_TO_CENTRE;
        //         _homingStepsLimit = _homingCentreOffsetMM * _motionHelper.getAxisParams(1)._stepsPerUnit;
        //         _homingStepsDone = 0;
        //         Log.info("Homing - %d steps to centre", _homingStepsLimit);
        //         break;
        //     }
        // }
        return true;

    }

    void service()
    {
        // Service homing activity
        bool homingActive = homingService();

        // Service the motion controller
        _motionHelper.service(!homingActive);
    }

};
