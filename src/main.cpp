#include <Arduino.h>
#include <WiFiManager.h>
#include <NTP.h>
#include <AccelStepper.h>
#include <Button2.h>

// Use this for Serial communication, comment for silent operation
#define VERBOSE

WiFiManager wm;
bool wm_nonblocking = false; //change if this causes issues

// Clock parameters
  // 4096 * 110 / 8 = 56320
  //#define STEPPER_RATIO             63.68395
  //#define STEPPER_ROTATION          64
  //#define GEAR_RATIO                110 / 8
  //#define STEPS_PER_ROTATION        STEPPER_RATIO * STEPPER_ROTATION * GEAR_RATIO // steps for a full turn of minute rotor
  #define STEPS_PER_ROTATION        57273
  #define STEPS_PER_MINUTE          STEPS_PER_ROTATION / 60

// Motors
  #define STEPPER_SPEED             800   // steps/s
  #define STEPPER_ACCEL             400   // steps/s2
  #define HALFSTEP 8
  #define FULLSTEP 4
  AccelStepper stepper(HALFSTEP, D5, D7, D6, D8);

// Buttons
  #define BUTTON1      D3     // forward
  #define BUTTON2      D2     // Reverse
  bool hr_jog = false;
  Button2 button_fwd;
  Button2 button_rvs;

// End Stop
  #define ENDSTOP             D4
  #define ENDSTOP_OFFSET   28211     // Steps from minute 0 to forward endstop trigger
  bool end_trig = false;

// Time Keeping (NTP)
  long utcOffsetInSeconds_DST  = -18000;
  long utcOffsetInSeconds      = -21600;
  #define NTP_UPDATE_INT         900000
  #define NTP_SERVER             "north-america.pool.ntp.org"
  int time_min_prev = 0;
  int time_min = 0;
  int time_min_set = 0;
  
  WiFiUDP ntpUDP;
  NTP ntp(ntpUDP);

void makeAbsMove(int minutes) {
  int min_elapse = (minutes + 60 - time_min_set) % 60;
  int position = STEPS_PER_ROTATION *  ((float)min_elapse / 60.0);
  //stepper.move(-20);
  //while (stepper.distanceToGo() > 0) {
  //  stepper.run();
  //  yield();
  //}
  stepper.moveTo(position);
  while (stepper.distanceToGo() > 0) {
    stepper.run();
    yield();
  }
  stepper.disableOutputs();
}

void JogFwd(Button2& btn) {
  // Use to jog clock forward
  Serial.println("Forward button pushed");
  stepper.setCurrentPosition(0);
  stepper.moveTo(12*STEPS_PER_ROTATION);
  stepper.setMaxSpeed(STEPPER_SPEED * 2);
  stepper.setAcceleration(STEPPER_ACCEL * 2);
}

void JogRvs(Button2& btn) {
  // Use to jog clock backward
  Serial.println("Reverse button pushed");
  stepper.setCurrentPosition(0);
  stepper.moveTo(-12*STEPS_PER_ROTATION);
  stepper.setMaxSpeed(STEPPER_SPEED * 2);
  stepper.setAcceleration(STEPPER_ACCEL * 2);
}

void JogHr(Button2& btn) {
  // Use to jog clock backward
  Serial.println("Jog an Hour");
  stepper.setCurrentPosition(0);
  stepper.moveTo(STEPS_PER_ROTATION);
  stepper.setMaxSpeed(STEPPER_SPEED * 2);
  stepper.setAcceleration(STEPPER_ACCEL);
  hr_jog = true;
}

void ButtonReleased(Button2& btn) {
  Serial.print("Moved "); Serial.print(stepper.currentPosition()); Serial.println(" steps");
  stepper.setCurrentPosition(0);
  //stepper.stop();
  stepper.moveTo(0);
  stepper.setMaxSpeed(STEPPER_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);
  stepper.disableOutputs();
  time_min_prev = ntp.minutes();
  time_min = ntp.minutes();
  time_min_set = ntp.minutes();
}

void Homing() {
  Serial.println("Running Home Process");
  //stepper.move(STEPS_PER_ROTATION);
  stepper.setSpeed(STEPPER_SPEED);
  int endstop_read = 1;
  while (endstop_read == 1) {
    stepper.runSpeed();
    yield();
    endstop_read = digitalRead(ENDSTOP);
    if (endstop_read == 0) {
      Serial.println("Endstop Triggered, setting new position");
    }
  }
  Serial.println("Moving to top of the hour");
  stepper.move(ENDSTOP_OFFSET);
  while (stepper.distanceToGo() > 0) {
    stepper.run();
    yield();
  }
  stepper.setCurrentPosition(0);
  stepper.disableOutputs();
  end_trig = false;
}

void HandleTime() {
  time_min = ntp.minutes();

  if(time_min_prev == time_min) {
    return;
  }
  #ifdef VERBOSE
    Serial.print("Minute: "); Serial.println(time_min);
  #endif

  // Absolute Movement
  makeAbsMove(time_min);
  time_min_prev = time_min;
  if (time_min == time_min_set) {
    stepper.setCurrentPosition(0);
  }
}

void setup() {

  #ifdef VERBOSE
    Serial.begin(115200);
    Serial.println();
    Serial.println("Starting program");
  #endif

  // Setup WiFi
  #ifdef VERBOSE
    Serial.println("Setting up WiFi");
  #endif
    WiFi.mode(WIFI_STA);

    // WiFiManager Parameters
    wm.setConnectTimeout(20);
    wm.setConfigPortalTimeout(300);

    bool res;
    res = wm.autoConnect("HollowClock3"); 
    if (res) {
        Serial.println("WiFi Connected Successfully!");
    } else {
        Serial.println("Failed to Connect WiFi!");
        ESP.restart();
    }

  // Setup NTP
  #ifdef VERBOSE
    Serial.println("Setting up NTP");
  #endif
  ntp.updateInterval(NTP_UPDATE_INT); // set to update from ntp server every 900 seconds, or 15 minutes
  ntp.ruleDST("CDT", Second, Sun, Mar, 2, -300);
  ntp.ruleSTD("CST",  First, Sun, Nov, 3, -360);
  ntp.begin(NTP_SERVER);
  

  // Setup Motors
  #ifdef VERBOSE
    Serial.println("Setting up motor");
  #endif
  stepper.setMaxSpeed(STEPPER_SPEED);
  stepper.setAcceleration(STEPPER_ACCEL);

  // Setup Buttons
  #ifdef VERBOSE
    Serial.println("Setting up buttons");
  #endif
  button_fwd.begin(BUTTON1);
  //button_fwd.setPressedHandler(JogFwd);
  button_fwd.setLongClickDetectedHandler(JogFwd);
  button_fwd.setReleasedHandler(ButtonReleased);
  button_fwd.setDoubleClickHandler(JogHr);
  button_rvs.begin(BUTTON2);
  button_rvs.setLongClickDetectedHandler(JogRvs);
  button_rvs.setReleasedHandler(ButtonReleased);
  
  // Setup End Stop
  pinMode(ENDSTOP,INPUT_PULLUP);
  Homing(); // Move to top of the hour
  // Move to current minute
  int homing_minute = ntp.minutes();
  Serial.print("Moving to current minute, "); Serial.println(homing_minute);
  makeAbsMove(homing_minute);
  stepper.setCurrentPosition(0);
  // Set time keeping
  time_min_prev = ntp.minutes();
  time_min = ntp.minutes();
  time_min_set = ntp.minutes();

  #ifdef VERBOSE
    Serial.println("Moving on to main program");
  #endif
}

void loop() {
  ntp.update();
  button_fwd.loop();
  button_rvs.loop();
  stepper.run();
  if (hr_jog && stepper.distanceToGo() == 0) {
    stepper.setCurrentPosition(0);
    hr_jog = false;
    time_min_prev = ntp.minutes();
    time_min = ntp.minutes();
    time_min_set = ntp.minutes();
    stepper.disableOutputs();
  }
  if (stepper.distanceToGo() == 0) {
    HandleTime();
  }
}
