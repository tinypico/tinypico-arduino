#include "Arduino.h"
#include "TinyPICOExpander.h"

TinyPICOExpander tpio = TinyPICOExpander();

void setup()
{
  // Serial.begin(460800);
  Serial.begin(115200);
  Serial.println("Getting differential readings from Channel 0 (A0/Positive & A1/Negative) and Channel 1 (A2/Positive & A3/Negative)");

  tpio.begin();
  tpio.analogSetGain(GAIN_TWOTHIRDS);
}

void loop()
{
  Serial.printf("A0: %06u | A1: %06u\r\n",
                tpio.analogReadDifferential(0),
                tpio.analogReadDifferential(1));
  delay(100);
}
