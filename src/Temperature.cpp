#include "Temperature.h"

Temperature::Temperature()
    : _spi(nullptr), _currentTemp(0.0), _smoothedTemp(0.0), _lastFault(0),
      _state(IDLE), _lastStateChangeTime(0), _lastReadAttempt(0) {}

void Temperature::begin() {
  // PIN_SPI_mosi/miso/sck/cs defined in config.h
  // Initialize SPI
  _spi = new SPIClass(
      FSPI); // Use FSPI or HSPI? ESP32-C6 has FSPI (SPI2) usually available?
             // Or just use 'SPI' global object.
             // ESP32-C6 Arduino usually maps 'SPI' to the default bus.

  // Explicitly begin SPI with the defined pins
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_CS);
  pinMode(PIN_SPI_CS, OUTPUT);
  digitalWrite(PIN_SPI_CS, HIGH);

  // Initial Config: 3-Wire, Manual Mode, Filter 50Hz (default)
  // Need to set wires. MAX31865 defaults to 2/4 wire. For 3-wire we usually
  // need to set logic. We will set CONFIG register in update loop or here?
  // Ideally here.

  // Reset
  writeRegister8(MAX31865_CONFIG_REG, 0x00);
  delay(10);

  // Set 3-wire mode if needed (VBIAS off for now)
  // 3-wire: set bit 4 (0x10).
  uint8_t config = MAX31865_CONFIG_3WIRE;
  writeRegister8(MAX31865_CONFIG_REG, config);

  _state = IDLE;
}

void Temperature::update() {
  unsigned long now = millis();

  switch (_state) {
  case IDLE:
    if (now - _lastReadAttempt >= _readInterval) {
      _lastReadAttempt = now;
      // 1. Enable VBIAS + 3WIRE
      uint8_t cfg = readRegister8(MAX31865_CONFIG_REG);
      cfg |= MAX31865_CONFIG_BIAS;
      writeRegister8(MAX31865_CONFIG_REG, cfg);

      _state = WAITING_FOR_BIAS;
      _lastStateChangeTime = now;
    }
    break;

  case WAITING_FOR_BIAS:
    if (now - _lastStateChangeTime >= 10) {
      // 2. Trigger 1-Shot
      uint8_t cfg = readRegister8(MAX31865_CONFIG_REG);
      cfg |= MAX31865_CONFIG_1SHOT;
      cfg |= MAX31865_CONFIG_BIAS; // Ensure bias stays on
      writeRegister8(MAX31865_CONFIG_REG, cfg);

      _state = WAITING_FOR_CONVERSION;
      _lastStateChangeTime = now;
    }
    break;

  case WAITING_FOR_CONVERSION:
    if (now - _lastStateChangeTime >= 65) {
      // 3. Read RTD
      uint16_t rtd = readRegister16(MAX31865_RTDMSB_REG);

      // Read Fault
      uint8_t fault = readRegister8(MAX31865_FAULTSTAT_REG);

      if (fault) {
        _lastFault = fault;
        // Clear fault
        uint8_t cfg = readRegister8(MAX31865_CONFIG_REG);
        cfg &= ~MAX31865_CONFIG_1SHOT;    // Clear shot
        cfg |= MAX31865_CONFIG_FAULTSTAT; // Clear fault bit 1
        writeRegister8(MAX31865_CONFIG_REG, cfg);
      } else {
        _lastFault = 0;
        // Remove fault bit clear just in case
        rtd >>= 1; // Remove fault bit from LSB of RTD

        float temp = calculateTemperature(rtd);

        if (temp > -50.0 && temp < 300.0) {
          _currentTemp = temp;
          if (_smoothedTemp == 0.0) {
            _smoothedTemp = _currentTemp;
          } else {
            _smoothedTemp = (TEMP_EMA_ALPHA * _currentTemp) +
                            ((1.0 - TEMP_EMA_ALPHA) * _smoothedTemp);
          }
        }
      }

      // 4. Disable VBIAS to save power
      uint8_t cfg = readRegister8(MAX31865_CONFIG_REG);
      cfg &= ~MAX31865_CONFIG_BIAS;
      writeRegister8(MAX31865_CONFIG_REG, cfg);

      _state = IDLE;
    }
    break;
  }
}

float Temperature::getTemperature() { return _smoothedTemp; }

uint8_t Temperature::getFault() { return _lastFault; }

bool Temperature::hasFault() { return _lastFault != 0; }

// --- SPI Helpers ---

void Temperature::writeRegister8(uint8_t addr, uint8_t data) {
  // Mode 1: CPOL=0, CPHA=1 ? Or Mode 3.
  // MAX31865 works with Mode 1 or 3.
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(PIN_SPI_CS, LOW);
  SPI.transfer(addr | 0x80); // Write MSB is 1
  SPI.transfer(data);
  digitalWrite(PIN_SPI_CS, HIGH);
  SPI.endTransaction();
}

uint8_t Temperature::readRegister8(uint8_t addr) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(PIN_SPI_CS, LOW);
  SPI.transfer(addr & 0x7F); // Read MSB is 0
  uint8_t ret = SPI.transfer(0xFF);
  digitalWrite(PIN_SPI_CS, HIGH);
  SPI.endTransaction();
  return ret;
}

uint16_t Temperature::readRegister16(uint8_t addr) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(PIN_SPI_CS, LOW);
  SPI.transfer(addr & 0x7F);
  uint16_t msb = SPI.transfer(0xFF);
  uint16_t lsb = SPI.transfer(0xFF);
  digitalWrite(PIN_SPI_CS, HIGH);
  SPI.endTransaction();
  return (msb << 8) | lsb;
}

float Temperature::calculateTemperature(uint16_t RTDraw) {
  // Callendar-Van Dusen equation for PT100
  // Simplified from Adafruit library
  float Rt = RTDraw;
  Rt /= 32768;
  Rt *= RREF;

  // Coefficients for PT100
  float a = 3.9083e-3;
  float b = -5.775e-7;

  float Z1 = -a;
  float Z2 = a * a - (4 * b);
  float Z3 = (4 * b) / RNOMINAL;
  float Z4 = 2 * b;

  float temp = Z2 + (Z3 * Rt);
  temp = (sqrt(temp) + Z1) / Z4;

  if (temp >= 0)
    return temp;

  // For < 0C, needs 4-th order polynomial, but espresso machines rarely freeze
  // and we don't need high precision below 0.
  // We'll stick to positive approximation or fallback.
  // (Adafruit lib does robust check, for now this is fine for PID > 80C)
  return temp;
}
