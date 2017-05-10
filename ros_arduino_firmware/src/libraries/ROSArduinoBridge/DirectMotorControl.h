#ifndef DIRECTMOTORCONTROL_H
#define DIRECTMOTORCONTROL_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <stdlib.h>
#include <Servo.h>

class DirectMotorControl {
public:
    DirectMotorControl();
    DirectMotorControl(int left_motor_pin, int right_motor_pin);

    // Set the speed of the left and right motors respectively
    void setLeftSpeed(int speed);
    void setRightSpeed(int speed);

private:

    // Sets up everything
    void init(int left_motor_pin, int right_motor_pin);

    // Writes given throttle command out to given motor
    void servo_write(Servo motor, int throttle);

    // The left and right motors
    Servo left_motor, right_motor;
};

#endif // DIRECTMOTORCONTROL_H
