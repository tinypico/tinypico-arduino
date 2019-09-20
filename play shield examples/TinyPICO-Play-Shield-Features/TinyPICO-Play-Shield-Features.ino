#include <TinyPICO.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

#include "secret.h"
#include "bitmaps.h"

#include <OneButton.h>

TinyPICO tp = TinyPICO();

#define LIGHT_SENSOR 32
#define LED 4
#define AUDIO 25

// Declaration for the SSD1306 display connected to I2C  with a resolution of 128x64 (SDA, SCL pins and -1 for reset pin as it's not used)
Adafruit_SSD1306 display( 128, 64, &Wire, -1 );

// Declaration for the LIS3DH accelerometer
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// Setup 4 button references with default state as LOW
OneButton button1(26, false);
OneButton button2(27, false);
OneButton button3(15, false);
OneButton button4(14, false);

int buttonStates[] = { 0, 0, 0, 0 };

unsigned long nextButtonHelp = 0;
uint8_t buttonHelpState = 0;


unsigned long nextReadTimeIMU = 0;
double roll = 0.00, pitch = 0.00;   //Roll & Pitch are the angles which rotate by the axis X and y
bool old_LED_state = false;


unsigned long nextReadTimeLightSensor = 0;
float lightSensorVal;

// display states
bool showLightSensor = true;
bool showIMU = true;
bool wasWifi = false;

// Wifi Stuff
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 36000;
const int   daylightOffset_sec = 3600;

// Loop states
// 0 - default state - either shows button help or day/time if WiFi connected
// 1 - connect or disconnect from WiFi depending on current status
// 2 - connecting to WiFi status screen
int currentState = 0;

void setup()
{
  Serial.begin(115200);

  tp.DotStar_SetPower( false );

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  pinMode( LED, OUTPUT );

  delay(200);

  // Initialise the SSD1306 OLED at address 0x03C
  // Alternate I2C Address is 0x3D
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed - Execution halted!");
    // Don't proceed, loop forever
    while (1);
  }
  Serial.println("SSD1306 OLED initialised");

  // Initialise the LIS3DH as address 0x18
  // Alternate I2C Address is 0x19
  if (! lis.begin(0x18))
  {
    Serial.println("No LIS3DH IMU Found or failed to start - Execution halted!");
    // Don't proceed, loop forever
    while (1);
  }
  Serial.println("LIS3DH found and initialised!");

  // Set the IMU rage - options are 2, 4, 8 or 16 G
  lis.setRange(LIS3DH_RANGE_4_G);

  Serial.print("LIS3DH Range = ");
  Serial.print(2 << lis.getRange());
  Serial.println("G");

  // Attach the buttons to their callbacks
  button1.attachClick(Click1);
  button2.attachClick(Click2);
  button3.attachClick(Click3);
  button4.attachClick(Click4);

  // Show initial TinyPICO Logo as a splash screen.
  display.clearDisplay();
  display.drawBitmap( 0, (64 - 30) / 2, TP_Logo, 128, 30, 1);
  display.display();

  // Play a boot sound
  BootSound();

  // Pause for 2 seconds
  delay(2000);

  // Clear the buffer
  display.clearDisplay();
  display.display();

  // Set button help timer to 2 seconds from now to make sure we see the first item
  nextButtonHelp = millis() + 2000;
}

void BootSound()
{
   for (int freq = 255; freq < 2000; freq = freq + 250)
   {
     tp.Tone( AUDIO, freq );
     delay(50);
  }

  tp.NoTone( AUDIO );
}

void loop() {
  // put your main code here, to run repeatedly:
  button1.tick();
  button2.tick();
  button3.tick();
  button4.tick();

  if ( currentState == 1 )
  {
    // If SSID not set, show error and cancel Wifi Connection
    if ( secret_ssid == "enter_ssid_here" )
    {
      buttonHelpState = 99;
      nextButtonHelp = millis() + 3000;
      currentState = 0;
      return;
    }

    if ( WiFi.status() == WL_CONNECTED )
    {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      display.clearDisplay();
      display.display();
      currentState = 0;
    }
    else
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 15);            // Start at top-left corner
      display.println( "Connecting To:" );
      display.println( secret_ssid );
      display.display();
      display.setCursor(0, 45);
      WiFi.begin(secret_ssid, secret_password);
      currentState = 2;
    }
  }
  else if ( currentState == 2 )
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      display.print( "." );
      display.display();
      delay(100);
    }
    else
    {
      wasWifi = true;
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

      display.clearDisplay();
      display.display();
      currentState = 0;
    }
  }
  // Idle state
  else if ( currentState == 0 )
  {
    if ( WiFi.status() == WL_CONNECTED )
    {
      display.setTextSize(1);
      display.fillRect( 0, 0, 111, 12, WHITE);
      display.drawBitmap( 112, 1, icon_wifi_rev, 14, 10, 1);
      display.drawRect( 111, 0, 16 , 12, WHITE);

      struct tm timeinfo;
      if (getLocalTime(&timeinfo))
      {
        display.setCursor(1, 2);
        display.setTextColor(BLACK);
        display.println(&timeinfo, "%A, %H:%M:%S");
      }
    }
    else
    {
      display.setTextSize(1);
      display.fillRect( 0, 0, 127, 12, WHITE);
      display.setCursor(1, 2);
      display.setTextColor(BLACK);

      switch ( buttonHelpState )
      {
        case 0:
          display.println( "[1] Toggle Light Sensor");
          break;
        case 1:
          display.println( "[2] Toggle IMU");
          break;
        case 2:
          display.println( "[3] ...");
          break;
        case 3:
          display.println( "[4] Toggle WiFi");
          break;
        case 99:
          display.println( "ERR: SSID Not Set!");
          break;
      }
    }

    if ( nextButtonHelp < millis() )
    {
      nextButtonHelp = millis() + 2000;
      buttonHelpState = ( buttonHelpState + 1 ) % 4;
    }

    display.setTextSize(2);
    display.setTextColor(WHITE);

    if ( showLightSensor )
    {
      display.fillRect( 0, 15, 127, 14, BLACK);
      display.setCursor(0, 15);
      display.println( String( GrabLightSensor() ) );
    }

    if ( showIMU )
    {
      GrabAccel();

      display.fillRect( 0, 30, 127, 30, BLACK);
      display.setCursor(0, 30);
      display.println( String( roll) );
      display.println( String( pitch ));

      // If the Pitch and Roll are close to 0,0, turn  on the blue LED
      bool LED_STATE = ( abs(roll) < 1 ) && ( abs( pitch ) < 1 );
      if ( LED_STATE != old_LED_state )
      {
        old_LED_state = LED_STATE;
        digitalWrite( LED, LED_STATE );
      }
    }

    // Flash the button number for the last state of buttons pressed
    display.setTextSize(1);
    for ( int b = 0; b < 4; b++ )
    {
      if ( buttonStates[b] > 1 )
      {
        buttonStates[b]--;
        int posX = 7 * b;
        display.setCursor( 100 + posX, 56 );
        display.print( String( b + 1 ) );
      }
      else if ( buttonStates[b] == 1 )
      {
        int posX = 7 * b;
        display.fillRect( 100 + posX, 54, 8, 9, BLACK);
      }
    }

    display.display();
  }
}
void Click1()
{
  buttonStates[0] = 10;
  ToggleLightSensor();
}

void Click2()
{
  buttonStates[1] = 10;
  ToggleIMU();
}

void Click3()
{
  buttonStates[2] = 10;
}

void Click4()
{
  buttonStates[3] = 10;
  currentState = 1;

}

void ToggleLightSensor()
{
  if ( showLightSensor )
  {
    display.fillRect( 0, 15, 127, 14, BLACK);
  }
  else
  {
    nextReadTimeLightSensor = 0;
  }
  showLightSensor = !showLightSensor;
}

void ToggleIMU()
{
  if ( showIMU )
  {
    display.fillRect( 0, 30, 127, 30, BLACK);
  }
  else
  {
    nextReadTimeIMU = 0;
  }
  showIMU = !showIMU;
}

float GrabLightSensor()
{
  // If it's not time to read the input, return the cached value
  if ( nextReadTimeLightSensor > millis() )
    return lightSensorVal;

  // Set the next read time to 500ms from now
  nextReadTimeLightSensor = millis() + 500;

  // Read the value from the sensor
  lightSensorVal = analogRead( LIGHT_SENSOR );
  return lightSensorVal;
}


void GrabAccel()
{
  // If it's not time to read the input, return early
  if ( nextReadTimeIMU > millis() )
    return;

  // Set the next read time to 200ms from now
  nextReadTimeIMU = millis() + 200;

  sensors_event_t event;
  lis.getEvent(&event);

  double x_Buff = float(event.acceleration.x);
  double y_Buff = float(event.acceleration.y);
  double z_Buff = float(event.acceleration.z);

  roll = atan2(y_Buff , z_Buff) * 57.3;
  pitch = atan2((- x_Buff) , sqrt(y_Buff * y_Buff + z_Buff * z_Buff)) * 57.3;
}

// Wifi Stuff
int8_t GetWifiQuality()
{
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}
