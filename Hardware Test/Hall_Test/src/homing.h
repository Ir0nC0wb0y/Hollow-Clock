#include <AccelStepper.h>
#include <filter.h>

// Endstop
  #define ENDSTOP                     D1
  #define ENDSTOP_OFFSET           28100   // Steps from Zero forward to endstop
  #define ENDSTOP_INVERTED                 // Define to use inverted logic on Endstop Pin

  #define HOMING_FAST               1200
  #define HOMING_SLOW                200

  #define FILTER_LIMIT             10000
  #define FILTER_TOLERANCE           500

  extern AccelStepper stepper;
  extern ExponentialFilter<int> RotationFilter;


class Homing {
  public:
    Homing();
    void Setup();
    void GoHome(bool slow_approach = false);
    bool IsTriggered();
    int  LastTrigger();
    int  ZeroPos();
    bool IsHomed();
    bool Handle(bool report = false);
    void HomeReport(bool report);

  private:
    bool _isHomed         =    false;
    bool _end_current     =    false;
    int _homing_state     =        0;
    int  _last_trigger    =        0;
    int _position_cur     =        0;
    int _filter_last      =        0;
    int _rotation_steps   =        0;
    int _zero_pos         =        0;
    bool _home_report     =    false;
    void _makeMove(int steps);
    void _endReport(bool updated);

};