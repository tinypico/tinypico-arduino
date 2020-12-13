#include "TinyPICOExpander.h"

const uint8_t sdCardSelect = 5;
const uint8_t sdCardDetect = 33;


TinyPICOExpander::TinyPICOExpander() {
    ads = new UM_ADS1015();
    mcp = new UM_MCP23017();
}

void TinyPICOExpander::begin()
{
    mcp->begin();
    ads->begin();
}

void TinyPICOExpander::begin(uint8_t MCP_I2C_Address, uint8_t ADS_I2C_Address)
{
    mcp->begin(MCP_I2C_Address);
    ads->begin(ADS_I2C_Address);
}
