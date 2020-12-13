#ifndef _UM_MCP23017_H_
#define _UM_MCP23017_H_

#include <Arduino.h>
#include <Wire.h>

#define MCP23017_ADDRESS 0x20

// registers
#define MCP23017_IODIRA 0x00
#define MCP23017_IPOLA 0x02
#define MCP23017_GPINTENA 0x04
#define MCP23017_DEFVALA 0x06
#define MCP23017_INTCONA 0x08
#define MCP23017_IOCONA 0x0A
#define MCP23017_GPPUA 0x0C
#define MCP23017_INTFA 0x0E
#define MCP23017_INTCAPA 0x10
#define MCP23017_GPIOA 0x12
#define MCP23017_OLATA 0x14

#define MCP23017_IODIRB 0x01
#define MCP23017_IPOLB 0x03
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALB 0x07
#define MCP23017_INTCONB 0x09
#define MCP23017_IOCONB 0x0B
#define MCP23017_GPPUB 0x0D
#define MCP23017_INTFB 0x0F
#define MCP23017_INTCAPB 0x11
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATB 0x15

#define MCP23017_INT_ERR 255

#define BUTTON0 0x01    // 0000000000000001
#define BUTTON1 0x02    // 0000000000000010
#define BUTTON2 0x04    // 0000000000000100
#define BUTTON3 0x08    // 0000000000001000
#define BUTTON4 0x10    // 0000000000010000
#define BUTTON5 0x20    // 0000000000100000
#define BUTTON6 0x40    // 0000000001000000
#define BUTTON7 0x80    // 0000000010000000
#define BUTTON8 0x100   // 0000000100000000
#define BUTTON9 0x200   // 0000001000000000
#define BUTTON10 0x400  // 0000010000000000
#define BUTTON11 0x800  // 0000100000000000
#define BUTTON12 0x1000 // 0001000000000000
#define BUTTON13 0x2000 // 0010000000000000
#define BUTTON14 0x4000 // 0100000000000000
#define BUTTON15 0x8000 // 1000000000000000

// Event class used for callbacks
class GPIOEvents
{
public:
    GPIOEvents() { ClearCBs(); }
    void ClearCBs() { chngFn = NULL; };

    typedef void (*chngCBFn)(uint16_t ports, uint8_t button, bool state);
    bool RegisterChangeCB(chngCBFn f)
    {
        chngFn = f;
        return true;
    }
    inline void change(uint16_t ports, uint8_t button, bool state)
    {
        if (chngFn)
            chngFn(ports, button, state);
    }

private:
    chngCBFn chngFn;
};

class UM_MCP23017
{
public:
    void begin(void) { begin(MCP23017_ADDRESS); }
    void begin(uint8_t addr);

    void updateRegisterBit(uint8_t pin, uint8_t pValue, uint8_t portAaddr, uint8_t portBaddr);
    bool write(uint8_t addr, uint8_t value);
    uint8_t read(unsigned int addr);

    void digitalWrite(uint8_t pin, uint8_t value);
    uint8_t digitalRead(uint8_t pin);

    void pinMode(uint8_t pin, uint8_t mode);
    void pullUp(uint8_t p, uint8_t d);

    uint16_t readPorts();
    uint8_t readPorts(uint8_t port);

    void setupInterrupts(uint8_t mirrorIntPin, uint8_t openDrain, uint8_t polarity);
    void setupInterruptPin(uint8_t p, uint8_t mode);
    uint8_t getLastInterruptPin();
    uint8_t getLastInterruptPinValue();

    bool RegisterChangeCB(GPIOEvents::chngCBFn fn)
    {
        return RegisterChangeCB(fn, 0xFF);
    }

    bool RegisterChangeCB(GPIOEvents::chngCBFn fn, uint16_t portMask)
    {
        m_prevPorts = readPorts();
        m_callbackPortMask = portMask;
        return m_cb.RegisterChangeCB(fn);
    }

    void update();

private:
    GPIOEvents m_cb;
    uint8_t m_i2cAddress;
    uint16_t m_prevPorts = 0;
    uint16_t m_callbackPortMask = 0xFF;
};

#endif