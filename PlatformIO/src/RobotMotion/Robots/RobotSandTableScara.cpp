// RBotFirmware
// Rob Dobson 2016-18

#include <Arduino.h>
#include "RobotSandTableScara.h"
#include "../MotionControl/MotionHelper.h"
#include "Utils.h"
#include "math.h"

// #define DEBUG_SANDTABLESCARA_MOTION 1
// #define DEBUG_SANDTABLE_CARTESIAN_TO_POLAR 1

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
    _motionHelper.setTransforms(ptToActuator, actuatorToPt, correctStepOverflow, convertCoords, setRobotAttributes);
}

RobotSandTableScara::~RobotSandTableScara()
{
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
#ifdef DEBUG_SANDTABLESCARA_MOTION
		Log.trace("%sptToActuator x %F y %F close to origin\n", MODULE_PREFIX, targetPt._pt[0], targetPt._pt[1]);
#endif

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

    // Apply this to calculate required steps
    relativePolarToSteps(relativePolarSolution, curAxisPositions, outActuator, axesParams);

    // Debug
#ifdef DEBUG_SANDTABLESCARA_MOTION
    Log.trace("%sptToAct newX %F newY %F prevX %F prevY %F dist %F abs steps %F, %F minRot1 %F, minRot2 %F\n", MODULE_PREFIX, 
                targetPt._pt[0], 
                targetPt._pt[1], 
                curAxisPositions._axisPositionMM.getVal(0), 
                curAxisPositions._axisPositionMM.getVal(1), 
                sqrt(targetPt._pt[0]*targetPt._pt[0]+targetPt._pt[1]*targetPt._pt[1]),
                outActuator.getVal(0), 
                outActuator.getVal(1), 
                relativePolarSolution.getVal(0), 
                relativePolarSolution.getVal(1));
#endif

    return true;
}

void RobotSandTableScara::actuatorToPt(AxisInt32s& actuatorPos, AxisFloats& outPt, AxisPosition& curPos, AxesParams& axesParams)
{
    // Get current polar
    AxisFloats curPolar;
    stepsToPolar(actuatorPos, curPolar, axesParams);

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

    // Compute axis positions from polar values
    float elbowX = shoulderElbowMM * sin(AxisUtils::d2r(curPolar.getVal(0)));
    float elbowY = shoulderElbowMM * cos(AxisUtils::d2r(curPolar.getVal(0)));
    float handXDiff = elbowHandMM * sin(AxisUtils::d2r(curPolar.getVal(1)));
    float handYDiff = elbowHandMM * cos(AxisUtils::d2r(curPolar.getVal(1)));
    outPt.setVal(0, elbowX + handXDiff);
    outPt.setVal(1, elbowY + handYDiff);    

    // Debug
#ifdef DEBUG_SANDTABLESCARA_MOTION
    Log.verbose("%sacToPt s1 %d s2 %d alpha %F beta %F x %F y %F shel %F elha %F elx %F ely %F haX %F hay %F\n", MODULE_PREFIX, 
                actuatorPos.getVal(0), actuatorPos.getVal(1),
                curPolar.getVal(0), curPolar.getVal(1),
                elbowX + handXDiff, elbowY + handYDiff,
                shoulderElbowMM, elbowHandMM,
                elbowX, elbowY,
                handXDiff, handYDiff);
#endif

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

#ifdef DEBUG_SANDTABLE_CARTESIAN_TO_POLAR
    Log.trace("%scartesianToPolar target X%F Y%F l1 %F, l2 %F\n", MODULE_PREFIX,
            targetPt.getVal(0), targetPt.getVal(1),
            shoulderElbowMM, elbowHandMM);	
    Log.trace("%scartesianToPolar %s 3rdSide %Fmm D1 %Fd D2 %Fd innerAng %Fd\n", MODULE_PREFIX,
            posValid ? "ok" : "OUT_OF_BOUNDS",
            thirdSideL3MM, AxisUtils::r2d(delta1), AxisUtils::r2d(delta2), AxisUtils::r2d(innerAngleOppThirdGamma));
    Log.trace("%scartesianToActuator alpha1 %Fd, beta1 %Fd\n", MODULE_PREFIX,
		    targetSoln1.getVal(0), targetSoln1.getVal(1));
#endif

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
#ifdef DEBUG_SANDTABLE_CARTESIAN_TO_POLAR
    Log.trace("%sstepsToPolar: ax0Steps %d ax1Steps %d a %Fd b %Fd\n", MODULE_PREFIX,
            actuatorCoords.getVal(0), actuatorCoords.getVal(1), rotationDegrees._pt[0], rotationDegrees._pt[1]);
#endif
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
#ifdef DEBUG_SANDTABLE_CARTESIAN_TO_POLAR
    Log.trace("%scalcRelativePolar: target %F cur %F diff %F best %F\n", MODULE_PREFIX,
            targetRotation, curRotation, diffAngle, bestRotation);
#endif
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
#ifdef DEBUG_SANDTABLE_CARTESIAN_TO_POLAR
    Log.trace("%srelativePolarToSteps: stepsRel0 %d stepsRel1 %d curSteps0 %d curSteps1 %d destSteps0 %F destSteps1 %F\n",
            MODULE_PREFIX,
            stepsRel0, stepsRel1, curAxisPositions._stepsFromHome.getVal(0), curAxisPositions._stepsFromHome.getVal(1),
            outActuator.getVal(0), outActuator.getVal(1));
#endif
}

void RobotSandTableScara::convertCoords(RobotCommandArgs& cmdArgs, AxesParams& axesParams)
{
    // Coordinates can be converted here if required
}

// Set robot attributes
void RobotSandTableScara::setRobotAttributes(AxesParams& axesParams, String& robotAttributes)
{
    // Calculate max and min cartesian size of robot
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

    // Set attributes
    constexpr int MAX_ATTR_STR_LEN = 400;
    char attrStr[MAX_ATTR_STR_LEN];
    sprintf(attrStr, "{\"sizeX\":%0.2f,\"sizeY\":%0.2f,\"sizeZ\":%0.2f,\"originX\":%0.2f,\"originY\":%0.2f,\"originZ\":%0.2f}",
            (shoulderElbowMM+elbowHandMM)*2, (shoulderElbowMM+elbowHandMM)*2, 0.0,
            (shoulderElbowMM+elbowHandMM), (shoulderElbowMM+elbowHandMM), 0.0);
    robotAttributes = attrStr;
}
