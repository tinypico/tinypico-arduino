#ifndef _UM_IOEXPANDER_H_
#define _UM_IOEXPANDER_H_

#include <Arduino.h>
#include "MCP23017.h"
#include "ADS1015.h"

class TinyPICOExpander
{
public:
    TinyPICOExpander();
    void begin();
    void begin(uint8_t MCP_I2C_Address, uint8_t ADS_I2C_Address);

    // digital
    void digitalWrite(uint8_t pin, uint8_t value) { mcp->digitalWrite(pin, value); }
    uint8_t digitalRead(uint8_t pin) { return mcp->digitalRead(pin); }
    void pinMode(uint8_t pin, uint8_t mode) { mcp->pinMode(pin, mode); }
    void pullUp(uint8_t p, uint8_t d) { mcp->pullUp(p, d); }
    uint16_t readPorts() { return mcp->readPorts(); }
    uint8_t readPorts(uint8_t port) { return mcp->readPorts(port); }
    void setupInterrupts(uint8_t mirrorIntPin, uint8_t openDrain, uint8_t polarity) { mcp->setupInterrupts(mirrorIntPin, openDrain, polarity); }
    void setupInterruptPin(uint8_t p, uint8_t mode) { mcp->setupInterruptPin(p, mode); }
    uint8_t getLastInterruptPin() { return mcp->getLastInterruptPin(); }
    uint8_t getLastInterruptPinValue() { return mcp->getLastInterruptPinValue(); }

    bool RegisterChangeCB(GPIOEvents::chngCBFn fn) { return mcp->RegisterChangeCB(fn); }
    bool RegisterChangeCB(GPIOEvents::chngCBFn fn, uint16_t portMask) { return mcp->RegisterChangeCB(fn, portMask); }

    // analog
    uint16_t analogReadSingleEnded(uint8_t channel) { return ads->analogReadSingleEnded(channel); }
    int16_t analogReadDifferential(uint8_t channel) { return ads->analogReadDifferential(channel); }
    void startComparator(uint8_t channel, int16_t threshold) { ads->startComparator(channel, threshold); }
    int16_t getLastConversionResults() { return ads->getLastConversionResults(); }
    void analogSetGain(adsGain_t gain) { ads->analogSetGain(gain); }
    adsGain_t analogGetGain(void) { return ads->analogGetGain(); }

    void update() { mcp->update(); };

private:
    UM_ADS1015 *ads;
    UM_MCP23017 *mcp;
};

#endif