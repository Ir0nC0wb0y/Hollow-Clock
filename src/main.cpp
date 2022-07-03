#include <Arduino.h>
#include <WiFiManager.h>
#include <NTP.h>
#include <AccelStepper.h>
#include <Button2.h>

// Use this for Serial communication, comment for silent operation
#define VERBOSE

// Please tune the following value if the clock gains or loses.
// Theoretically, standard of this value is 60000.
#define MILLIS_PER_MIN 60000 // milliseconds per a minute

// Motor and clock parameters
// 4096 * 110 / 8 = 56320
#define STEPS_PER_ROTATION        56320 // steps for a full turn of minute rotor
#define STEPS_PER_MINUTE          STEPS_PER_ROTATION / 60
#define STEPPER_SPEED             400   // steps/s
#define STEPPER_ACCEL             400   // steps/s2

// wait for a single step of stepper
int delaytime = 1;

// ports used to control the stepper motor
// if your motor rotate to the opposite direction, 
// change the order as {7, 6, 5, 4};
//int port[4] = {14, 12, 13, 15};
#define HALFSTEP 8
#define FULLSTEP 4
AccelStepper stepper(HALFSTEP, D5, D7, D6, D8);

// Buttons
#define BUTTON1      D3     // forward
#define BUTTON2      D2     // Reverse
Button2 button_fwd;
Button2 button_rvs;

/*
// sequence of stepper motor control

int seq[8][4] = {
  {  LOW, HIGH, HIGH,  LOW},
  {  LOW,  LOW, HIGH,  LOW},
  {  LOW,  LOW, HIGH, HIGH},
  {  LOW,  LOW,  LOW, HIGH},
  { HIGH,  LOW,  LOW, HIGH},
  { HIGH,  LOW,  LOW,  LOW},
  { HIGH, HIGH,  LOW,  LOW},
  {  LOW, HIGH,  LOW,  LOW}
};


void rotate(int step) {
  static int phase = 0;
  int i, j;
  int delta = (step > 0) ? 1 : 7;
  int dt = 20;

  step = (step > 0) ? step : -step;
  for(j = 0; j < step; j++) {
    phase = (phase + delta) % 8;
    for(i = 0; i < 4; i++) {
      digitalWrite(port[i], seq[phase][i]);
    }
    delay(dt);
    if(dt > delaytime) dt--;
  }
  // power cut
  for(i = 0; i < 4; i++) {
    digitalWrite(port[i], LOW);
  }
}
*/

void makeMove(int steps) {
  #ifdef VERBOSE
    Serial.print("Moving "); Serial.print(steps); Serial.println(" steps");
  #endif
  int curPos = stepper.currentPosition();
  stepper.moveTo(curPos-20);        // back off to reduce starting load
  while (stepper.distanceToGo() > 0) {
    stepper.run();
    yield();
  }
  stepper.moveTo(curPos + steps + 20); // make move plus difference from previous
  while (stepper.distanceToGo() > 0) {
    stepper.run();
    yield();
  }
  stepper.disableOutputs();
  #ifdef VERBOSE
    Serial.print("New position: "); Serial.println(stepper.currentPosition());
  #endif
}

void JogFwd(Button2& btn) {
  // Use to jog clock forward
  Serial.println("Forward button pushed");
  stepper.move(12*STEPS_PER_ROTATION);
  stepper.setMaxSpeed(STEPPER_SPEED*2);
}

void JogRvs(Button2& btn) {
  // Use to jog clock backward
  Serial.println("Reverse button pushed");
  stepper.move(-12*STEPS_PER_ROTATION);
  stepper.setMaxSpeed(STEPPER_SPEED*2);
}

void ButtonReleased(Button2& btn) {
  stepper.setMaxSpeed(STEPPER_SPEED);
  stepper.stop();
  stepper.setCurrentPosition(0);
  stepper.disableOutputs();
}

void setup() {

  #ifdef VERBOSE
    Serial.begin(115200);
    Serial.println();
    Serial.println("Starting program");
  #endif

  #ifdef VERBOSE
    Serial.println("Setting up motor");
  #endif
  stepper.setMaxSpeed(STEPPER_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);

  #ifdef VERBOSE
    Serial.println("Setting up buttons");
  #endif
  button_fwd.begin(BUTTON1);
  button_fwd.setClickHandler(JogFwd);
  button_fwd.setReleasedHandler(ButtonReleased);
  button_rvs.begin(BUTTON2);
  button_rvs.setClickHandler(JogRvs);
  button_rvs.setReleasedHandler(ButtonReleased);
  
  #ifdef VERBOSE
    Serial.println("Moving on to main program");
  #endif
}

void loop() {
  button_fwd.loop();
  button_rvs.loop();
  if (button_fwd.isPressed() || button_rvs.isPressed()) {
    if (button_fwd.isPressed()) {
      stepper.move(STEPS_PER_MINUTE);
    }
    if (button_rvs.isPressed()) {
      stepper.move(-1*STEPS_PER_MINUTE);
    }
    stepper.run();

  } else {
    // Run based on time
    static long prev_min = 0, prev_pos = 0;
    long min;
    static long pos;
    
    min = millis() / MILLIS_PER_MIN;
    if(prev_min == min) {
      return;
    }
    prev_min = min;
    pos = STEPS_PER_MINUTE * min;
    // add one extra steps every 2 out of 3 minutes (will be easier when comparing minutes from NTP)
    makeMove(pos - prev_pos);
    prev_pos = pos;
    #ifdef VERBOSE
      Serial.println("rotated a minute");
    #endif
  }
}
