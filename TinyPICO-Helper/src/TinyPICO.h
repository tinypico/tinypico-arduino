// ---------------------------------------------------------------------------
// TinyPICO Helper Library - v1.4 - 18/10/2019
//
// AUTHOR/LICENSE:
// Created by Seon Rozenblum - seon@unexpectedmaker.com
// Copyright 2016 License: GNU GPL v3 http://www.gnu.org/licenses/gpl-3.0.html
//
// LINKS:
// Project home: http://tinypico.com
// Blog: http://tinypico.com
//
// DISCLAIMER:
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
//
// PURPOSE:
// Helper Library for the TinyPICO http://tinypico.com
//

// HISTORY:

//
// v1.4 - Support for esp32 calibrated battery voltage conversion ( @joey232 )
//      - Removed temperature senser functions - This has been depreciated by Espressif
//      - See https://github.com/espressif/esp-idf/issues/146
// v1.3 - Code cleanup for SWSPI bit-banging and fixed single set color not working the first time
// v1.2 - Fixed incorrect attenuation calc in the battery voltage method
// v1.1 - Fixed folder structure to be compliant with the Arduino Library Manager requirements
// v1.0 - Initial Release
//
// ---------------------------------------------------------------------------

#ifndef TinyPICO_h
	#define TinyPICO_h


  	#if defined(ARDUINO) && ARDUINO >= 100
    	#include <Arduino.h>
  	#else
    	#include <WProgram.h>
			#include <pins_arduino.h>
		#endif

	#include <SPI.h>
	
	#define DOTSTAR_PWR 13
	#define DOTSTAR_DATA 2
	#define DOTSTAR_CLK 12

	#define BAT_CHARGE 34
	#define BAT_VOLTAGE 35

	class TinyPICO
	{
		public:
			TinyPICO();
			~TinyPICO();
			
			// TinyPICO Features
			void DotStar_SetPower( bool state );
			float GetBatteryVoltage();
			bool IsChargingBattery();

			// Dotstar
			void DotStar_Clear();                                // Set all pixel data to zero
			void DotStar_SetBrightness( uint8_t );                 // Set global brightness 0-255
			void DotStar_SetPixelColor( uint32_t c );
			void DotStar_SetPixelColor( uint8_t r, uint8_t g, uint8_t b );
			void DotStar_Show( void );															// Issue color data to strip
			void DotStar_CycleColor();
			void DotStar_CycleColor( unsigned long wait );		
			uint32_t Color( uint8_t r, uint8_t g, uint8_t b ); // R,G,B to 32-bit color   

            // Tone for making sound on any ESP32 - just using channel 0
            void Tone( uint8_t, uint32_t );
            void NoTone( uint8_t );

			
		protected:
			void swspi_init(void);                      // Start bitbang SPI
			void swspi_out(uint8_t n);                  // Bitbang SPI write
			void swspi_end(void);                       // Stop bitbang SPI
			
		private:
			unsigned long nextVoltage; 
			float lastMeasuredVoltage;
			byte colorRotation;
			unsigned long nextRotation;
			uint8_t brightness;                             // Global brightness setting  
			uint8_t pixel[ 3 ];                             // LED RGB values (3 bytes ea.)  
			bool isInit;
            bool isToneInit;
	};



#endif