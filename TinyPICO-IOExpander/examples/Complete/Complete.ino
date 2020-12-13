// SETUP NOTES:
// This test uses 4 buttons, 4 POTS, a breadboard and 10 optional LED's.
//
// Termainl:
// - You should connect your TinyPICO to a proper ANSI terminal such as TeraTerm and set the baud rate to 115200
// - ASCII escape codes are used to control advanced debug output
//
// Expander Setup:
// - Plug in TinyPICO
// - Connect GND and 3.3V to breadboard power rails
// - Connect Expander Board INTERRUPT A pin to TinyPICO Pin 27
//
// Button Setup:
// - Place 4 buttons on a breadboard and connect one side of each button to GND
// - Connect the 4 buttons to Expander shield IO pins marked 5, 6, 7, 8 on the Expander Shield
//
// Potentiometer Setup
// -- Place 4 pots on the breadboard. Connect them all to ground and 3.3v
// -- Connect the wiper pins to A0, A1, A2 and A3
//
// Optional LED Setup:
// - Place 10 LED's on the Breadboard connecting the short leg / negative / cathode to GND via a relevant resitor
// - Connect 8 of the LED's long leg / positive / anode to Expander Shield IO pins marked 1, 2, 3, 4, 13, 14, 15, 16
//
// Optional Interrupt Pins LED
// - Instead of conecting INTTERUPT A pin to TinyPICO Pin 27 tie these pins together on a spare row on the breadboard
// - Plug the one of the remaining LEDs long leg / positive / anode to the same row.
// - This LED will be high / ON and will go low / turn OFF when a GPIO interrupt is fired.
// - Connect the remaining LEDs long leg / positive / anode to the ALERT pin.
// - This LED will be high / ON and will go low / turn OFF when a comparator interrupt is fired.
//
// Expected Behaviour
// - The first 4 LED's will cycle
// - The next four LEDS will alternate a staggered pattern X 0 X 0 >> 0 X 0 X >> X 0 X 0 ....
// - All four buttons will generate a callback and show a message in the serial console debug screen
// - When buttons 1 & 2 are both held together, a message will be displayed in the serial console debug screen
// - Only buttons 3 & 4 will generate an interrupt handle message
// - Interrupts for other buttons will be blocked while holding down a button, although callbacks will continue to fire
// - When in analog sinle ended mode all four pot will be shown on the debug output screen
// - When in analog differential channels 0 (A0+ / A1-) & 1 (A2+ / A3-) will be shown on the debug screen output.
// - To test differential mode, go to analog mode. Set channel 0 to 500 and channel 1 to 200. Go to differential mode and channel 0 should read 300;
// - When in comparitor mode, the comparator value will read from channel 0 and be shown on the debug screen output. One the comparitor exceeds 500 the ALERT pin will go LOW

#include "Arduino.h"
#include "TinyPICOExpander.h"

typedef enum
{
  Single,
  Differential,
  Comparator
} adsMode_t;

TinyPICOExpander tpio = TinyPICOExpander();

volatile bool handleInterrupt = false;
volatile int interruptFires = 0; // count the number of times an interrupt has been fired
int callbackFires = 0;           // count the number of times a callback has been called
int cyclePort = 0;               // current port number that is being set in togglePorts

adsMode_t adsMode = Single;
int gainSetting = 0x00;

unsigned long lastToggle;

// Routine to print int as binary with leading zeros
void printBits(uint16_t b, int bits)
{
  for (int i = bits - 1; i >= 0; i--)
    Serial.print(bitRead(b, i));
}

// routine called as interrupt handler / ISR to flag interrupt process required
// we don't read the MCP23017 ports via I2C during an interrupt as it will create watchdog &  issues
// we set a bool flag and process the interrupt via the main loop
void InterruptFlag()
{
  handleInterrupt = true;
  interruptFires++;
}

// code called when the interrupt flag is processed
void ProcessInterrupt()
{
  // detachInterrupt(27);
  Serial.printf("\033[13;0H");
  Serial.printf("| Interrupt! - Pin: %03d | Value: %03d | Fires: %03d\r\n", tpio.getLastInterruptPin(), tpio.getLastInterruptPinValue(), interruptFires);
  Serial.printf("\033[H");
  // attachInterrupt(27, InterruptFlag, FALLING);
}

// test code to cycle and stagger port bits for testing
void togglePorts()
{
  if (millis() - lastToggle < 1000)
    return;

  lastToggle = millis();

  // cycle bits 0-3
  tpio.digitalWrite(0, cyclePort == 0 ? LOW : HIGH);
  tpio.digitalWrite(1, cyclePort == 1 ? LOW : HIGH);
  tpio.digitalWrite(2, cyclePort == 2 ? LOW : HIGH);
  tpio.digitalWrite(3, cyclePort == 3 ? LOW : HIGH);

  // bits 4-7 are buttons

  // stagger bits 8-15
  tpio.digitalWrite(8, cyclePort % 2 == 0 ? LOW : HIGH);
  tpio.digitalWrite(9, cyclePort % 2 != 0 ? LOW : HIGH);
  tpio.digitalWrite(10, cyclePort % 2 == 0 ? LOW : HIGH);
  tpio.digitalWrite(11, cyclePort % 2 != 0 ? LOW : HIGH);
  tpio.digitalWrite(12, cyclePort % 2 == 0 ? LOW : HIGH);
  tpio.digitalWrite(13, cyclePort % 2 != 0 ? LOW : HIGH);
  tpio.digitalWrite(14, cyclePort % 2 == 0 ? LOW : HIGH);
  tpio.digitalWrite(15, cyclePort % 2 != 0 ? LOW : HIGH);

  cyclePort++;
  if (cyclePort > 3)
    cyclePort = 0;
}

// draw initial debug screen via serial output
void drawScreen()
{
  Serial.printf("\033[2J\033[H\033[?25l"); // clear screen, home cursor, hide cursor
  Serial.println(".---------------------------------------------------------------------------------------------------------------------------------.");
  Serial.println("| \033[41mTinyPICO IO Expander Shield Test v0.1                                                                                          \033[m |");
  Serial.println("|---------------------------------------------------------------------------------------------------------------------------------|");
  Serial.println("| \033[44mDIGITAL                                                                                                                        \033[m |");
  Serial.println("|---------------------------------------------------------------.-----------------------------------------------------------------|");
  Serial.println("| \033[7mCycle                                                        \033[m | \033[7mButtons                                                        \033[m |");
  Serial.println("| Port00: -       Port01: -       Port02: -       Port03: -     | Port04: -       Port05: -       Port06: -       Port07: -       |");
  Serial.println("|-------------------------------------------------------------- +-----------------------------------------------------------------|");
  Serial.println("| \033[7mStaggered                                                    \033[m | \033[7mStaggered                                                      \033[m |");
  Serial.println("| Port08: -       Port09: -       Port10: -       Port11: -     | Port12: -       Port13: -       Port14: -       Port15: -       |");
  Serial.println("|---------------------------------------------------------------+-----------------------------------------------------------------|");
  Serial.println("| \033[7mInterrupt                                                    \033[m | \033[7mPorts                                                          \033[m |");
  Serial.println("| Interrupt! - Pin: --- | Value: --- | Fires: ---               | Ports: ---------------- |  PortA : -------- |  PortB : -------- |");
  Serial.println("|---------------------------------------------------------------+-----------------------------------------------------------------|");
  Serial.println("| \033[7mCallback                                                                                                                       \033[m |");
  Serial.println("| Callback! - Ports: ---------------- | Button: --- | State: ----- | Fires: ---                                                   |");
  Serial.println("|---------------------------------------------------------------------------------------------------------------------------------|");
  Serial.println("| \033[44mANALOG                                                                                                                         \033[m |");
  Serial.println("|---------------------------------------------------------------------------------------------------------------------------------|");
  switch(adsMode)
  {
  case Single:
    Serial.println("| \033[7mSingle Ended                                                                                                                   \033[m |");
    Serial.println("| 0: ----------------        |        1: ----------------         |         2: ----------------        |      3: ---------------- |");
    Serial.println("| 0: ----------------        |        1: ----------------         |         2: ----------------        |      3: ---------------- |");
    break;
  case Differential:
    Serial.println("| \033[7mDifferential Inputs                                                                                                            \033[m |");
    Serial.println("| 0: ----------------        |        1: ----------------         |         2: ----------------        |      3: ---------------- |");
    Serial.println("| 0: ----------------        |        1: ----------------         |         2: ----------------        |      3: ---------------- |");
    break;
  case Comparator:
    Serial.println("| \033[7mComparator                                                                                                                     \033[m |");
    Serial.println("| 0: ----------------        | Compare: -----                                                                                     |");
    Serial.println("| 0: ----------------                                                                                                             |");
    break;
  default:
    break;
  }
  Serial.println("|---------------------------------------------------------------------------------------------------------------------------------|");
  Serial.println("| \033[7mGain                                                         \033[m | \033[7mNone                                                           \033[m |");
  Serial.println("| Gain:                                                         |                                                                 |");
  Serial.println("|---------------------------------------------------------------------------------------------------------------------------------|");
  Serial.println("| \033[38;5;242mNOTES:                                                                                                                         \033[m |");
  Serial.println("| \033[38;5;242mDigital: [BUTTON1-4] - Generate Callback                                    [BUTTON3-4] - Generate Interrupt                   \033[m |");
  Serial.println("| \033[38;5;242m Analog: [BUTTON1]   - Switch Analog Mode (Single|Differential|Comparator)  [BUTTON2]   - Cycle Gain (2/3x|1x|2x|4x|8x|16x)    \033[m |");
  Serial.println("`---------------------------------------------------------------+-----------------------------------------------------------------'");
}

// update only values for debug screen via serial output
void updateScreen()
{
  // GPIO

  Serial.printf("\033[7;11H%d", tpio.digitalRead(0));
  Serial.printf("\033[7;27H%d", tpio.digitalRead(1));
  Serial.printf("\033[7;43H%d", tpio.digitalRead(2));
  Serial.printf("\033[7;59H%d", tpio.digitalRead(3));

  Serial.printf("\033[7;75H%d", tpio.digitalRead(4));
  Serial.printf("\033[7;91H%d", tpio.digitalRead(5));
  Serial.printf("\033[7;107H%d", tpio.digitalRead(6));
  Serial.printf("\033[7;123H%d", tpio.digitalRead(7));

  Serial.printf("\033[10;11H%d", tpio.digitalRead(8));
  Serial.printf("\033[10;27H%d", tpio.digitalRead(9));
  Serial.printf("\033[10;43H%d", tpio.digitalRead(10));
  Serial.printf("\033[10;59H%d", tpio.digitalRead(11));

  Serial.printf("\033[10;75H%d", tpio.digitalRead(12));
  Serial.printf("\033[10;91H%d", tpio.digitalRead(13));
  Serial.printf("\033[10;107H%d", tpio.digitalRead(14));
  Serial.printf("\033[10;123H%d", tpio.digitalRead(15));

  Serial.print("\033[13;67HPorts: ");
  printBits(tpio.readPorts(), 16);
  Serial.print(" |  PortA : ");
  printBits(tpio.readPorts(MCP23017_GPIOA), 8);
  Serial.print(" |  PortB : ");
  printBits(tpio.readPorts(MCP23017_GPIOB), 8);
  Serial.print(" |\r\n");

  // ADC
  uint16_t value;

  switch (adsMode)
  {
  case Single:
    value = tpio.analogReadSingleEnded(0);
    Serial.print("\033[21;6H");
    printBits(value, 16);
    Serial.print("\033[22;6H");
    Serial.printf("%16u", value);

    value = tpio.analogReadSingleEnded(1);
    Serial.print("\033[21;42H");
    printBits(value, 16);
    Serial.print("\033[22;42H");
    Serial.printf("%16u", value);

    value = tpio.analogReadSingleEnded(2);
    Serial.print("\033[21;80H");
    printBits(value, 16);
    Serial.print("\033[22;80H");
    Serial.printf("%16u", value);

    value = tpio.analogReadSingleEnded(3);
    Serial.print("\033[21;114H");
    printBits(value, 16);
    Serial.print("\033[22;114H");
    Serial.printf("%16u", value);
    break;
  case Differential:
    value = tpio.analogReadDifferential(0);
    Serial.print("\033[21;6H");
    printBits(value, 16);
    Serial.print("\033[22;6H");
    Serial.printf("%16u", value);

    Serial.print("\033[21;42H----------------");
    Serial.print("\033[22;42H----------------");

    value = tpio.analogReadDifferential(1);
    Serial.print("\033[21;80H");
    printBits(value, 16);
    Serial.print("\033[22;80H");
    Serial.printf("%16u", value);

    Serial.print("\033[21;114H----------------");
    Serial.print("\033[22;114H----------------");
    break;
  case Comparator:
    value = tpio.getLastConversionResults();
    Serial.print("\033[21;6H");
    printBits(value, 16);
    Serial.print("\033[22;6H");
    Serial.printf("%16u", value);
    break;
  default:
    break;
  }

  Serial.print("\033[25;3HGain: ");
  switch (tpio.analogGetGain())
  {
  case GAIN_TWOTHIRDS:
    Serial.print("x2/3");
    break;
  case GAIN_ONE:
    Serial.print("x1  ");
    break;
  case GAIN_TWO:
    Serial.print("x2  ");
    break;
  case GAIN_FOUR:
    Serial.print("x4  ");
    break;
  case GAIN_EIGHT:
    Serial.print("x8  ");
    break;
  case GAIN_SIXTEEN:
    Serial.print("x16 ");
    break;
  default:
    break;
  }
}

// routine called when port changes state. This is an alternative to using interrupts
// it requires update be called in the main loop and uses a polling technique generating more I2C calls
// it is however more versatile than interrupts as it can detect multiple simultaneous button presses and supports callbacks
void GPIOChangeCallback(uint16_t ports, uint8_t button, bool state)
{
  callbackFires++;
  Serial.printf("\033[16;0H");
  Serial.printf("| Callback! - Ports: ");
  printBits(ports, 16);
  Serial.printf(" | Button: %03d | State: %s | Fires: %03d | ", button, state ? "TRUE " : "FALSE", callbackFires);

  if (state == true)
    switch (button)
    {
    case 4:
      adsMode = adsMode_t(((int)adsMode) + 1);
      if (adsMode > Comparator)
        adsMode = Single;
      if (adsMode == Comparator)
          tpio.startComparator(0, 500); // ~1.5v
      drawScreen();
      break;
    case 5:
      gainSetting += 0x200;
      if (gainSetting > 0xA00)
        gainSetting = 0x00;
      tpio.analogSetGain((adsGain_t)gainSetting);
      break;
    default:
      break;
    }

  // detect multiple button press
  // ports are pullups and will go low when pressed so we NOT (!) the bit test
  if (!(ports & BUTTON4) && !(ports & BUTTON5))
    Serial.printf("Button 1 & 2 Pressed!");
  else
    Serial.printf("                     ");
}

void setup()
{
  // Serial.begin(460800);
  Serial.begin(115200);

  tpio.begin();
  gainSetting = tpio.analogGetGain();

  tpio.pinMode(0, OUTPUT);
  tpio.pinMode(1, OUTPUT);
  tpio.pinMode(2, OUTPUT);
  tpio.pinMode(3, OUTPUT);

  tpio.pinMode(8, OUTPUT);
  tpio.pinMode(9, OUTPUT);
  tpio.pinMode(10, OUTPUT);
  tpio.pinMode(11, OUTPUT);
  tpio.pinMode(12, OUTPUT);
  tpio.pinMode(13, OUTPUT);
  tpio.pinMode(14, OUTPUT);
  tpio.pinMode(15, OUTPUT);

  tpio.pinMode(4, INPUT);
  tpio.pinMode(5, INPUT);
  tpio.pinMode(6, INPUT);
  tpio.pinMode(7, INPUT);
  tpio.pullUp(4, HIGH);
  tpio.pullUp(5, HIGH);
  tpio.pullUp(6, HIGH);
  tpio.pullUp(7, HIGH);

  tpio.setupInterrupts(false, false, LOW);
  tpio.setupInterruptPin(7, FALLING);
  tpio.setupInterruptPin(6, FALLING);

  pinMode(27, INPUT);
  attachInterrupt(27, InterruptFlag, FALLING);

  // register after pin setup.
  // In this example we only want to create call back events for the buttons and not for the changes made in the togglePorts function
  // to do this we pass in a bit mask to register callback. The value 0xF0 represents ports 4-7 / 0000 0000 1111 0000.
  tpio.RegisterChangeCB(GPIOChangeCallback, 0xF0);

  drawScreen();
}

void loop()
{
  // process interrupt if ISR handler is flagged
  if (handleInterrupt)
  {
    handleInterrupt = false;
    ProcessInterrupt();
  }

  togglePorts();
  updateScreen();

  tpio.update();
}
