#include <TinyPICO.h>

TinyPICO tp = TinyPICO();

void setup()
{
}
 
void loop()
{
  // play a set of tones
  for (int freq = 255; freq < 2000; freq = freq + 250)
  {
     tp.Tone( 25, freq );
     delay(100);
  }

  tp.NoTone();
  delay(2000);
}
