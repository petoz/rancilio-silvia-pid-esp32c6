#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "config.h"
#include <Arduino.h>
#include <SPI.h>

// MAX31865 Register Definitions
#define MAX31865_CONFIG_REG 0x00
#define MAX31865_RTDMSB_REG 0x01
#define MAX31865_RTDLSB_REG 0x02
#define MAX31865_FAULTSTAT_REG 0x07

// Config Bits
#define MAX31865_CONFIG_BIAS 0x80
#define MAX31865_CONFIG_MODEAUTO 0x40
#define MAX31865_CONFIG_1SHOT 0x20
#define MAX31865_CONFIG_3WIRE 0x10
#define MAX31865_CONFIG_FAULTSTAT 0x02

// Reference resistor and nominal resistance
#define RREF 430.0
#define RNOMINAL 100.0

class Temperature {
public:
  Temperature();
  void begin();
  void update();
  float getTemperature();
  uint8_t getFault();
  bool hasFault();
  void clearFault();

private:
  void writeRegister8(uint8_t addr, uint8_t data);
  uint8_t readRegister8(uint8_t addr);
  uint16_t readRegister16(uint8_t addr);
  float calculateTemperature(uint16_t rtdRaw);

  SPIClass *_spi;
  float _currentTemp;
  float _smoothedTemp;
  uint8_t _lastFault;

  enum State { IDLE, WAITING_FOR_BIAS, WAITING_FOR_CONVERSION };
  State _state;
  unsigned long _lastStateChangeTime;
  unsigned long _lastReadAttempt;
  const unsigned long _readInterval = 1000;
};

#endif
