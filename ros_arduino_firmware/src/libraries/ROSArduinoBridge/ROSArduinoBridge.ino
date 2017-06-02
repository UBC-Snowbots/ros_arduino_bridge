/*********************************************************************
 *  ROSArduinoBridge
 
    A set of simple serial commands to control a differential drive
    robot and receive back sensor and odometry data. Default 
    configuration assumes use of an Arduino Mega + Pololu motor
    controller shield + Robogaia Mega Encoder shield.  Edit the
    readEncoder() and setMotorSpeed() wrapper functions if using 
    different motor controller or encoder method.

    Created for the Pi Robot Project: http://www.pirobot.org
    and the Home Brew Robotics Club (HBRC): http://hbrobotics.org
    
    Authors: Patrick Goebel, James Nugen

    Inspired and modeled after the ArbotiX driver by Michael Ferguson
    
    Software License Agreement (BSD License)

    Copyright (c) 2012, Patrick Goebel.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
       disclaimer in the documentation and/or other materials provided
       with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#define USE_BASE      // Enable the base controller code
//#undef USE_BASE     // Disable the base controller code

/* Define the motor controller and encoder library you are using */
#ifdef USE_BASE
   /* The Pololu VNH5019 dual motor driver shield */
   //#define POLOLU_VNH5019

   /* The Pololu MC33926 dual motor driver shield */
   //#define POLOLU_MC33926

   /* The RoboGaia encoder shield */
   //#define ROBOGAIA
   
   /* Motors directly attached to the Arduino board */
   #define MOTORS_DIRECTLY_ATTACHED
   
   /* Encoders directly attached to Arduino board */
   #define ARDUINO_ENC_COUNTER
#endif

//#define USE_SERVOS  // Enable use of PWM servos as defined in servos.h
#undef USE_SERVOS     // Disable use of PWM servos

/* Serial port baud rate */
#define BAUDRATE  57600

/* Maximum PWM signal */
#define MAX_PWM        90
#define STOP            0

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

/* Used to read encoders (added by UBC Snowbots) */
#include "digitalWriteFast_verA.h"
#include "Arduino.h"


/* Include definition of serial commands */
#include "commands.h"

/* Sensor functions */
#include "sensors.h"

/* Include servo support if required */
#ifdef USE_SERVOS
   #include <Servo.h>
   #include "servos.h"
#endif

/* Include SoftwareSerial if required */
#ifdef MOTORS_DIRECTLY_ATTACHED
  #include <SoftwareSerial.h>
#endif

#ifdef USE_BASE
  /* Motor driver function definitions */
  #include "motor_driver.h"

  /* Encoder driver function definitions */
  #include "encoder_driver.h"

  /* PID parameters and functions */
  #include "diff_controller.h"

  /* Run the PID loop at 30 times per second */
  #define PID_RATE           30     // Hz

  /* Convert the rate into an interval */
  const int PID_INTERVAL = 1000 / PID_RATE;
  
  /* Track the next time we make a PID calculation */
  unsigned long nextPID = PID_INTERVAL;

  /* Stop the robot if it hasn't received a movement command
   in this number of milliseconds */
  #define AUTO_STOP_INTERVAL 2000
  long lastMotorCommand = AUTO_STOP_INTERVAL;
#endif

#include <Wire.h>

/* Variable initialization */

// A pair of varibles to help parse serial commands (thanks Fergs)
int arg = 0;
int index = 0;

// Variable to hold an input character
char chr;

// Variable to hold the current single-character command
char cmd;

// Character arrays to hold the first and second arguments
char argv1[16];
char argv2[16];

// The arguments converted to integers
long arg1;
long arg2;


// RC_Slave Arduino setup

// If pin 6 is HIGH, we take input from Serial (Laptop)
// If pin 6 is LOW, we take input from I2C from Wire (RC_Slave)
//    In this case, we send it a request character, and use that to send motor commands
const int RC_PIN = 6;

// constants won't change. Used here to set a pin number :
const int ledPin =  13;// the number of the LED pin

// Variables will change :
int ledState = LOW;             // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change :
const long BLINK_INTERVAL = 1000;           // interval at which to blink (milliseconds)

// Size of payload being sent from the RC_Slave Arduino
const int BUFFER_SIZE = 10;


enum robot_state {
  autonomous,
  controlled
};

robot_state current_state;

/* Clear the current command parameters */
void resetCommand() {
  cmd = NULL;
  memset(argv1, 0, sizeof(argv1));
  memset(argv2, 0, sizeof(argv2));
  arg1 = 0;
  arg2 = 0;
  arg = 0;
  index = 0;
}

/* Run a command.  Commands are defined in commands.h */
int runCommand() {
  int i = 0;
  char *p = argv1;
  char *str;
  int pid_args[4];
  arg1 = atoi(argv1);
  arg2 = atoi(argv2);
  Serial.print("arg1: ");Serial.print(argv1);Serial.print("| arg2: ");Serial.println(argv2);
  switch(cmd) {
  case GET_BAUDRATE:
    Serial.println(BAUDRATE);
    break;
  case ANALOG_READ:
    Serial.println(analogRead(arg1));
    break;
  case DIGITAL_READ:
    Serial.println(digitalRead(arg1));
    break;
  case ANALOG_WRITE:
    analogWrite(arg1, arg2);
    Serial.println("OK");
    break;
  case DIGITAL_WRITE:
    if (arg2 == 0) digitalWrite(arg1, LOW);
    else if (arg2 == 1) digitalWrite(arg1, HIGH);
    Serial.println("OK"); 
    break;
  case PIN_MODE:
    if (arg2 == 0) pinMode(arg1, INPUT);
    else if (arg2 == 1) pinMode(arg1, OUTPUT);
    Serial.println("OK");
    break;
  case PING:
    Serial.println(Ping(arg1));
    break;
#ifdef USE_SERVOS
  case SERVO_WRITE:
    servos[arg1].setTargetPosition(arg2);
    Serial.println("OK");
    break;
  case SERVO_READ:
    Serial.println(servos[arg1].getServo().read());
    break;
#endif
    
#ifdef USE_BASE
  case READ_ENCODERS:
    Serial.print(readEncoder(LEFT));
    Serial.print(" ");
    Serial.println(readEncoder(RIGHT));
    break;
   case RESET_ENCODERS:
    resetEncoders();
    resetPID();
    Serial.println("OK");
    break;
  case MOTOR_SPEEDS:
    // Reset the auto stop timer 
    lastMotorCommand = millis();
    if (arg1 == 0 && arg2 == 0) {
      setMotorSpeeds(0, 0);
      resetPID();
      moving = 0;
    }
    else moving = 1;
    leftPID.TargetTicksPerFrame = arg1;
    rightPID.TargetTicksPerFrame = arg2;
    Serial.println("OK"); 
    break;
  case UPDATE_PID:
    while ((str = strtok_r(p, ":", &p)) != '\0') {
       pid_args[i] = atoi(str);
       i++;
    }
    
    Kp = pid_args[0];
    Kd = pid_args[1];
    Ki = pid_args[2];
    Ko = pid_args[3];
    Serial.print(Kp);Serial.print(" ");Serial.print(Kd);Serial.print(" ");Serial.println(Ko);
    Serial.println("OK");
    break;
  case READ_PID_OUTPUT:
    // Get the PWM commands produced by the PID controller 
    // (diff_controller)
    Serial.print(leftPID.PTerm);Serial.print(" ");
    Serial.print(leftPID.ITerm);Serial.print(" ");
    Serial.print(leftPID.DTerm);Serial.print(" ");
    Serial.print(leftPID.output);
    Serial.print(" ");
    Serial.print(rightPID.PTerm);Serial.print(" ");
    Serial.print(rightPID.ITerm);Serial.print(" ");
    Serial.print(rightPID.DTerm);Serial.print(" ");
    Serial.println(rightPID.output);
    break;
#endif
  default:
    Serial.println(cmd);
    Serial.println("Invalid Command");
    break;
  }
}

/* Setup function--runs once at startup. */
void setup() {
  Serial.begin(BAUDRATE);
  pinMode(ledPin, OUTPUT);
// Initialize the motor controller if used */
#ifdef USE_BASE
#ifdef ARDUINO_ENC_COUNTER
  // Left encoder
  pinMode(c_LeftEncoderPinA, INPUT);      // sets pin A as input
  digitalWrite(c_LeftEncoderPinA, LOW);  // turn on pullup resistors
  pinMode(c_LeftEncoderPinB, INPUT);      // sets pin B as input
  digitalWrite(c_LeftEncoderPinB, LOW);  // turn on pullup resistors
  attachInterrupt(c_LeftEncoderInterrupt, HandleLeftMotorInterruptA, RISING);
  
  // Right encoder
  pinMode(c_RightEncoderPinA, INPUT);      // sets pin A as input
  digitalWrite(c_RightEncoderPinA, LOW);  // turn on pullup resistors
  pinMode(c_RightEncoderPinB, INPUT);      // sets pin B as input
  digitalWrite(c_RightEncoderPinB, LOW);  // turn on pullup resistors
  attachInterrupt(c_RightEncoderInterrupt, HandleRightMotorInterruptA, RISING);
#endif
initMotorController();
resetPID();
#endif

/* Attach servos if used */
#ifdef USE_SERVOS
  int i;
  for (i = 0; i < N_SERVOS; i++) {
    servos[i].initServo(
        servoPins[i],
        stepDelay[i],
        servoInitPosition[i]);
  }
#endif

  // Setting up the I2C with slave
  Wire.begin();
  TWBR = 12;
  // I2C/USB Serial triggers
  pinMode(RC_PIN, INPUT);

}

/* Enter the main loop.  Read and parse input from the serial port
  and run any valid commands. Run a PID calculation at the target
  interval and check for auto-stop conditions.
*/
void loop() {

  // Sets the robot state depending on RC_Slave input pin
  getRobotState();
  if (current_state == autonomous) {
    
    // check to see if it's time to blink the LED; that is, if the
    // difference between the current time and last time you blinked
    // the LED is bigger than the interval at which you want to
    // blink the LED.
    unsigned long currentMillis = millis();
  
    if (currentMillis - previousMillis >= BLINK_INTERVAL) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
  
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
  
      // set the LED with the ledState of the variable:
      digitalWrite(ledPin, ledState);
    }
    
    while (Serial.available() > 0) {
      // Read the next character
      chr = Serial.read();
      // Terminate a command with a CR
      if (chr == 13) {
        if (arg == 1) argv1[index] = NULL;
        else if (arg == 2) argv2[index] = NULL;
        runCommand();
        resetCommand();
      }
      // Use spaces to delimit parts of the command
      else if (chr == ' ') {
        // Step through the arguments
        if (arg == 0) arg = 1;
        else if (arg == 1)  {
          argv1[index] = NULL;
          arg = 2;
          index = 0;
        }
        continue;
      }
      else {
        if (arg == 0) {
          // The first arg is the single-letter command
          cmd = chr;
        }
        else if (arg == 1) {
          // Subsequent arguments can be more than one character
          argv1[index] = chr;
          index++;
        }
        else if (arg == 2) {
          argv2[index] = chr;
          index++;
        }
      }
    }
  } else {
    
    ledState = HIGH;
    digitalWrite(ledPin, ledState);
    
    if (Wire.requestFrom(1, BUFFER_SIZE) == BUFFER_SIZE) {
        int i = 0;
        for (; i < BUFFER_SIZE; i++){
          
          // ---- CHARACTER DEBUG BLOCK ----
          /*
          char ch = Wire.read();
          if (ch == '\r') {
            // "Flush" the buffer
            while (Wire.available() > 0) {
              Wire.read();
            }
            Serial.println("");
            delay(20);
            break;
          } else {
            Serial.print(ch);
          }
          */
          // ---- CHARACTER DEBUG BLOCK ----
          
          // ---- MAIN FUNCTIONAL BLOCK ----
          // Read the next character
          chr = Wire.read();
          // Terminate a command with a CR
          if (chr == 13) {
            if (arg == 1) argv1[index] = NULL;
            else if (arg == 2) argv2[index] = NULL;
            while (Wire.available() > 0) {
              Wire.read();
              i++;
            }
            runCommand();
            resetCommand();
          }
          // Use spaces to delimit parts of the command
          else if (chr == ' ') {
            // Step through the arguments
            if (arg == 0) arg = 1;
            else if (arg == 1)  {
              argv1[index] = NULL;
              arg = 2;
              index = 0;
            }
            continue;
          }
          else {
            if (arg == 0) {
              // The first arg is the single-letter command
              cmd = chr;
            }
            else if (arg == 1) {
              // Subsequent arguments can be more than one character
              argv1[index] = chr;
              index++;
            }
            else if (arg == 2) {
              argv2[index] = chr;
              index++;
            }
          }
          
          // ---- MAIN FUNCTIONAL BLOCK ---- 

        }
      } else {
        Serial.println("ERROR: Unexpected buffer size");
      }
  }

// If we are using base control, run a PID calculation at the appropriate intervals
#ifdef USE_BASE
  if (millis() > nextPID) {
    updatePID();
    nextPID += PID_INTERVAL;
  }
  
  // Check to see if we have exceeded the auto-stop interval
  if ((millis() - lastMotorCommand) > AUTO_STOP_INTERVAL) {;
    setMotorSpeeds(0, 0);
    moving = 0;
  }
#endif

// Sweep servos
#ifdef USE_SERVOS
  int i;
  for (i = 0; i < N_SERVOS; i++) {
    servos[i].doSweep();
  }
#endif
}

void getRobotState() {
  if (digitalRead(RC_PIN))
    current_state = autonomous;
  else
    current_state = controlled;
}
