// ---------------------------------------------------------------------------
// Created by Seon Rozenblum - seon@unexpectedmaker.com
// Copyright 2016 License: GNU GPL v3 http://www.gnu.org/licenses/gpl-3.0.html
//
// See "TinyPICO.h" for purpose, syntax, version history, links, and more.
// ---------------------------------------------------------------------------

#include "TinyPICO.h"
#include <SPI.h>

	#ifdef __cplusplus
			extern "C" {
	#endif
			uint8_t temprature_sens_read();
	#ifdef __cplusplus
			}
	#endif

TinyPICO::TinyPICO()
{
    pinMode( DOTSTAR_PWR, OUTPUT );
    pinMode( BAT_CHARGE, INPUT );
    pinMode( BAT_VOLTAGE, INPUT );

    DotStar_SetPower( false );
    nextVoltage = millis();

    for (int i = 0; i < 3; i++ )
        pixel[i] = 0;

    isInit = false;
    brightness = 128;
    colorRotation = 0;
    nextRotation = 0;
}

TinyPICO::~TinyPICO()
{
    isInit = false;
    DotStar_SetPower( false );
}

void TinyPICO::DotStar_SetBrightness(uint8_t b)
{
    // Stored brightness value is different than what's passed.  This
    // optimizes the actual scaling math later, allowing a fast 8x8-bit
    // multiply and taking the MSB.  'brightness' is a uint8_t, adding 1
    // here may (intentionally) roll over...so 0 = max brightness (color
    // values are interpreted literally; no scaling), 1 = min brightness
    // (off), 255 = just below max brightness.
    brightness = b + 1;
}

// Convert separate R,G,B to packed value
uint32_t TinyPICO::Color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void TinyPICO::DotStar_Show(void)
{
    if ( !isInit )
    {
        isInit = true;
        swspi_init();
        delay(200);
    }
    uint8_t i;            // -> LED data
    uint16_t n   = 1;     // Counter
    uint16_t b16 = (uint16_t)brightness; // Type-convert for fixed-point math

    for(i=0; i<4; i++) swspi_out(0);    // Start-frame marker
    if(brightness)
    {                     // Scale pixel brightness on output
        do
        {                               // For each pixel...
            swspi_out(0xFF);                //  Pixel start
            for(i=0; i<3; i++) swspi_out((pixel[i] * b16) >> 8); // Scale, write
        } while(--n);
    }
    else
    {                             // Full brightness (no scaling)
        do
        {                               // For each pixel...
            swspi_out(0xFF);                //  Pixel start
            for(i=0; i<3; i++) swspi_out(pixel[i]); // R,G,B
        } while(--n);
    }
    for(i=0; i<1; i++) swspi_out(0xFF); // End-frame marker (see note above)
}


void TinyPICO::swspi_out(uint8_t n)
{
    for(uint8_t i=8; i--; n <<= 1)
    {
        if (n & 0x80)
            digitalWrite(DOTSTAR_DATA, HIGH);
        else
            digitalWrite(DOTSTAR_DATA, LOW);
        digitalWrite(DOTSTAR_CLK, HIGH);
        digitalWrite(DOTSTAR_CLK, LOW);
    }
}

void TinyPICO::DotStar_Clear() { // Write 0s (off) to full pixel buffer
    for (int i = 0; i < 3; i++ )
        pixel[i] = 0;

    DotStar_Show();
}

// Set pixel color, separate R,G,B values (0-255 ea.)
void TinyPICO::DotStar_SetPixelColor(uint8_t r, uint8_t g, uint8_t b)
{
    pixel[0] = b;
    pixel[1] = g;
    pixel[2] = r;

    DotStar_Show();
}

// Set pixel color, 'packed' RGB value (0x000000 - 0xFFFFFF)
void TinyPICO::DotStar_SetPixelColor(uint32_t c)
{
    pixel[0] = (uint8_t)c;
    pixel[1] = (uint8_t)(c >>  8);
    pixel[2] = (uint8_t)(c >> 16);

    DotStar_Show();
}

void TinyPICO::swspi_init(void)
{
    DotStar_SetPower( true );
    digitalWrite(DOTSTAR_DATA , LOW);
    digitalWrite(DOTSTAR_CLK, LOW);
}

void TinyPICO::swspi_end()
{
    DotStar_SetPower( false );
}

// Switch the DotStar power
void TinyPICO::DotStar_SetPower( bool state )
{
	digitalWrite( DOTSTAR_PWR, !state );
	pinMode( DOTSTAR_DATA, state ? OUTPUT : INPUT_PULLDOWN );
	pinMode( DOTSTAR_CLK, state ? OUTPUT : INPUT_PULLDOWN );
}

void TinyPICO::DotStar_CycleColor()
{
    DotStar_CycleColor(0);
}

void TinyPICO::DotStar_CycleColor( unsigned long wait = 0 )
{
    if ( millis() > nextRotation + wait )
    {
        nextRotation = millis();

        colorRotation++;
        byte WheelPos = 255 - colorRotation;
        if(WheelPos < 85)
        {
            DotStar_SetPixelColor(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            DotStar_SetPixelColor(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            DotStar_SetPixelColor(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
        DotStar_Show();
    }
}

// Return the current charge state of the battery
bool TinyPICO::IsChargingBattery()
{
    int measuredVal = 0;
    for ( int i = 0; i < 10; i++ )
    {
        int v = digitalRead( BAT_CHARGE );
        measuredVal += v;
    }

    return ( measuredVal == 0);
}

// Return a *rough* estimate of the current battery voltage
float TinyPICO::GetBatteryVoltage()
{
	// only check voltage every 1 second
	if ( nextVoltage - millis() > 0 )
	{
		nextVoltage = millis() + 1000;

		// grab latest voltage
  		lastMeasuredVoltage = analogRead(BAT_VOLTAGE);
  		lastMeasuredVoltage /= 3838; // convert to voltage
  		lastMeasuredVoltage *= 4.2;  // Multiply by 3.7V, our reference voltage
	}

	return ( lastMeasuredVoltage );
}

uint8_t TinyPICO::Get_Internal_Temp_F()
{
    return( temprature_sens_read() );
}

float TinyPICO::Get_Internal_Temp_C()
{
    float temp_farenheit = temprature_sens_read();
    return( ( temp_farenheit - 32 ) / 1.8 );
}

// Tone - Suond wrapper
void TinyPICO::Tone( uint8_t pin, uint32_t freq )
{
    if ( !isToneInit )
    {
        ledcSetup(0, freq, 8); // Channel 0, resolution 8
        ledcAttachPin( pin , 0 );
        isToneInit = true;
    }

    ledcWriteTone( 0, freq );
}

void TinyPICO::NoTone()
{
    if ( isToneInit )
    {
        ledcWriteTone(0, 0);
        isToneInit = false;
    }
}