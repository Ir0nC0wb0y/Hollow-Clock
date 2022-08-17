#include <AccelStepper.h>

// Endstop
  #define ENDSTOP                     D1
  #define ENDSTOP_OFFSET           28211   // Steps from minute 0 to forward endstop trigger
  #define ENDSTOP_INVERTED                 // Define to use inverted logic on Endstop Pin

  #define HOMING_FAST                800
  #define HOMING_SLOW                200

  extern AccelStepper stepper;


class Homing {
    public:
        Homing();
        void Setup();
        void GoHome(bool slow_approach = false);
        bool IsTriggered();
        bool IsHomed();


    private:
        bool _isHomed     =        false;
        void _makeMove(int steps);

};