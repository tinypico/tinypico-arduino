#include "MCP23017.h"

void UM_MCP23017::begin(uint8_t addr)
{
    m_i2cAddress = addr;

    Wire.begin(21, 22);

    // initialise ports
    write(MCP23017_IODIRA, 0xff);
    write(MCP23017_IODIRB, 0xff);
}

void UM_MCP23017::updateRegisterBit(uint8_t pin, uint8_t pValue, uint8_t portAaddr, uint8_t portBaddr)
{
    uint8_t regAddr = (pin < 8) ? portAaddr : portBaddr;
    uint8_t bit = pin % 8;
    uint8_t regValue = read(regAddr);

    // set the value for the particular bit
    bitWrite(regValue, bit, pValue);

    write(regAddr, regValue);
}

bool UM_MCP23017::write(uint8_t addr, uint8_t value)
{
    Wire.beginTransmission(m_i2cAddress);
    Wire.write(addr);
    Wire.write(value);
    if (Wire.endTransmission() == 0)
        return true;
    return false;
}

uint8_t UM_MCP23017::read(unsigned int addr)
{
    unsigned int value;
    Wire.beginTransmission(m_i2cAddress);
    Wire.write(addr);
    if (Wire.endTransmission(false) != 0)
        return 0;
    if (Wire.requestFrom((int)m_i2cAddress, 2) < 2)
        return 0;
    value = Wire.read();
    return value;
}

void UM_MCP23017::digitalWrite(uint8_t pin, uint8_t value)
{
    uint8_t gpio;
    uint8_t bit = pin % 8;

    // read the current GPIO output latches
    uint8_t regAddr = (pin < 8) ? MCP23017_OLATA : MCP23017_OLATB;
    gpio = read(regAddr);

    // set the pin and direction
    bitWrite(gpio, bit, value);

    // write the new GPIO
    regAddr = (pin < 8) ? MCP23017_GPIOA : MCP23017_GPIOB;
    write(regAddr, gpio);
}

uint8_t UM_MCP23017::digitalRead(uint8_t pin)
{
    uint8_t bit = pin % 8;
    uint8_t regAddr = (pin < 8) ? MCP23017_GPIOA : MCP23017_GPIOB;
    return (read(regAddr) >> bit) & 0x1;
}

void UM_MCP23017::pinMode(uint8_t pin, uint8_t mode)
{
    updateRegisterBit(pin, (mode == INPUT), MCP23017_IODIRA, MCP23017_IODIRB);
}

void UM_MCP23017::pullUp(uint8_t p, uint8_t d)
{
    updateRegisterBit(p, d, MCP23017_GPPUA, MCP23017_GPPUB);
}


uint8_t UM_MCP23017::readPorts(uint8_t port)
{
    Wire.beginTransmission(m_i2cAddress);
    Wire.write(port);
    Wire.endTransmission();

    Wire.requestFrom((int)m_i2cAddress, 1);
    return Wire.read();
}

uint16_t UM_MCP23017::readPorts()
{
    uint16_t ba = 0;
    uint8_t a;

    // read the current GPIO output latches
    Wire.beginTransmission(m_i2cAddress);
    Wire.write(MCP23017_GPIOA);
    Wire.endTransmission();
    Wire.requestFrom((int)m_i2cAddress, 2);
    a = Wire.read();
    ba = Wire.read();
    ba <<= 8;
    ba |= a;

    return ba;
}

// INTCON / 0x0A & 0x0B / Configuration Register / Page 20 / 3.5.6
// The IOCON register contains several bits for configuring the device
// Bit 7        6       5       4       3       2       1       0
//     BANK     MIRROR  SEQOP   DISSLW  HAEN    ODR     INTPOL  -
void UM_MCP23017::setupInterrupts(uint8_t mirrorIntPin, uint8_t openDrain, uint8_t polarity)
{
    // configure the port A
    uint8_t ioconfValue = read(MCP23017_IOCONA);
    bitWrite(ioconfValue, 6, mirrorIntPin);
    bitWrite(ioconfValue, 2, openDrain);
    bitWrite(ioconfValue, 1, polarity);
    write(MCP23017_IOCONA, ioconfValue);

    // Configure the port B
    ioconfValue = read(MCP23017_IOCONB);
    bitWrite(ioconfValue, 6, mirrorIntPin);
    bitWrite(ioconfValue, 2, openDrain);
    bitWrite(ioconfValue, 1, polarity);
    write(MCP23017_IOCONB, ioconfValue);
}

void UM_MCP23017::setupInterruptPin(uint8_t pin, uint8_t mode)
{

    // set the pin interrupt control (0 means change, 1 means compare against given value);
    updateRegisterBit(pin, (mode != CHANGE), MCP23017_INTCONA, MCP23017_INTCONB);
    // if the mode is not CHANGE, we need to set up a default value, different value triggers interrupt

    // In a RISING interrupt the default value is 0, interrupt is triggered when the pin goes to 1.
    // In a FALLING interrupt the default value is 1, interrupt is triggered when pin goes to 0.
    updateRegisterBit(pin, (mode == FALLING), MCP23017_DEFVALA, MCP23017_DEFVALB);

    // enable the pin for interrupt
    updateRegisterBit(pin, HIGH, MCP23017_GPINTENA, MCP23017_GPINTENB);
}

// INTFA / 0x0E / Interrupt Flag Register / Page 22 / 3.5.8
// The INTF register reflects the interrupt condition on the port pins of any pin that is enabled for interrupts via the GPINTEN register.
// A set bit indicates that the associated pin caused the interrupt.
// This register is read-only. Writes to this register will be ignored.
uint8_t UM_MCP23017::getLastInterruptPin()
{
    uint8_t intf;

    // try port A
    intf = read(MCP23017_INTFA);

    for (int i = 0; i < 8; i++)
        if (bitRead(intf, i))
            return i;

    // try port B
    intf = read(MCP23017_INTFB);
    for (int i = 0; i < 8; i++)
        if (bitRead(intf, i))
            return i + 8;

    return MCP23017_INT_ERR;
}

// INTCAP / 0x10 & 0x11 . Interrupt Captured Register / Page 23 / 3.5.9
// The INTCAP register captures the GPIO port value at the time the interrupt occurred.
// The register is read-only and is updated only when an interrupt occurs.
// The register remains unchanged until the interrupt is cleared via a read of INTCAP or GPIO.
uint8_t UM_MCP23017::getLastInterruptPinValue()
{
    uint8_t intPin = getLastInterruptPin();
    if (intPin != MCP23017_INT_ERR)
    {
        uint8_t intcapreg = (intPin < 8) ? MCP23017_INTCAPA : MCP23017_INTCAPB;
        uint8_t bit = intPin % 8;
        return (read(intcapreg) >> bit) & (0x01);
    }

    return MCP23017_INT_ERR;
}

void UM_MCP23017::update()
{
    uint16_t ports = readPorts();

    if (ports ^ m_prevPorts) // if change
    for (int i = 0; i < 16; i++)
    {
        if (((ports & m_callbackPortMask) & (1UL << i)) ^ (((m_prevPorts & m_callbackPortMask) & (1UL << i))))
            m_cb.change(ports, i, !(ports & (1UL << i)));
    }
    m_prevPorts = ports;
}