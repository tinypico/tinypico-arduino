#include "Arduino.h"
#include "TinyPICOExpander.h"

TinyPICOExpander tpio = TinyPICOExpander();

void setup()
{
  // Serial.begin(460800);
  Serial.begin(115200);
  Serial.println("Getting single ended readings from Channel 0-4");

  tpio.begin();
  tpio.analogSetGain(GAIN_TWOTHIRDS);
}

void loop()
{
  Serial.printf("A0: %06u | A1: %06u | A2: %06u | A3: %06u\r\n",
                tpio.analogReadSingleEnded(0),
                tpio.analogReadSingleEnded(1),
                tpio.analogReadSingleEnded(2),
                tpio.analogReadSingleEnded(3));

  delay(100);
}
