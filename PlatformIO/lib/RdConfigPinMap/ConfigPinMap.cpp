// Config Pin Map (maps as string to a pin on a processor)
// Rob Dobson 2016-2018

#include "Arduino.h"
#include "ConfigPinMap.h"

#if defined(ESP32)

const char *ConfigPinMap::_pinMapOtherStr[] = {"DAC1", "DAC2", "SCL", "SDA", "RX", "TX", "MISO", "MOSI", "SCK"};
int ConfigPinMap::_pinMapOtherPin[] = {DAC1, DAC2, SCL, SDA, RX, TX, MISO, MOSI, SCK};
int ConfigPinMap::_pinMapOtherLen = sizeof(ConfigPinMap::_pinMapOtherPin) / sizeof(int);
int ConfigPinMap::_pinMapD[] = {};
int ConfigPinMap::_pinMapDLen = sizeof(ConfigPinMap::_pinMapD) / sizeof(int);
int ConfigPinMap::_pinMapA[] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12};
int ConfigPinMap::_pinMapALen = sizeof(ConfigPinMap::_pinMapA) / sizeof(int);

#else

int ConfigPinMap::_pinMapOtherStr[] = {};
int ConfigPinMap::_pinMapOtherPin[] = {};
int ConfigPinMap::_pinMapOtherLen = sizeof(ConfigPinMap::_pinMapOther) / sizeof(int);
;

#if PLATFORM_ID == 6    // Photon
int ConfigPinMap::_pinMapD[] = {D0, D1, D2, D3, D4, D5, D6, D7};
int ConfigPinMap::_pinMapDLen = sizeof(ConfigPinMap::_pinMapD) / sizeof(int);
int ConfigPinMap::_pinMapA[] = {A0, A1, A2, A3, A4, A5, A6, A7};
int ConfigPinMap::_pinMapALen = sizeof(ConfigPinMap::_pinMapA) / sizeof(int);
#elif PLATFORM_ID == 88 // RedBear Duo
int ConfigPinMap::_pinMapD[] = {D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, D16, D17};
int ConfigPinMap::_pinMapDLen = sizeof(ConfigPinMap::_pinMapD) / sizeof(int);
int ConfigPinMap::_pinMapA[] = {A0, A1, A2, A3, A4, A5, A6, A7};
int ConfigPinMap::_pinMapALen = sizeof(ConfigPinMap::_pinMapA) / sizeof(int);
#else
int ConfigPinMap::_pinMapD[] = {0, 1, 2, 3, 4, 5, 6, 7};
int ConfigPinMap::_pinMapDLen = sizeof(ConfigPinMap::_pinMapD) / sizeof(int);
int ConfigPinMap::_pinMapA[] = {10, 11, 12, 13, 14, 15, 16, 17};
int ConfigPinMap::_pinMapALen = sizeof(ConfigPinMap::_pinMapA) / sizeof(int);
#endif

#endif