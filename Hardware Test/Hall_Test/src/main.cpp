#include <Arduino.h>
#include <AccelStepper.h>
#include <filter.h>

#include "homing.h"

// Clock
  // 4096 * 110 / 8 = 56320
  #define STEPS_PER_ROTATION        56320
  #define FILTER_WEIGHT                 5
  ExponentialFilter<int> RotationFilter(FILTER_WEIGHT,STEPS_PER_ROTATION);
  int filter_last           =     STEPS_PER_ROTATION;

// Motors
  #define STEPPER_SPEED               800   // steps/s
  #define STEPPER_ACCEL               400   // steps/s2
  #define HALFSTEP 8
  #define FULLSTEP 4
  AccelStepper stepper(HALFSTEP, D5, D7, D6, D8);

// Endstop
  Homing endstop;

// Loop
  #define LOOP_TIME                 5000
  unsigned long loop_next    =         0;

void MakeMove(int move_steps) {
  stepper.move(move_steps); // roughly half a minute backwards
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    yield();
  }
}

void RunRotation() {
  //Serial.println("Running Rotation");
  // Rotate 1 hour
  stepper.move(RotationFilter.Current());

  bool endstop_current = endstop.IsTriggered(); // this should evaluate as false
  //bool endstop_trigger = false;
  int rotation_steps = 0;
  while (stepper.distanceToGo() >= 0 ) {
    stepper.run();
    if (endstop.IsTriggered()) {
      if (endstop_current == false) {
        Serial.print("DistanceToGo: "); Serial.println(stepper.distanceToGo());
        rotation_steps = stepper.currentPosition();
        RotationFilter.Filter(rotation_steps);

        // continue rotation
        int remaining_steps = stepper.distanceToGo();
        Serial.print("DistanceToGo: "); Serial.println(remaining_steps);
        stepper.setCurrentPosition(0);
        stepper.move(remaining_steps);
        Serial.print("DistanceToGo: "); Serial.println(stepper.distanceToGo());

        endstop_current = true;
      }
    //} else if (endstop_current == true) {
    //  endstop_current = false;
    }
    yield();
  }
  
  // Reduce motor heat while inactive
  stepper.disableOutputs();
 
  // Output data
    // in format: Filter, Last Rotation, Error, Filter Change
    Serial.print("Filter "); Serial.print(RotationFilter.Current()); Serial.print(", Last Rotation "); Serial.print(rotation_steps); Serial.print(", Error "); Serial.print(RotationFilter.Current()-rotation_steps); Serial.print(", Filter Change "); Serial.println(RotationFilter.Current() - filter_last);
    filter_last = RotationFilter.Current();

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

  // Go to minute 0 (from offset)
  Serial.print("Moving to minute 0");
  MakeMove(ENDSTOP_OFFSET);
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