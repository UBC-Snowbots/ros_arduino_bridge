#include "DirectMotorControl.h"

DirectMotorControl::DirectMotorControl(){
    _left_motor_pin = 11;
    _right_motor_pin = 10;
}

DirectMotorControl::DirectMotorControl(int left_motor_pin, int right_motor_pin){
    _left_motor_pin = left_motor_pin;
    _right_motor_pin = right_motor_pin;
}

void DirectMotorControl::init(){
    _left_motor.attach(_left_motor_pin);
    _right_motor.attach(_right_motor_pin);
}

void DirectMotorControl::setLeftSpeed(int speed){
    servo_write(_left_motor, speed);
}


void DirectMotorControl::setRightSpeed(int speed){
    servo_write(_right_motor, speed);
}

// This maps to motor control values for the Talon Motors on Jack Frost,
// a robot under the UBC Snowbots Robotics Team
void DirectMotorControl::servo_write(Servo& motor, int throttle) {
  //Serial.println(throttle);
  // we are using the Talon SRX - looks like duty cycle is between 1 - 2ms
  // seen here https://www.ctr-electronics.com/Talon%20SRX%20User's%20Guide.pdf
  // throttle can be as high as 110 and as low as 70 after calculation
  // PWM input pulse high time can be between 1 and 2 ms. So 1000-2000 microseconds
  
  // note if using the Servo library to do PWM produces issues
  // alternate implementations can be found here: http://www.circuitstoday.com/pwm-generation-and-control-using-arduino 
  throttle = constrain(throttle, 70, 110);
  throttle = map(throttle, 70, 110, 1000, 2000); 
  motor.writeMicroseconds(throttle);
}


