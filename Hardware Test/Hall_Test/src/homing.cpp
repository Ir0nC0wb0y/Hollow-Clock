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
  //_end_current = IsTriggered();
  Serial.print("...Homing");
  GoHome();
  Serial.print(".");
  _makeMove(-1000); // back off more than backlash to approach slower
  Serial.print(".");
  GoHome(true);   // slow approach endstop to avoid momentum of system
  Serial.println(".Homed!");
}

void Homing::GoHome(bool slow_approach) {
  //Serial.println("Running Home Process");
  if (slow_approach) {
    stepper.setSpeed(HOMING_SLOW);
  } else {
    stepper.setSpeed(HOMING_FAST);
  }

  while (!IsTriggered()) {
    stepper.runSpeed();
    yield();
  }

  if (slow_approach) {
    stepper.setCurrentPosition(ENDSTOP_OFFSET); // to make the stepper position 0 to be minute zero
    stepper.disableOutputs();
    _last_trigger = ENDSTOP_OFFSET;
    _zero_pos = 0;

    _isHomed = true;
    _homing_state = 1;
  }
}

bool Homing::IsTriggered() {
  if (digitalRead(ENDSTOP) == ENDSTOP_TRIGGER) {
    return true;
  } else {
    return false;
  }
}

int Homing::LastTrigger() {
  return _last_trigger;
}

int Homing::ZeroPos() {
  return _zero_pos;
}

bool Homing::IsHomed() {
  return _isHomed;
}

bool Homing::Handle(bool report) {
  bool end_trigger = false;

  _position_cur = stepper.currentPosition();
  _filter_last = RotationFilter.Current();
  
  if (_position_cur >= _zero_pos + _filter_last - FILTER_TOLERANCE) {
    // This only triggers between 59 and 60 when commanded to go to 60, this needs to be more robust (say, if SPOOF is 57 to 02)
    _zero_pos = (_last_trigger - ENDSTOP_OFFSET) + _filter_last;
    Serial.print("Updated Zero Position "); Serial.println(_zero_pos);
  }

  if (IsTriggered()) {
    if (!_end_current) {
      _rotation_steps = abs(_position_cur - _last_trigger);

      if (abs(_rotation_steps - _filter_last) <= FILTER_LIMIT) {
        RotationFilter.Filter(_rotation_steps); // Update filter
        _zero_pos = _position_cur - ENDSTOP_OFFSET;
        _endReport(true);
      } else {
        _endReport(false);
      }
      
      _last_trigger = _position_cur;
      _end_current = true;
      end_trigger = true;
    }
  } else {
    _end_current = false;
    // _end_current is necessary to account for the non zero width of the sensor trigger area
  }

  return end_trigger;

}

void Homing::_makeMove(int steps) {
  stepper.move(steps); // roughly half a minute backwards
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    yield();
  }
}

void Homing::_endReport(bool updated) {
  if (_home_report) {
    if (updated) {
      int filter_new = RotationFilter.Current();
      Serial.print("Filter Updated: rotation steps "); Serial.print(_rotation_steps);
      Serial.print(", new filter "); Serial.print(filter_new);
      Serial.print(", Error: "); Serial.print(_rotation_steps - _filter_last);
      Serial.print(", Filter Change: "); Serial.print(filter_new - _filter_last);
      Serial.print(", current position "); Serial.print(_position_cur);
      Serial.print(", Zero Pos "); Serial.print(_zero_pos);
      Serial.println();

    } else {
      Serial.print("Filter not updated: current position "); Serial.print(_position_cur);
      Serial.print(", cur filter "); Serial.print(_filter_last);
      Serial.print(", rotation_steps "); Serial.print(_rotation_steps);
      Serial.print(", last_trigger "); Serial.print(_last_trigger);
      Serial.print(", Zero Pos "); Serial.print(_zero_pos);
      Serial.println();

    }
  }
}

void Homing::HomeReport(bool report) {
  _home_report = report;
}