#include <TinyPICO.h>

TinyPICO tp = TinyPICO();

const uint8_t audioDAC = 25;

void setup()
{
}
 
void loop()
{
  // play a set of tones
  for (int freq = 255; freq < 2000; freq = freq + 250)
  {
     tp.Tone( audioDAC, freq );
     delay(100);
  }

  tp.NoTone( audioDAC );
  delay(2000);
}
