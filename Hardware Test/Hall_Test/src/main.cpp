#include <Arduino.h>
#include <WiFiManager.h>
#include <NTP.h>
#include <AccelStepper.h>
#include <filter.h>
#include <Button2.h>

#include "homing.h"

// WiFi
  WiFiManager wm;
  #define WIFI_CHECK_INTERVAL      900000
  unsigned long wifi_check_next  =      0;

// Time Keeping (NTP)
  long utcOffsetInSeconds_DST    = -18000;
  long utcOffsetInSeconds        = -21600;
  #define NTP_UPDATE_INT           900000
  #define NTP_SERVER             "north-america.pool.ntp.org"
  int time_min_prev              =      0;
  
  WiFiUDP ntpUDP;
  NTP ntp(ntpUDP);

// Clock
  // 4096 * 110 / 8 = 56320
  #define STEPS_PER_ROTATION        56320
  #define FILTER_WEIGHT                25
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
  #define LOOP_TIME                 1000
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

  stepper.disableOutputs();
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

  stepper.disableOutputs();
}

void HandleTime() {
  int time_min = ntp.minutes();
  if (time_min != time_min_prev) {
    RunToMinute(time_min);
    time_min_prev = time_min;
  }
}

void setup() {
  // Serial start
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // WiFi Setup
  Serial.println("Setting up WiFi: ");
  wm.setConnectTimeout(20);
  wm.setConfigPortalTimeout(300);

  bool res;
  res = wm.autoConnect("HollowClock3"); 
  if (res) {
      Serial.println("WiFi Connected Successfully!");
  } else {
      Serial.println("Failed to Connect WiFi!");
      ESP.restart(); // In lieu of restarting (bootloop if WiFi unavailable), just don't use connected stuff
  }
  wifi_check_next = millis() + WIFI_CHECK_INTERVAL;
  Serial.println();
  

  // NTP Setup
  Serial.println("Setting up NTP");
  ntp.updateInterval(NTP_UPDATE_INT); // set to update from ntp server every 900 seconds, or 15 minutes
  ntp.ruleDST("CDT", Second, Sun, Mar, 2, -300);
  ntp.ruleSTD("CST",  First, Sun, Nov, 3, -360);
  ntp.begin(NTP_SERVER);

  // Motor Setup
  Serial.println("Setting up motor");
  stepper.setMaxSpeed(STEPPER_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);

  // Endstop Setup (and home)
  endstop.Setup();
  stepper.setMaxSpeed(STEPPER_SPEED);

  // Go to minute 0
  //Serial.print("Moving to Zero Position");
  //MakeMoveTo(endstop.ZeroPos());

  Serial.println("...Done!");

  // Prepare Loop
  loop_next = millis()+ LOOP_TIME;
}

void loop() {
  ntp.update();
  HandleTime();
  
  // Check WiFi Connection
  if (millis() >= wifi_check_next) {
    if (WiFi.status() != WL_CONNECTED) {
      wm.autoConnect("HollowClock3");
    }
    wifi_check_next = millis() + WIFI_CHECK_INTERVAL;
  }
}