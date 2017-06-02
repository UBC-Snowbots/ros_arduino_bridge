/* RC Slave Firmware
   Author: Vincent Yuan,
   Modified: Valerian Ratu
   Date Last Modified: May 28, 2017
*/

#include <stdlib.h>
#include <Wire.h>

#define TRIM 2 // error margin for joysticks
#define BAUD_RATE 57600
#define TRIG 11

// RC Joystick variables & tuning
// Might be easier to tune the RC itself
const int JOYSTICK_MARGIN = 50;
const int THROTTLE_PULSE_MIN = 949;
const int THROTTLE_PULSE_MAX = 1700;
//const int THROTTLE_PULSE_REST = 1325;
const int TURN_PULSE_MIN = 949;
const int TURN_PULSE_MAX = 1700;
//const int TURN_PULSE_REST = 1325;

enum rc_pins {
    unused = 1,     // Channel 1
    throttle = 2,   // Channel 2
    mode = 3,       // Channel 3
    turn = 4        // Channel 4
};

enum robot_state {
    remote,
    stop,
    autonomous
};

robot_state current_state;


// Number of tick for PID controller
// i.e. if MAX_SPEED = 20 then: "m MAX_SPEED MAX_SPEED" = "m 20 20"
const int MAX_SPEED = 20;
const int STOP_SPEED = 0;


// buffer inputs
int linear_x = 0;
int angular_z = 0;
int prev_linear_x = 0;
int prev_angular_z = 0;

// these variables will store the joystick ranges - used to figure out direction, turn etc
int linearXHigh = 0, linearXLow = 0, angularZHigh = 0, angularZLow = 0;
int left_throttle = 0; int right_throttle = 0; 

char msg[10];
int throttle_buf[3]; int turn_buf[3];
int throttle_ind = 0; int turn_ind = 0;


void setup() {

  Serial.begin(BAUD_RATE);
  Wire.begin(1);
  TWBR = 12;
  Wire.onRequest(req_event);

  pinMode(unused, INPUT);
  pinMode(throttle, INPUT);
  pinMode(mode, INPUT);
  pinMode(turn, INPUT);
  pinMode(TRIG,OUTPUT);

  // calculate OFFSET
  set_offset();
}


void loop() {
  // read RC input
  rc_read();

  if (current_state == autonomous) { // Auto Mode
    digitalWrite(TRIG,HIGH);
  }
  else if (current_state == remote) { //RC Mode
    digitalWrite(TRIG,LOW);
  }
  else { // STOP MODE
    digitalWrite(TRIG,LOW);
  }

}

/*
* Calculates OFFSET for the joystick controllers.
*/
void set_offset() {
  int THROTTLE_PULSE_REST = 1325;
  int TURN_PULSE_REST = 1325;
  // JOYSTICK_MARGIN is an error margin for joystick control
  // e.g. if the joystick is moved just a little bit, it is assumed that no movement
  // is desired.
  linearXHigh = THROTTLE_PULSE_REST + JOYSTICK_MARGIN;
  linearXLow = THROTTLE_PULSE_REST - JOYSTICK_MARGIN;

  angularZHigh = TURN_PULSE_REST + JOYSTICK_MARGIN;
  angularZLow = TURN_PULSE_REST - JOYSTICK_MARGIN;
}

/*
* Read in from RC controller. Mode and speed are determined here.
*/
void rc_read() {
  
  int throttle_val = pulseIn(throttle, HIGH);
  int turn_val = pulseIn(turn, HIGH);
  int mode_val = pulseIn(mode, HIGH);

  throttle_buf[throttle_ind++] = throttle_val;
  turn_buf[turn_ind++] = turn_val;

  if (throttle_ind >= 3){
    throttle_ind = 0;
  }
  
  if (turn_ind >= 3){
    turn_ind = 0;
  }

  int avg_throttle = 0;
  int avg_turn = 0;

  int i = 0;
  for (;i < 3;i++){
    avg_throttle += throttle_buf[i];
    avg_turn += turn_buf[i];
  }
  
  avg_throttle = avg_throttle / 3;
  avg_turn = avg_turn / 3;


  
  // Debug RC inputs
  /*
  Serial.print("Throttle: ");Serial.print(throttle_val);
  Serial.print("\t| Turn: ");Serial.print(turn_val);
  Serial.print("\t| Mode: ");Serial.println(mode_val);
  */

  /*
  Serial.print("Avg Throttle: ");Serial.print(avg_throttle);
  Serial.print("\t| Avg Turn: ");Serial.println(avg_turn);
  */

  if (avg_throttle < linearXHigh && avg_throttle > linearXLow)
    linear_x = STOP_SPEED;
  else
    linear_x = map(avg_throttle, THROTTLE_PULSE_MIN, THROTTLE_PULSE_MAX, -MAX_SPEED, MAX_SPEED);

  if (avg_turn < angularZHigh && avg_turn > angularZLow)
    angular_z = STOP_SPEED;
  else
    angular_z = map(avg_turn, TURN_PULSE_MIN, TURN_PULSE_MAX, -MAX_SPEED/2, MAX_SPEED/2);

  // if joystick movement is not outside of the error range, assume no movement is desired
  if (abs(linear_x) < TRIM)
    linear_x = STOP_SPEED;
  if (abs(angular_z) < TRIM)
    angular_z = STOP_SPEED;
  

  if (mode_val < 1200)
    current_state = stop;
  else if (1200 <= mode_val && mode_val < 1600)
    current_state = remote;
  else if (1600 <= mode_val && mode_val < 2000)
    current_state = autonomous;
  else
    current_state = stop;

  // Debug post map() values
  /*
  Serial.print("Throttle: ");Serial.print(linear_x);
  Serial.print("\t| Turn: ");Serial.print(angular_z);
  Serial.print("\t| Mode: ");Serial.println(current_state);
  */
  
  left_throttle = linear_x + angular_z;
  right_throttle = linear_x - angular_z;
  
  if (left_throttle > MAX_SPEED) {
    left_throttle = MAX_SPEED;
  } else if (left_throttle < -MAX_SPEED) {
    left_throttle = -MAX_SPEED;
  }

  if (right_throttle > MAX_SPEED) {
    right_throttle = MAX_SPEED;
  } else if (right_throttle < -MAX_SPEED) {
    right_throttle = -MAX_SPEED;
  }
  
  Serial.print("Left Throttle: ");Serial.print(left_throttle);
  Serial.print("| Right Throttle: ");Serial.println(right_throttle);

  noInterrupts();
  sprintf(msg,"m %d %d\r", left_throttle, right_throttle);
  interrupts();
}


// moves the robot. Turning is taken into account
 void package_msg(bool stop){
  if (stop) {
    Wire.write("m 0 0\r");
    return;
  }
  Wire.write(msg);
}

void req_event(){
    if (current_state == stop ) {
        package_msg(true);
    } else {
        package_msg(false);
    }
}