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
    
    // Sets up everything
    void init();

private:

    // Writes given throttle command out to given motor
    void servo_write(Servo& motor, int throttle);

    // The left and right motor pins
    int _left_motor_pin, _right_motor_pin;
    
    // The left and right motors
    Servo _left_motor, _right_motor;
};

#endif
