#include <Arduino.h>
#include <WiFiManager.h>
#include <NTP.h>
#include <AccelStepper.h>
#include <Button2.h>

// Use this for Serial communication, comment for silent operation
//#define VERBOSE

WiFiManager wm;
bool wm_nonblocking = false; //change if this causes issues

// Clock parameters
  // 4096 * 110 / 8 = 56320
  #define STEPS_PER_ROTATION        56320 // steps for a full turn of minute rotor
  #define STEPS_PER_MINUTE          STEPS_PER_ROTATION / 60

// Motors
  #define STEPPER_SPEED             400   // steps/s
  #define STEPPER_ACCEL             400   // steps/s2
  #define HALFSTEP 8
  #define FULLSTEP 4
  AccelStepper stepper(HALFSTEP, D5, D7, D6, D8);

// Buttons
  #define BUTTON1      D3     // forward
  #define BUTTON2      D2     // Reverse
  Button2 button_fwd;
  Button2 button_rvs;

// Time Keeping (NTP)
  long utcOffsetInSeconds_DST  = -18000;
  long utcOffsetInSeconds      = -21600;
  #define NTP_UPDATE_INT         900000
  #define NTP_SERVER             "north-america.pool.ntp.org"
  int time_min_prev = 0;
  int time_min = 0;
  
  WiFiUDP ntpUDP;
  NTP ntp(ntpUDP);

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
  ntp.updateInterval(900000); // set to update from ntp server every 900 seconds, or 15 minutes
  ntp.ruleDST("CDT", Second, Sun, Mar, 2, -300);
  ntp.ruleSTD("CST",  First, Sun, Nov, 3, -360);
  ntp.begin(NTP_SERVER);
  time_min_prev = ntp.minutes();
  time_min = ntp.minutes();

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
    ntp.update();
    
    time_min = ntp.minutes();

    if(time_min_prev == time_min) {
      return;
    }
    #ifdef VERBOSE
      Serial.print("Minute: "); Serial.println(time_min);
    #endif
    
    int pos = STEPS_PER_MINUTE * (time_min - time_min_prev);
    // add one extra steps every 2 out of 3 minutes
    if (time_min % 3 > 0) {
      pos = pos + 1;
    }
    makeMove(pos);
    time_min_prev = time_min;
  }
}
