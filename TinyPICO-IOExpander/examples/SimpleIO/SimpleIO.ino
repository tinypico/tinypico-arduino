#include "Arduino.h"
#include "TinyPICOExpander.h"

// Note: These samples do not debounce inputs

TinyPICOExpander tpio = TinyPICOExpander();

void GPIOChangeCallback(uint16_t ports, uint8_t button, bool state) {
  Serial.printf("Button: %02d | State: %s\r\n", button, state ? "TRUE " : "FALSE");
}

void setup()
{
  // Serial.begin(460800);
  Serial.begin(115200);

  tpio.begin();

  // configure 4 buttons
  tpio.pinMode(4, INPUT);
  tpio.pinMode(5, INPUT);
  tpio.pinMode(6, INPUT);
  tpio.pinMode(7, INPUT);
  
  tpio.pullUp(4, HIGH);
  tpio.pullUp(5, HIGH);
  tpio.pullUp(6, HIGH);
  tpio.pullUp(7, HIGH);

  // Register callback
  tpio.RegisterChangeCB(GPIOChangeCallback);
}

void loop()
{
  tpio.update();
}
