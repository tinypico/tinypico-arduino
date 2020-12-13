#include "ADS1015.h"

bool UM_ADS1015::write(uint8_t addr, uint16_t value)
{
  Wire.beginTransmission(m_i2cAddress);
  Wire.write(addr);
  Wire.write((uint8_t)(value >> 8));
  Wire.write((uint8_t)(value & 0xFF));
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

uint16_t UM_ADS1015::read(unsigned int addr)
{
  unsigned int value;
  Wire.beginTransmission(m_i2cAddress);
  Wire.write(addr);
  if (Wire.endTransmission(false) != 0)
    return 0;
  if (Wire.requestFrom(m_i2cAddress, (uint8_t)2) < 2)
    return 0;
  return ((Wire.read() << 8) | Wire.read());
}

void UM_ADS1015::begin(uint8_t addr)
{
  m_i2cAddress = addr;
  m_conversionDelay = ADS1015_CONVERSIONDELAY;
  m_bitShift = 4;
  m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */

  Wire.begin();
}

void UM_ADS1015::analogSetGain(adsGain_t gain)
{
  m_gain = gain;
}

adsGain_t UM_ADS1015::analogGetGain()
{
  return m_gain;
}

uint16_t UM_ADS1015::analogReadSingleEnded(uint8_t channel)
{
  if (channel > 3)
  {
    return 0;
  }

  // Start with default values
  uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE |    // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;

  // Set single-ended input channel
  switch (channel)
  {
  case (0):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
    break;
  case (1):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
    break;
  case (2):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
    break;
  case (3):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
    break;
  }

  // Set 'start single-conversion' bit
  config |= ADS1015_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  write(ADS1015_REG_POINTER_CONFIG, config);

  // Wait for the conversion to complete
  delay(m_conversionDelay);

  // Read the conversion results
  // Shift 12-bit results right 4 bits for the ADS1015
  return read(ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
}

int16_t UM_ADS1015::analogReadDifferential(uint8_t channel)
{
  // Start with default values
  uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE |    // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;

  // Set channels
  if (channel == 0)
    config |= ADS1015_REG_CONFIG_MUX_DIFF_0_1; // AIN0 = P, AIN1 = N
  if (channel == 1)
    config |= ADS1015_REG_CONFIG_MUX_DIFF_2_3; // AIN2 = P, AIN3 = N

  // Set 'start single-conversion' bit
  config |= ADS1015_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  write(ADS1015_REG_POINTER_CONFIG, config);

  // Wait for the conversion to complete
  delay(m_conversionDelay);

  // Read the conversion results
  uint16_t res = read(ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
  if (m_bitShift == 0)
  {
    return (int16_t)res;
  }
  else
  {
    // Shift 12-bit results right 4 bits for the ADS1015,
    // making sure we keep the sign bit intact
    if (res > 0x07FF)
    {
      // negative number - extend the sign to 16th bit
      res |= 0xF000;
    }
    return (int16_t)res;
  }
}

void UM_ADS1015::startComparator(uint8_t channel, int16_t threshold)
{
  // Start with default values
  uint16_t config = ADS1015_REG_CONFIG_CQUE_1CONV |   // Comparator enabled and asserts on 1 match
                    ADS1015_REG_CONFIG_CLAT_LATCH |   // Latching mode
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_MODE_CONTIN |  // Continuous conversion mode
                    ADS1015_REG_CONFIG_MODE_CONTIN;   // Continuous conversion mode

  // Set PGA/voltage range
  config |= m_gain;

  // Set single-ended input channel
  switch (channel)
  {
  case (0):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
    break;
  case (1):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
    break;
  case (2):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
    break;
  case (3):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
    break;
  }

  // Set the high threshold register
  // Shift 12-bit results left 4 bits for the ADS1015
  write(ADS1015_REG_POINTER_HITHRESH, threshold << m_bitShift);

  // Write config register to the ADC
  write(ADS1015_REG_POINTER_CONFIG, config);
}

int16_t UM_ADS1015::getLastConversionResults()
{
  // Wait for the conversion to complete
  delay(m_conversionDelay);

  // Read the conversion results
  uint16_t res = read(ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
  if (m_bitShift == 0)
  {
    return (int16_t)res;
  }
  else
  {
    // Shift 12-bit results right 4 bits for the ADS1015,
    // making sure we keep the sign bit intact
    if (res > 0x07FF)
    {
      // negative number - extend the sign to 16th bit
      res |= 0xF000;
    }
    return (int16_t)res;
  }
}
