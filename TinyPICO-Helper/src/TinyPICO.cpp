// ---------------------------------------------------------------------------
// TinyPICO Helper Library - v1.4 - 18/10/2019
//
// Created by Seon Rozenblum - seon@unexpectedmaker.com
// Copyright 2019 License: MIT https://github.com/tinypico/tinypico-arduino/blob/master/LICENSE
//
// See "TinyPICO.h" for purpose, syntax, version history, links, and more.
//
// v1.4 - Support for esp32 calibrated battery voltage conversion ( @joey232 )
//      - Removed temperature senser functions - This has been depreciated by Espressif
//      - See https://github.com/espressif/esp-idf/issues/146
// v1.3 - Code cleanup for SWSPI bit-banging and fixed single set color not working the first time
// v1.2 - Fixed incorrect attenuation calc in the battery voltage method
// v1.1 - Fixed folder structure to be compliant with the Arduino Library Manager requirements
// v1.0 - Initial Release
// ---------------------------------------------------------------------------

#include "TinyPICO.h"
#include <SPI.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

	#ifdef __cplusplus
			extern "C" {
	#endif
			uint8_t temprature_sens_read();
	#ifdef __cplusplus
			}
	#endif

// Battery divider resistor values
#define UPPER_DIVIDER 442
#define LOWER_DIVIDER 160
#define DEFAULT_VREF  1100  // Default referance voltage in mv
#define BATT_CHANNEL ADC1_CHANNEL_7  // Battery voltage ADC input

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
        delay(10);
    }
    
    uint16_t b16 = (uint16_t)brightness; // Type-convert for fixed-point math

    // Start-frame marker
    for( int i=0; i<4; i++) swspi_out(0x00);    
    
    // Pixel start
    swspi_out(0xFF);     
    
    for( int i=0; i<3; i++)
    {
        if( brightness > 0)
            swspi_out((pixel[i] * b16) >> 8); // Scale, write - Scaling pixel brightness on output
        else
            swspi_out(pixel[i]); // R,G,B @Full brightness (no scaling) 
    }             

    // // End frame marker
    swspi_out(0xFF);  
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
    delay(1);
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
    uint32_t raw, mv;
    esp_adc_cal_characteristics_t chars;

    // only check voltage every 1 second
    if ( nextVoltage - millis() > 0 )
    {
        nextVoltage = millis() + 1000;

        // grab latest voltage
        analogRead(BAT_VOLTAGE);  // Just to get the ADC setup
        raw = adc1_get_raw(BATT_CHANNEL);  // Read of raw ADC value

        // Get ADC calibration values
        esp_adc_cal_characterize(ADC_UNIT_1,ADC_ATTEN_11db ,ADC_WIDTH_BIT_12,DEFAULT_VREF,&chars);

        // Convert to calibrated mv then volts
        mv = esp_adc_cal_raw_to_voltage(raw, &chars) * (LOWER_DIVIDER+UPPER_DIVIDER) / LOWER_DIVIDER;
        lastMeasuredVoltage = (float)mv / 1000.0;
    }

    return ( lastMeasuredVoltage );
}

// Internal Temp Sensor
// This has been removed from Silicon and future IDF API
// See this for more information: https://github.com/espressif/esp-idf/issues/146
// uint8_t TinyPICO::Get_Internal_Temp_F()
// {
//     return( temprature_sens_read() );
// }

// float TinyPICO::Get_Internal_Temp_C()
// {
//     float temp_farenheit = temprature_sens_read();
//     return( ( temp_farenheit - 32 ) / 1.8 );
// }

// Tone - Sound wrapper
void TinyPICO::Tone( uint8_t pin, uint32_t freq )
{
    if ( !isToneInit )
    {
        pinMode( pin, OUTPUT);
        ledcSetup(0, freq, 8); // Channel 0, resolution 8
        ledcAttachPin( pin , 0 );
        isToneInit = true;
    }

    ledcWriteTone( 0, freq );
}

void TinyPICO::NoTone( uint8_t pin )
{
    if ( isToneInit )
    {
        ledcWriteTone(0, 0);
        pinMode( pin, INPUT_PULLDOWN);
        isToneInit = false;
    }
}
