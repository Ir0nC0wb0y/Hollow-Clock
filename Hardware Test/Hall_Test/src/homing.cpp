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
  _end_current = IsTriggered();
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


  stepper.setCurrentPosition(ENDSTOP_OFFSET);
  stepper.disableOutputs();
  _last_trigger = ENDSTOP_OFFSET;

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

void Homing::Handle() {
  
  if (IsTriggered()) {
      if (_end_current == false) {
        int rotation_cur = stepper.currentPosition();
        int filter_cur = RotationFilter.Current();
        int rotation_steps = abs(rotation_cur - _last_trigger);
        /*
        Serial.println();
        Serial.print("rotation_cur, filter_cur, rotation_steps, last_trigger: ");
        Serial.print(rotation_cur); Serial.print(", ");
        Serial.print(filter_cur); Serial.print(", ");
        Serial.print(rotation_steps); Serial.print(", ");
        Serial.println(_last_trigger);
        */

        if (abs(rotation_steps - filter_cur) <= FILTER_LIMIT) {
          RotationFilter.Filter(rotation_steps);
          int filter_new = RotationFilter.Current();
          Serial.print("Filter Updated, rotation: "); Serial.print(rotation_steps); Serial.print(", new filter: "); Serial.print(filter_new); Serial.print(", Error: "); Serial.print(filter_cur-rotation_steps); Serial.print(", Filter Change: "); Serial.println(filter_new-filter_cur);
          //stepper.setCurrentPosition(filter_new + ENDSTOP_OFFSET);
          //stepper.moveTo(filter_new);
        } else {
          Serial.println("Filter not updated: ");
          Serial.print("rotation_cur, filter_cur, rotation_steps, last_trigger: ");
          Serial.print(rotation_cur); Serial.print(", ");
          Serial.print(filter_cur); Serial.print(", ");
          Serial.print(rotation_steps); Serial.print(", ");
          Serial.println(_last_trigger);
        }
        _last_trigger = rotation_cur;
        _end_current = true;
      }
  } else if (_end_current == true) {
    _end_current = false;
  }

}

void Homing::_makeMove(int steps) {
  stepper.move(steps); // roughly half a minute backwards
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    yield();
  }
}