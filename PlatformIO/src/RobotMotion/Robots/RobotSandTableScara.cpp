// RBotFirmware
// Rob Dobson 2016-18

#include <Arduino.h>
#include "RobotSandTableScara.h"
#include "../MotionControl/MotionHelper.h"
#include "Utils.h"
#include "math.h"

static const char* MODULE_PREFIX = "SandTableScara: ";

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
    _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow, convertCoords);

    // Light
    pinMode(A0, OUTPUT);
    digitalWrite(A0, 1);
}

RobotSandTableScara::~RobotSandTableScara()
{
    pinMode(A0, INPUT);
}

// Convert a cartesian point to actuator coordinates
bool RobotSandTableScara::ptToActuator(AxisFloats& targetPt, AxisFloats& outActuator, 
            AxisPosition& curAxisPositions, AxesParams& axesParams, bool allowOutOfBounds)
{
    // Convert the current position to polar wrapped 0..360 degrees
    AxisFloats curPolar;
    stepsToPolar(curAxisPositions._stepsFromHome, curPolar, axesParams);

    // Best relative polar solution
    AxisFloats relativePolarSolution;

	// Check for points close to the origin
	if (AxisUtils::isApprox(targetPt._pt[0], 0, 1) && (AxisUtils::isApprox(targetPt._pt[1], 0, 1)))
	{
		// Special case
		// Log.trace("%sptToActuator x %F y %F close to origin\n", MODULE_PREFIX, targetPt._pt[0], targetPt._pt[1]);

		// Keep the current position for alpha, set beta to alpha+180 (i.e. doubled-back so end-effector is in centre)
        relativePolarSolution.setVal(0, 0);
        relativePolarSolution.setVal(1, calcRelativePolar(curPolar.getVal(0) + 180, curPolar.getVal(1)));
	}
    else
    {
        // Convert the target cartesian coords to polar wrapped to 0..360 degrees
        AxisFloats soln1, soln2;
        bool isValid = cartesianToPolar(targetPt, soln1, soln2, axesParams);
        if ((!isValid) && (!allowOutOfBounds))
        {
            Log.verbose("%sOut of bounds not allowed\n", MODULE_PREFIX);
            return false;
        }

        // Find the minimum rotation for each motor
        float a1Rel = calcRelativePolar(soln1.getVal(0), curPolar.getVal(0));
        float b1Rel = calcRelativePolar(soln1.getVal(1), curPolar.getVal(1));
        float a2Rel = calcRelativePolar(soln2.getVal(0), curPolar.getVal(0));
        float b2Rel = calcRelativePolar(soln2.getVal(1), curPolar.getVal(1));

        // Which solution involves least overall rotation
        if (abs(a1Rel) + abs(b1Rel) <= abs(a2Rel) + abs(b2Rel))
        {
            relativePolarSolution.setVal(0, a1Rel);
            relativePolarSolution.setVal(1, b1Rel);
        }
        else
        {
            relativePolarSolution.setVal(0, a2Rel);
            relativePolarSolution.setVal(1, b2Rel);
        }
    }

    // Debug
	// Log.trace("%srelativePolarSolution minRot1 %F, minRot2 %F\n", MODULE_PREFIX,
	// 	relativePolarSolution.getVal(0), relativePolarSolution.getVal(1));

    // Apply this to calculate required steps
    relativePolarToSteps(relativePolarSolution, curAxisPositions, outActuator, axesParams);

    // Debug
    // Log.trace("%sTo x %F y %F dist %F abs steps %F, %F\n", MODULE_PREFIX, 
    //             targetPt._pt[0], targetPt._pt[1], sqrt(targetPt._pt[0]*targetPt._pt[0]+targetPt._pt[1]*targetPt._pt[1]),
    //             outActuator.getVal(0), outActuator.getVal(1));

    return true;
}

void RobotSandTableScara::actuatorToPt(AxisFloats& actuatorPos, AxisFloats& outPt, AxisPosition& curPos, AxesParams& axesParams)
{
    // Not currently used so unimplemented
}

void RobotSandTableScara::correctStepOverflow(AxisPosition& curPos, AxesParams& axesParams)
{
    // Since the robot is polar each stepper can be considered to have a value between
    // 0 and the stepsPerRot
    int32_t stepsPerRot0 = int32_t(roundf(axesParams.getStepsPerRot(0)));
    curPos._stepsFromHome.setVal(0, (curPos._stepsFromHome.getVal(0) + stepsPerRot0) % stepsPerRot0);
    int32_t stepsPerRot1 = int32_t(roundf(axesParams.getStepsPerRot(1)));
    curPos._stepsFromHome.setVal(1, (curPos._stepsFromHome.getVal(1) + stepsPerRot1) % stepsPerRot1);
}

bool RobotSandTableScara::cartesianToPolar(AxisFloats& targetPt, AxisFloats& targetSoln1, 
                    AxisFloats& targetSoln2, AxesParams& axesParams)
{
	// Calculate arm lengths
	// The maxVal for axis0 and axis1 are used to determine the arm lengths
	// The radius of the machine is the sum of these two lengths
	float shoulderElbowMM = 0, elbowHandMM = 0;
	bool axis0MaxValid = axesParams.getMaxVal(0, shoulderElbowMM);
	bool axis1MaxValid = axesParams.getMaxVal(1, elbowHandMM);
    // If not valid set to some values to avoid arithmetic errors
	if (!axis0MaxValid)
		shoulderElbowMM = 100;
	if (!axis1MaxValid)
		elbowHandMM = 100;

	// Calculate distance from origin to pt (forms one side of triangle where arm segments form other sides)
	float thirdSideL3MM = sqrt(pow(targetPt._pt[0], 2) + pow(targetPt._pt[1], 2));

	// Check validity of position
	bool posValid = thirdSideL3MM <= shoulderElbowMM + elbowHandMM;

	// Calculate angle from North to the point (note in atan2 X and Y are flipped from normal as angles are clockwise)
	float delta1 = atan2(targetPt._pt[0], targetPt._pt[1]);
	if (delta1 < 0)
		delta1 += M_PI * 2;

	// Calculate angle of triangle opposite elbow-hand side
	float delta2 = AxisUtils::cosineRule(thirdSideL3MM, shoulderElbowMM, elbowHandMM);

	// Calculate angle of triangle opposite third side
	float innerAngleOppThirdGamma = AxisUtils::cosineRule(shoulderElbowMM, elbowHandMM, thirdSideL3MM);

	// The two pairs of angles that solve these equations
	// alpha is the angle from shoulder to elbow
	// beta is angle from elbow to hand
	float alpha1rads = delta1 - delta2;
	float beta1rads = alpha1rads - innerAngleOppThirdGamma + M_PI;
	float alpha2rads = delta1 + delta2;
	float beta2rads = alpha2rads + innerAngleOppThirdGamma - M_PI;

	// Calculate the alpha and beta angles in degrees
	targetSoln1.setVal(0, AxisUtils::r2d(AxisUtils::wrapRadians(alpha1rads + 2 * M_PI)));
	targetSoln1.setVal(1, AxisUtils::r2d(AxisUtils::wrapRadians(beta1rads + 2 * M_PI)));
	targetSoln2.setVal(0, AxisUtils::r2d(AxisUtils::wrapRadians(alpha2rads + 2 * M_PI)));
	targetSoln2.setVal(1, AxisUtils::r2d(AxisUtils::wrapRadians(beta2rads + 2 * M_PI)));

    // Log.trace("%scartesianToPolar target X%F Y%F l1 %F, l2 %F\n", MODULE_PREFIX,
    //                 targetPt.getVal(0), targetPt.getVal(1),
    //                  shoulderElbowMM, elbowHandMM);	
    // Log.trace("%scartesianToPolar %s 3rdSide %Fmm D1 %Fd D2 %Fd innerAng %Fd\n", MODULE_PREFIX,
	// 	posValid ? "ok" : "OUT_OF_BOUNDS",
	// 	thirdSideL3MM, AxisUtils::r2d(delta1), AxisUtils::r2d(delta2), AxisUtils::r2d(innerAngleOppThirdGamma));
	// Log.trace("%scartesianToActuator alpha1 %Fd, beta1 %Fd\n", MODULE_PREFIX,
	// 	targetSoln1.getVal(0), targetSoln1.getVal(1));

    return posValid;
}

void RobotSandTableScara::stepsToPolar(AxisInt32s& actuatorCoords, AxisFloats& rotationDegrees, AxesParams& axesParams)
{
    // Axis 0 positive steps clockwise, axis 1 postive steps are anticlockwise
    // Axis 0 zero steps is at 0 degrees, axis 1 zero steps is at 180 degrees
    // All angles returned are in degrees clockwise from North
    double axis0Degrees = AxisUtils::wrapDegrees(actuatorCoords.getVal(0) * 360 / axesParams.getStepsPerRot(0));
    double axis1Degrees = AxisUtils::wrapDegrees(540 - (actuatorCoords.getVal(1) * 360 / axesParams.getStepsPerRot(1)));
    rotationDegrees.set(axis0Degrees, axis1Degrees);
    // Log.trace("%sstepsToPolar: ax0Steps %d ax1Steps %d a %Fd b %Fd\n", MODULE_PREFIX,
    //         actuatorCoords.getVal(0), actuatorCoords.getVal(1), rotationDegrees._pt[0], rotationDegrees._pt[1]);
}

float RobotSandTableScara::calcRelativePolar(float targetRotation, float curRotation)
{
    // Calculate the difference angle
    float diffAngle = targetRotation - curRotation;

    // For angles between -180 and +180 just use the diffAngle
    float bestRotation = diffAngle;
    if (diffAngle <= -180)
        bestRotation = 360 + diffAngle;
    else if (diffAngle > 180)
        bestRotation = diffAngle - 360;
    // Log.trace("%scalcRelativePolar: target %F cur %F diff %F best %F\n", MODULE_PREFIX,
    //         targetRotation, curRotation, diffAngle, bestRotation);
    return bestRotation;
}

void RobotSandTableScara::relativePolarToSteps(AxisFloats& relativePolar, AxisPosition& curAxisPositions, 
            AxisFloats& outActuator, AxesParams& axesParams)
{
    // Convert relative polar to steps
    int32_t stepsRel0 = int32_t(roundf(relativePolar.getVal(0) * axesParams.getStepsPerRot(0) / 360));
    int32_t stepsRel1 = int32_t(roundf(-relativePolar.getVal(1) * axesParams.getStepsPerRot(1) / 360));

    // Add to existing
    outActuator.setVal(0, curAxisPositions._stepsFromHome.getVal(0) + stepsRel0);
    outActuator.setVal(1, curAxisPositions._stepsFromHome.getVal(1) + stepsRel1);
    // Log.trace("%srelativePolarToSteps: stepsRel0 %d stepsRel1 %d curSteps0 %d curSteps1 %d destSteps0 %F destSteps1 %F\n",
    //         MODULE_PREFIX,
    //         stepsRel0, stepsRel1, curAxisPositions._stepsFromHome.getVal(0), curAxisPositions._stepsFromHome.getVal(1),
    //         outActuator.getVal(0), outActuator.getVal(1));
}

void RobotSandTableScara::convertCoords(RobotCommandArgs& cmdArgs, AxesParams& axesParams)
{
    // If coords are Theta-Rho
    if (cmdArgs.isThetaRho())
    {
        // Convert based on robot size
        float shoulderElbowMM = 0, elbowHandMM = 0;
        bool axis0MaxValid = axesParams.getMaxVal(0, shoulderElbowMM);
        bool axis1MaxValid = axesParams.getMaxVal(1, elbowHandMM);
        if (axis0MaxValid && axis1MaxValid)
        {
            float radius = shoulderElbowMM + elbowHandMM;
            float theta = cmdArgs.getValCoordUnits(0);
            float rho = cmdArgs.getValCoordUnits(1);
            // Calculate coords
            float xVal = sin(theta) * rho * radius;
            float yVal = cos(theta) * rho * radius;
            cmdArgs.setAxisValMM(0, xVal, true);
            cmdArgs.setAxisValMM(1, yVal, true);
            Log.verbose("%sconvertCoords theta %F rho %F -> x %F y %F\n", MODULE_PREFIX,
                        theta, rho, xVal, yVal);
        }
    }
}