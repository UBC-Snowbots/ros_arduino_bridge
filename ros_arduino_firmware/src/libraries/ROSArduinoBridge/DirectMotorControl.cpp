#include "DirectMotorControl.h"

DirectMotorControl::DirectMotorControl(){
    // Set default pins for left and right motors
    int right_motor_pin = 10;
    int left_motor_pin = 11;
    init(left_motor_pin, right_motor_pin);
}

DirectMotorControl::DirectMotorControl(int left_motor_pin, int right_motor_pin){
    init(left_motor_pin, right_motor_pin);
}

void DirectMotorControl::init(int left_motor_pin, int right_motor_pin){
    left_motor.attach(left_motor_pin);
    right_motor.attach(right_motor_pin);
}

void DirectMotorControl::setLeftSpeed(int speed){
    servo_write(left_motor, speed);
}

void DirectMotorControl::setRightSpeed(int speed){
    servo_write(right_motor, speed);
}

// This maps to motor control values for the Talon Motors on Jack Frost,
// a robot under the UBC Snowbots Robotics Team
void DirectMotorControl::servo_write(Servo motor, int throttle) {
  // we are using the Talon SRX - looks like duty cycle is between 1 - 2ms
  // seen here https://www.ctr-electronics.com/Talon%20SRX%20User's%20Guide.pdf
  // throttle can be as high as 110 and as low as 70 after calculation
  // PWM input pulse high time can be between 1 and 2 ms. So 1000-2000 microseconds
  
  // note if using the Servo library to do PWM produces issues
  // alternate implementations can be found here: http://www.circuitstoday.com/pwm-generation-and-control-using-arduino 
  throttle = map(throttle, 70, 110, 1000, 2000); 
  motor.writeMicroseconds(throttle);
}


