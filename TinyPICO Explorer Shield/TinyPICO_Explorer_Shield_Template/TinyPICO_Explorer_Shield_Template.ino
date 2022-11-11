/*
   Explorer Shield Template
*/

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

#include "buttons.h"
#include "secret.h"
#include "bitmaps.h"
#include "helpers.h"

#if defined(ARDUINO_TINYS3)

#define IMU_INT 6
#define LIGHT_SENSOR 7
#define AUDIO 21
#define SD_CARD_DETECT 5
#define SD_CARD_CS 34
#define TFT_BACKLIGHT 4
#define TFT_CS 2
#define TCT_DC 1

#elif defined(ARDUINO_TINYS2)

#define IMU_INT 33
#define LIGHT_SENSOR 38 // Digital HIGH/LOW only
#define AUDIO 18
#define SD_CARD_DETECT 17
#define SD_CARD_CS 14
#define TFT_BACKLIGHT 7
#define TFT_CS 5
#define TCT_DC 4

#else

#define IMU_INT 33
#define LIGHT_SENSOR 32
#define AUDIO 25
#define SD_CARD_DETECT 26
#define SD_CARD_CS 5
#define TFT_BACKLIGHT 27
#define TFT_CS 14
#define TCT_DC 4

#endif

#define TFT_RESET -1

// Declaration for the ST7789
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TCT_DC, TFT_RESET);

// Declaration for the LIS3DH accelerometer
Adafruit_LIS3DH lis = Adafruit_LIS3DH();


ExplorerButtonManager buttonManager;

int idle_time_to_deepsleep = 1000 * 30; // 30 seconds in millis
int idle_time_to_deepsleep_warning = 1000 * 10; // 10 seconds in millis
unsigned long last_button_touched = 0;
unsigned long one_second_step = 0;
bool is_sec_step = false;
bool is_showing_ds_warning = false;
unsigned long nextReadTimeIMU = 0;
double roll = 0.00, pitch = 0.00;   //Roll & Pitch are the angles which rotate by the axis X and y

unsigned long nextReadTimeLightSensor = 0;
float lightSensorVal;

int currentState = 0;
bool isToneInit = false;

void Tone(uint32_t freq)
{
  if ( !isToneInit )
  {
    pinMode( AUDIO, OUTPUT);
    ledcSetup(0, freq, 8); // Channel 0, resolution 8
    ledcAttachPin( AUDIO, 0 );
    isToneInit = true;
  }
  ledcWriteTone( 0, freq );
}

void NoTone()
{
  if (isToneInit)
  {
    ledcWriteTone(0, 0);
    pinMode( AUDIO, INPUT_PULLDOWN);
    isToneInit = false;
  }
}

void BootSound()
{
  for (int freq = 255; freq < 2000; freq = freq + 250)
  {
    Tone(freq);
    delay(50);
  }
  NoTone();
}

void beep( int freq, int hold = 25 )
{
  Tone(freq);
  delay(hold);
  NoTone();
}

void button_Touched()
{
  beep(500, 10);
}

void button1_Click()
{
  Serial.println("Clicked 1");
  beep(1000);
}

void button1_LongPress()
{
  Serial.println("Long Press 1");
  beep(2000, 50);
}

void setup()
{
  Serial.begin(115200);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Disable any IO deep sleep hold that was set before going into dfeep sleep.
#if defined(ARDUINO_TINYS3)
  gpio_hold_dis(GPIO_NUM_4);
#elif defined(ARDUINO_TINYS2)
  gpio_hold_dis(GPIO_NUM_7);
#else
  gpio_hold_dis(GPIO_NUM_27);
#endif
  gpio_deep_sleep_hold_dis();

  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(SD_CARD_DETECT, INPUT_PULLUP);

  delay(500);

  pinMode(TFT_BACKLIGHT, OUTPUT);
  // Digital ON/OFF for TFT Backlight
  digitalWrite(TFT_BACKLIGHT, HIGH);

  // PWM control for TFT Backlight
  // Setup timer and attach timer to a led pin
  // ledcSetup(0, 1000, 13);
  // ledcAttachPin(TFT_BACKLIGHT, 0);
  // ledcWrite(0, 0);

  Serial.println();

  // Init ST7789 240x240
  tft.init(240, 240);
  tft.fillScreen(ST77XX_BLACK);

  // Initialise the button manager
  buttonManager.begin(button_Touched);
  // Example of how to wire up a button for click and long press callbacks
  buttonManager.assignCallbacks('1', button1_Click, button1_LongPress);

  // Initialise the LIS3DH at addreess 0x18
  if (!lis.begin(0x18))
  {
    Serial.println("Error - LIS3DH Not Found or failed to start - Execution halted!");
    while (1) {
      delay(10);
    }
  }
  else
  {
    // Set the IMU rage - options are 2, 4, 8 or 16 G
    lis.setRange(LIS3DH_RANGE_4_G);

    Serial.print("LIS3DH Range = ");
    Serial.print(2 << lis.getRange());
    Serial.println("G");
  }

  delay(100);

  // Show initial UM Logo as a splash screen.
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(0);
  tft.drawBitmap( 30, ( tft.height() / 2 ) - 45 , UM_Logo, 180, 90, ST77XX_WHITE);

  // Play a boot sound
  BootSound();

  delay(1000);

  // Show TinyPICO Logo & Explorer Shield info
  tft.fillScreen(ST77XX_BLACK);
  tft.drawBitmap( 25, 20, TP_Logo, 190, 44, ST77XX_BLUE);
  tft.setTextSize(2);
  tft.setCursor(30, 72);
  tft.println( "EXPLORER SHIELD" );

  // Gety the state of the SD Card
  Serial.print("uSD Card: ");
  Serial.println(GetSDCard());

}

void loop() {

  // Tick buttons via the Button Manager
  last_button_touched = buttonManager.tick();


  if ( millis() - one_second_step > 1000 )
  {
    one_second_step = millis();
    is_sec_step = true;
  }
  else
    is_sec_step = false;


  // Lastly, run the deep sleep timer if you want the device to sleep after a certain period
  // RunDeepSleepTimer();
}

void RunDeepSleepTimer()
{
  // Have we been idle long enough to go into deep sleep?
  if (millis() - last_button_touched > idle_time_to_deepsleep)
    SetupDeepSleep();
  else if (millis() - last_button_touched > idle_time_to_deepsleep - idle_time_to_deepsleep_warning )
  {
    // We only show the deep sleep warning when there is 10 seconds left or less and a whole second has flipped
    if (is_sec_step)
      ShowDeepSleepWarning((last_button_touched + idle_time_to_deepsleep) - millis() );
  }
  else if (is_showing_ds_warning)
  {
    // If we were showing the message, but the user touched a button to keep the shield alive,
    // we need to clear the message
    is_showing_ds_warning = false;
    tft.fillRect(0, 220, 240, 20, ST77XX_BLACK);
  }
}
void ShowDeepSleepWarning(int time_left)
{
  // we want to count down from 10 to 1, not 9 to 0
  time_left = round(time_left / 1000) + 1;

  tft.fillRect(0, 220, 240, 20, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLUE);
  println_Center(tft, "DEEP SLEEP in " + String(time_left) + "s", tft.width() / 2, 230);
  is_showing_ds_warning = true;
}

void DeepSleepCallback()
{
  //placeholder callback function
}


void SetupDeepSleep()
{
  /*
     Make sure we display a message to the user before this happens, letting them know we are
     going to go into deep sleep, so they have an opportunity to touch a button and prevent it from happening

  */

  // Interrupt on IO15 (Touch Pad 3) with a thresholf of 70 for higher sensitivity
  touchAttachInterrupt(T3, DeepSleepCallback, 70);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();

  // Clear the TFT
  tft.fillScreen(ST77XX_BLACK);
  digitalWrite(TFT_BACKLIGHT, LOW);

  /*
     Hold the backlight IO low during deep sleep to reduce power GPIO_NUM_27
     is the gpio_num_t struct for TFT_BACKLIGHT/IO27 for TinyPICO as an example

  */

#if defined(ARDUINO_TINYS3)
  gpio_hold_en(GPIO_NUM_4);
#elif defined(ARDUINO_TINYS2)
  gpio_hold_en(GPIO_NUM_7);
#else
  gpio_hold_en(GPIO_NUM_27);
#endif

  gpio_deep_sleep_hold_en();
  delay(5);

  //Go to sleep now
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}


float GrabLightSensor()
{
  // If it's not time to read the input, return the cached value
  if ( nextReadTimeLightSensor > millis() )
    return lightSensorVal;
  // Set the next read time to 500ms from now
  nextReadTimeLightSensor = millis() + 500;
  lightSensorVal = 0;
  // Read the value from the sensor

#if defined(ARDUINO_TINYS2)
  // TinyS2 LIGHT_SENSOR pin isn't an ADC pin.
  lightSensorVal = digitalRead( LIGHT_SENSOR );
#else
  lightSensorVal = analogRead( LIGHT_SENSOR );
#endif

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

  Serial.print("IMU Roll: ");
  Serial.print(roll);
  Serial.print("Pitch: ");
  Serial.println(pitch);

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

int GetSDCard()
{
  if (digitalRead(SD_CARD_DETECT) == LOW )
  {
    Serial.println("SDCard found! Initializing...");

    if (!SD.begin(SD_CARD_CS))
    {
      Serial.println("SDCard initialization failed!");
      return 2;
    }
    else
    {
      Serial.println("SDCard found!..");
      return 1;
    }
  }
  else
  {
    Serial.println("No SDCard found! Please insert!");
    return 0;
  }
}
