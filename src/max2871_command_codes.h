#pragma once
#include <stdint.h>

const static uint8_t CmdGeneral = 0x00U;
const static uint8_t CmdRfOff = 0x01U;
const static uint8_t CmdRfPowerMinus4 = 0x02U;
const static uint8_t CmdRfPowerMinus1 = 0x03U;
const static uint8_t CmdRfPowerPlus2 = 0x04U;
const static uint8_t CmdRfPowerPlus5 = 0x05U;
const static uint8_t CmdSetFrequency = 0x06U;
const static uint8_t CmdMuxTriState = 0x07U;
const static uint8_t CmdMuxDigitalLockDetect = 0x08U;
const static uint8_t CmdDivaMode = 0x09U;
