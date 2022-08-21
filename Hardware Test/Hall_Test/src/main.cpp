#include <Arduino.h>
#include <AccelStepper.h>
#include <filter.h>

#include "homing.h"

// Clock
  // 4096 * 110 / 8 = 56320
  #define STEPS_PER_ROTATION        56320
  #define FILTER_WEIGHT                10
  ExponentialFilter<int> RotationFilter(FILTER_WEIGHT,STEPS_PER_ROTATION);
  //int filter_last           =     STEPS_PER_ROTATION;

// Motors
  #define STEPPER_SPEED               800   // steps/s
  #define STEPPER_ACCEL               400   // steps/s2
  #define HALFSTEP 8
  #define FULLSTEP 4
  AccelStepper stepper(HALFSTEP, D5, D7, D6, D8);

// Endstop
  Homing endstop;

// Loop
  #define LOOP_TIME                35000
  unsigned long loop_next    =         0;

void MakeMove(int move_steps) {
  stepper.move(move_steps); // roughly half a minute backwards
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    endstop.Handle();
    yield();
  }
}

void MakeMoveTo(int move_steps) {
  stepper.moveTo(move_steps); // roughly half a minute backwards
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    endstop.Handle();
    yield();
  }
}

void RunRotation() {
  //Serial.println("Running Rotation");
  // Rotate 1 hour
  stepper.move(RotationFilter.Current());
  stepper.setMaxSpeed(STEPPER_SPEED);

  //bool endstop_current = endstop.IsTriggered(); // this should evaluate as false
  //bool endstop_trigger = false;
  //int rotation_steps = 0;
  unsigned long while_break = 120000 + millis();
  while (stepper.distanceToGo() != 0 ) {
    stepper.run();
    endstop.Handle();
    yield();
    if (millis() >= while_break) {
      Serial.print("Breaking out of while, current distance to go: "); Serial.println(stepper.distanceToGo());
      break;
    }
  }
  
  // Reduce motor heat while inactive
  stepper.disableOutputs();
  //stepper.setCurrentPosition(0);
 
  // Output data
    // in format: Filter, Last Rotation, Error, Filter Change
    //Serial.print("Filter "); Serial.print(RotationFilter.Current()); Serial.print(", Last Rotation "); Serial.print(rotation_steps); Serial.print(", Error "); Serial.print(RotationFilter.Current()-rotation_steps); Serial.print(", Filter Change "); Serial.println(RotationFilter.Current() - filter_last);
    //filter_last = RotationFilter.Current();

  // Backup to remove uncertainty for next rotation
    //MakeMove(-1000);
    //endstop.GoHome(true);
}

void setup() {
  // Serial start
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // Motor Setup
  Serial.println("Setting up motor");
  stepper.setMaxSpeed(STEPPER_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);

  // Endstop Setup (and home)
  endstop.Setup();
  stepper.setMaxSpeed(STEPPER_SPEED);

  // Go to minute 0
  Serial.print("Moving to minute 0");
  MakeMoveTo(0);

  Serial.println("...Done!");

  // Prepare Loop
  loop_next = millis()+ LOOP_TIME;
}

void loop() {
  // initiate process at interval
  if (millis() >= loop_next) {
    RunRotation();
    loop_next = millis() + LOOP_TIME;
  }
}