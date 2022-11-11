#include <Adafruit_MPR121.h>

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

extern "C" {
  typedef void (*callbackFunction)(void);
}

class ExplorerButton
{
  public:
    ExplorerButton();

    void setupButton(char button, Adafruit_MPR121 cap, callbackFunction touchBeep);
    void attachPress(callbackFunction newFunction);
    void attachPressLong(callbackFunction newFunction);

    bool tick(void);
    bool tick(bool level);
    void reset(void);

    int getId(char button);

  private:
    Adafruit_MPR121 _cap;
    char _ids[12] = {'3', '2', '1', 'D', 'L', 'R', '4', 'U', 'B', 'A', 'Y', 'X'};
    int _id = -1; // button face id from MPR
    unsigned int _clickTicks = 100; // number of ticks before a click is detected
    unsigned int _pressTicks = 300; // number of ticks before a long press is detected

    // These variables will hold functions acting as event source.
    callbackFunction _touchBeep = NULL;
    callbackFunction _pressFunc = NULL;
    callbackFunction _pressLongFunc = NULL;

    int _state = 0;
    unsigned long _startTime;
};


ExplorerButton::ExplorerButton()
{
}

void ExplorerButton::setupButton(char button, Adafruit_MPR121 cap, callbackFunction touchBeep = nullptr)
{
  _cap = cap;
  _id = getId(button);
  _touchBeep = touchBeep;
}

int ExplorerButton::getId( char button )
{
  for ( int i = 0; i < 12; i++ )
  {
    if ( _ids[i] == button )
      return i;
  }
  return -1;
}

// save function for click event
void ExplorerButton::attachPress(callbackFunction newFunction)
{
  _pressFunc = newFunction;
}

// save function for doubleClick event
void ExplorerButton::attachPressLong(callbackFunction newFunction)
{
  _pressLongFunc = newFunction;
}

void ExplorerButton::reset(void) {
  _state = 0;
  _startTime = 0;
}

bool ExplorerButton::tick(void)
{
  // We only want to tick this button if it has an ID.
  if (_id > -1 )
    return tick(_cap.touched() & _BV(_id));

   return false;
}

bool ExplorerButton::tick(bool activeLevel)
{
  unsigned long now = millis(); // current time in milis.
  bool was_touched = false;

  if (_state == 0) // Start state
  {
    if (activeLevel)
    {
      _state = 1; // step to state 1
      _startTime = now; // remember starting time

      was_touched = true;

      if ( _touchBeep )
        _touchBeep();
    } // if
  }
  else if (_state == 1) // Pressed state
  {
    if (!activeLevel) // Did we let go of the button?
    {
      was_touched = true;

      // Did we hold long enough for a long press and do we have a long press callback?
      if ( ((unsigned long)(now - _startTime) > _pressTicks) && _pressLongFunc )
      {
        _pressLongFunc();
      }
      // Ok, not a long press, so do we have a click callback?
      else if ( ((unsigned long)(now - _startTime) > _clickTicks) && _pressFunc )
      {
        _pressFunc();
      }
      reset();
    }
  }

  return was_touched;
};



class ExplorerButtonManager
{
  public:
    ExplorerButtonManager();
    unsigned long tick(void);
    void begin(callbackFunction touchBeep);
    void assignCallbacks(char face, callbackFunction click, callbackFunction press);
    int getId(char button);
  private:
    Adafruit_MPR121 cap;
    char _ids[12] = {'3', '2', '1', 'D', 'L', 'R', '4', 'U', 'B', 'A', 'Y', 'X'};
    ExplorerButton buttons[12];
    unsigned long last_touch = 0;
};

ExplorerButtonManager::ExplorerButtonManager()
{
}

void ExplorerButtonManager::begin(callbackFunction touchBeep = nullptr)
{
  Serial.println("Button Manager Setup!");
  // Declaration for the MPR121 Cap Touch IC
  cap = Adafruit_MPR121();
  // Initialise MPR121 at address 0x5A
  if (!cap.begin(0x5A))
  {
    Serial.println("Error - MPR121 Not found  - Execution halted!");
    while (1) {
      delay(10);
    }
  }

  for ( int id = 0; id < 12; id++ )
  {
    buttons[id] = ExplorerButton();
    buttons[id].setupButton(_ids[id], cap, touchBeep );
  }

  last_touch = millis();
};

unsigned long ExplorerButtonManager::tick()
{
  for ( int id = 0; id < 12; id++ )
  {
    if (buttons[id].tick())
    {
      // a button was touched, so update last button touch time
      last_touch = millis();
    }
  }

  // Special case for first tick, to ensure it's the milllis() time for the first loop of the program
  if (last_touch == 0)
    last_touch = millis();
  
  return last_touch;
}

void ExplorerButtonManager::assignCallbacks(char face, callbackFunction click, callbackFunction press)
{
  int id = getId(face);
  if (click != nullptr)
    buttons[id].attachPress(click);
  if (press != nullptr)
    buttons[id].attachPressLong(press);
}

int ExplorerButtonManager::getId( char button )
{
  for ( int i = 0; i < 12; i++ )
  {
    if ( _ids[i] == button )
      return i;
  }
  return -1;
}
