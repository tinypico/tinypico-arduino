#include "Arduino.h"
#include "TinyPICOExpander.h"

TinyPICOExpander tpio = TinyPICOExpander();

// Connect an LED from the ALERT pin on the TinyPICO Expander Shield to GND.
// This LED will assert (active LOW / turn off) when the comparator condition is met (1.5v / reading of 500)

void setup()
{
  // Serial.begin(460800);
  Serial.begin(115200);
  Serial.println("Asserting comparator interrupt on ALERT (Active LOW) when voltage on channel 0 exceeds ~1.5v (a reading of 500)");

  tpio.begin();
  tpio.analogSetGain(GAIN_TWOTHIRDS);
  tpio.startComparator(0, 500); // ~1.5v
}

void loop()
{
  Serial.printf("Comparator Reading: %06u\r\n",
                tpio.getLastConversionResults());
  delay(100);
}
