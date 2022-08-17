#include <Arduino.h>
#include "homing.h"

#ifdef ENDSTOP_INVERTED
  #define ENDSTOP_TRIGGER 0
#else
  #define ENDSTOP_TRIGGER 1
#endif

Homing::Homing() {
    return;
}

void Homing::Setup() {
  Serial.print("Setting up Endstop");
  pinMode(ENDSTOP,INPUT);
  Serial.print("...Homing");
  GoHome();
  Serial.print(".");
  _makeMove(-1000); // back off roughly a minute to approach slower
  Serial.print(".");
  GoHome(true);   // slow approach endstop to avoid momentum of system
  Serial.println(".Homed!");
}

void Homing::GoHome(bool slow_approach) {
  //Serial.println("Running Home Process");
  //int endstop_read = 1;
  if (slow_approach) {
    stepper.setSpeed(HOMING_SLOW);
  } else {
    stepper.setSpeed(HOMING_FAST);
  }

  while (!IsTriggered()) {
    stepper.runSpeed();
    yield();
  }

  stepper.setCurrentPosition(0);
  stepper.disableOutputs();

  _isHomed = true;
}

bool Homing::IsTriggered() {
  if (digitalRead(ENDSTOP) == ENDSTOP_TRIGGER) {
    return true;
  } else {
    return false;
  }
}

bool Homing::IsHomed() {
  return _isHomed;
}

void Homing::_makeMove(int steps) {
  stepper.move(steps); // roughly half a minute backwards
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    yield();
  }
}