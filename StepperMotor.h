// RBotFirmware
// Rob Dobson 2016

#pragma once

class StepperMotor
{
public:
    // MOTOR_TYPE_DRIVER has an A4988 or similar stepper driver chip that just requires step and direction
    typedef enum
    {
        MOTOR_TYPE_NONE,
        MOTOR_TYPE_DRIVER
    } MOTOR_TYPE;
    MOTOR_TYPE _motorType;

private:
    // Motor operates in reverse (i.e. direction low = forwards)
    bool _motorDirectionReversed;

    // Minimum width of stepping pulse
    int _minPulseWidthUs;

    // Pins for the motor
    int _pin1Step;
    int _pin2Direction;

public:
    // For MOTOR_TYPE_DRIVER pin1 is step and pin2 is direction
    StepperMotor(MOTOR_TYPE motorType, uint8_t pin1Step, uint8_t pin2Direction)
    {
        if (motorType == MOTOR_TYPE_DRIVER)
        {
            if (pin1Step != -1 && pin2Direction != -1)
            {
                _motorType = motorType;
                _motorDirectionReversed = false;
                _minPulseWidthUs = 1;
                // Setup the pins
                pinMode(pin1Step, OUTPUT);
                _pin1Step = pin1Step;
                pinMode(pin2Direction, OUTPUT);
                _pin2Direction = pin2Direction;
            }
        }
        else
        {
            motorType = MOTOR_TYPE_NONE;
        }
    }

    ~StepperMotor()
    {
        deinit();
    }

    void deinit()
    {
        _motorType = MOTOR_TYPE_NONE;
        // Free up pins
        if (_pin1Step != -1)
            pinMode(_pin1Step, INPUT);
        if (_pin2Direction != -1)
            pinMode(_pin2Direction, INPUT);
    }

    void step(bool direction)
    {
        // Set direction
        digitalWrite(_pin2Direction, _motorDirectionReversed ? direction : !direction);

        // Step
        digitalWrite(_pin1Step, true);
        delayMicroseconds(_minPulseWidthUs);
        digitalWrite(_pin1Step, false);
    }

};
