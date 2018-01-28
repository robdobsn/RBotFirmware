// RBotFirmware
// Rob Dobson 2016

#pragma once

class StepperMotor
{
private:
  // Motor type
  RobotConsts::MOTOR_TYPE _motorType;

  // Motor operates in reverse (i.e. direction low = forwards)
  bool _motorDirectionReversed;

  // Minimum width of stepping pulse
  int _minPulseWidthUs;

  // Pins for the motor
  int _pinStep;
  int _pinDirection;

  // Current step pin level
  bool _pinStepLevel;

public:
  // For MOTOR_TYPE_DRIVER two pins are used step & direction
  StepperMotor(RobotConsts::MOTOR_TYPE motorType, uint8_t pinStep, uint8_t pinDirection)
  {
    if (motorType == RobotConsts::MOTOR_TYPE_DRIVER)
    {
      if (pinStep != -1 && pinDirection != -1)
      {
        _motorType              = motorType;
        _motorDirectionReversed = false;
        _minPulseWidthUs        = 1;
        // Setup the pins
        pinMode(pinStep, OUTPUT);
        _pinStep = pinStep;
        pinMode(pinDirection, OUTPUT);
        _pinDirection = pinDirection;
        _pinStepLevel = 0;
      }
    }
    else
    {
      motorType = RobotConsts::MOTOR_TYPE_NONE;
    }
  }

  ~StepperMotor()
  {
    deinit();
  }

  void deinit()
  {
    _motorType = RobotConsts::MOTOR_TYPE_NONE;
    // Free up pins
    if (_pinStep != -1)
      pinMode(_pinStep, INPUT);
    if (_pinDirection != -1)
      pinMode(_pinDirection, INPUT);
  }

  // Set direction
  void stepDirn(bool dirn)
  {
    digitalWrite(_pinDirection, _motorDirectionReversed ? dirn : !dirn);
  }

  bool stepEnd()
  {
    if (!_pinStepLevel)
      return false;
    digitalWrite(_pinStep, false);
    _pinStepLevel = false;
    return true;
  }

  void stepStart()
  {
    digitalWrite(_pinStep, true);
    _pinStepLevel = true;
  }

  void stepSync(bool direction)
  {
    // Set direction
    digitalWrite(_pinDirection, _motorDirectionReversed ? direction : !direction);

    // Step
    digitalWrite(_pinStep, true);
    delayMicroseconds(_minPulseWidthUs);
    digitalWrite(_pinStep, false);
  }

  void getPins(int& stepPin, int& dirnPin, bool& dirnReverse)
  {
    stepPin     = _pinStep;
    dirnPin     = _pinDirection;
    dirnReverse = _motorDirectionReversed;
  }

  RobotConsts::MOTOR_TYPE getMotorType()
  {
    return _motorType;
  }
};
