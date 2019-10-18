#include <TinyPICO.h>

// Initialise the TinyPICO library
TinyPICO tp = TinyPICO();

void setup()
{
  // Not used
}

void loop()
{
  // Cycle the DotStar colour every 25 milliseconds
  tp.DotStar_CycleColor(25);

  // You can set the DotStar colour directly using r,g,b values
  // tp.DotStar_SetPixelColor( 255, 128, 0 );

  // You can set the DotStar colour directly using a uint32_t value
  // tp.DotStar_SetPixelColor( 0xFFC900 );

  // You can clear the DotStar too
  // tp.DotStar_Clear();

  // To power down the DotStar for deep sleep you call this
  // tp.DotStar_SetPower( false );
}
