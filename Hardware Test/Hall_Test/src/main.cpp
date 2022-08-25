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
  #define LOOP_TIME                  250  // Schwaz 35 seconds, changed to 5 for testing
  unsigned long loop_next    =         0;
  int loop_minute            =         0;
  //int test_rotations         =         0;
  //#define ROTATION_RESET               5  // How many full rotations until the clock resets (will be used later to reset the clock in the middle of the night)

void MakeMove(int move_steps) {
  stepper.move(move_steps); // roughly half a minute backwards
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    endstop.Handle();
    yield();
  }
  // Reduce motor heat while inactive
  stepper.disableOutputs();
}

void MakeMoveTo(int move_steps) {
  stepper.moveTo(move_steps); // roughly half a minute backwards
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    endstop.Handle();
    yield();
  }
  // Reduce motor heat while inactive
  stepper.disableOutputs();
}

void RunRotation() {
  //Serial.println("Running Rotation");
  // Rotate 1 hour
  //stepper.move(RotationFilter.Current()); // This only incorporates the updated rotation steps
  stepper.moveTo(endstop.ZeroPos() + RotationFilter.Current()); // New system to incorporate most recent (completed) endstop adjustment
  stepper.setMaxSpeed(STEPPER_SPEED);


  unsigned long while_break = 120000 + millis();
  while (stepper.distanceToGo() != 0 ) {
    stepper.run();
    endstop.Handle();
    yield();
    if (millis() >= while_break) {
      Serial.print("Breaking out of Rotation, current distance to go: "); Serial.println(stepper.distanceToGo());
      break;
    }
  }

}

void RunToMinute(int minute) {
  minute = minute % 60;
  if (minute == 0) {
    minute = 60;
  }
  int minute_steps = RotationFilter.Current() * (float(minute) / 60.0);
  stepper.moveTo(endstop.ZeroPos() + minute_steps);
  stepper.setMaxSpeed(STEPPER_SPEED);

  Serial.print("Moving to minute "); Serial.print(minute);
  Serial.print(", distance to go "); Serial.print(stepper.distanceToGo());
  Serial.print(", final position "); Serial.print(stepper.targetPosition());
  Serial.print(", zero position "); Serial.print(endstop.ZeroPos());
  Serial.println();

  unsigned long while_break = 120000 + millis();
  while (stepper.distanceToGo() != 0 ) {
    stepper.run();
    endstop.Handle();
    yield();
    if (millis() >= while_break) {
      Serial.print("Breaking out of R2M, current distance to go: "); Serial.println(stepper.distanceToGo());
      break;
    }
  }
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
    //RunRotation();

    loop_minute = loop_minute + 1;
    if (loop_minute > 60) {
      loop_minute = 1;
    }
    RunToMinute(loop_minute);

    loop_next = millis() + LOOP_TIME;
    /*if (test_rotations >= ROTATION_RESET) {
      MakeMoveTo(endstop.ZeroPos());
      stepper.setCurrentPosition(0); // Need to inform the Homing Class, may use that to update the current position, instead of doing it here
      Serial.println();
      Serial.print("Reset position after "); Serial.print(test_rotations); Serial.println(" turns");
      Serial.println();
      test_rotations = 0;
    }*/
  }
}