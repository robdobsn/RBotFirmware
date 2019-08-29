// RBotFirmware
// Rob Dobson 2016-18

#include "RobotController.h"
#include "ConfigBase.h"
#include "RdJson.h"
#include "RobotCommandArgs.h"

// Robot types
#include "Robots/RobotMugBot.h"
#include "Robots/RobotGeistBot.h"
#include "Robots/RobotHockeyBot.h"
#include "Robots/RobotSandTableScara.h"
#include "Robots/RobotXYBot.h"

RobotController::RobotController()
{
    // Init
    _pRobot = NULL;
}

RobotController::~RobotController()
{
    delete _pRobot;
}

bool RobotController::init(const char* configStr)
{
    // Init
    delete _pRobot;
    _pRobot = NULL;

    // Get the geometry
    ConfigBase robotGeom(RdJson::getString("robotGeom", "NONE", configStr).c_str());

    // Get the robot geometry from the config
    String robotModel = robotGeom.getString("model", "");
    if (robotModel.equalsIgnoreCase("MugBot"))
    {
        Log.notice("Constructing %s\n", robotModel.c_str());
        _pRobot = new RobotMugBot(robotModel.c_str(), _motionHelper);
        if (!_pRobot)
            return false;
        _pRobot->init(configStr);
    }
    else if (robotModel.equalsIgnoreCase("GeistBot"))
    {
        Log.notice("Constructing %s\n", robotModel.c_str());
        _pRobot = new RobotGeistBot(robotModel.c_str(), _motionHelper);
        if (!_pRobot)
            return false;
        _pRobot->init(configStr);
    }
    else if (robotModel.equalsIgnoreCase("SingleArmScara"))
    {
        Log.notice("Constructing %s\n", robotModel.c_str());
        _pRobot = new RobotSandTableScara(robotModel.c_str(), _motionHelper);
        if (!_pRobot)
            return false;
        _pRobot->init(configStr);
    }
    else if (robotModel.equalsIgnoreCase("HBot"))
    {
        Log.notice("Constructing %s\n", robotModel.c_str());
        _pRobot = new RobotHockeyBot(robotModel.c_str(), _motionHelper);
        if (!_pRobot)
            return false;
        _pRobot->init(configStr);
    }
    else if ((robotModel.equalsIgnoreCase("Cartesian")) || (robotModel.equalsIgnoreCase("XYBot")))
    {
        Log.notice("Constructing %s\n", robotModel.c_str());
        _pRobot = new RobotXYBot(robotModel.c_str(), _motionHelper);
        if (!_pRobot)
            return false;
        _pRobot->init(configStr);
    }
    else
    {
        Log.notice("Cannot determine robotModel %s\n", robotModel.c_str());
    }

    if (_pRobot)
    {
        _motionHelper.setIntrumentationMode(INSTRUMENT_MOTION_ACTUATOR_CONFIG);
        _pRobot->pause(false);
    }

    return true;
}

// Pause (or un-pause) all motion
void RobotController::pause(bool pauseIt)
{
    if (pauseIt)
        Log.notice("RobotController: pausing\n");
    else
        Log.notice("RobotController: resuming\n");
    if (!_pRobot)
        return;
    _pRobot->pause(pauseIt);
}

// Stop
void RobotController::stop()
{
    Log.notice("RobotController: stop\n");
    if (!_pRobot)
        return;
    _pRobot->stop();
}

// Check if paused
bool RobotController::isPaused()
{
    if (!_pRobot)
        return false;
    return _pRobot->isPaused();
}

// Service (called frequently)
void RobotController::service()
{
    if (!_pRobot)
        return;
    _pRobot->service();
}

// Movement commands
void RobotController::actuator(double value)
{
    if (!_pRobot)
        return;
    _pRobot->actuator(value);
}

// Check if the robot can accept a (motion) command
bool RobotController::canAcceptCommand()
{
    if (!_pRobot)
        return false;
    return _pRobot->canAcceptCommand();
}

void RobotController::moveTo(RobotCommandArgs& args)
{
    if (!_pRobot)
        return;
    _pRobot->moveTo(args);
}

// Set motion parameters
void RobotController::setMotionParams(RobotCommandArgs& args)
{
    if (!_pRobot)
        return;
    _pRobot->setMotionParams(args);
}

// Get status
void RobotController::getCurStatus(RobotCommandArgs& args)
{
    if (!_pRobot)
        return;
    _pRobot->getCurStatus(args);
}

// Get robot attributes
void RobotController::getRobotAttributes(String& robotAttrs)
{
    robotAttrs = "{}";
    if (!_pRobot)
        return;
    _pRobot->getRobotAttributes(robotAttrs);
}

// Go Home
void RobotController::goHome(RobotCommandArgs& args)
{
    if (!_pRobot)
        return;
    _pRobot->goHome(args);
}

// Set Home
void RobotController::setHome(RobotCommandArgs& args)
{
    if (!_pRobot)
        return;
    _pRobot->setHome(args);
}

bool RobotController::wasActiveInLastNSeconds(int nSeconds)
{
    if (!_pRobot)
        return false;
    return _pRobot->wasActiveInLastNSeconds(nSeconds);
}

String RobotController::getDebugStr()
{
    return _motionHelper.getDebugStr();
}
