#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

SensirionI2CScd4x scd4x;

const int update_interval = 5000;

char menu_banner[] = "\n"
                     "# SCD40 Menu\n"
                     "# -----------------------------------------\n"
                     "# 1: Serial Number\n"
                     "# 2: ASC (Automatic Self Calibration) state\n"
                     "# 3: FRC (Forced Re-Calibration)\n"
                     "# 4: Factory Reset\n"
                     "# 5: Read Measurement\n"
                     "\n";

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("SN: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void stopPeriodicMeasurement() {
  uint16_t error;
  char errorMessage[256];

  error = scd4x.stopPeriodicMeasurement();

  if (error) {
    Serial.print(F("Error trying to execute stopPeriodicMeasurement(): "));
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.println(F("Stop Periodic Measurement."));
  }
}

void startPeriodicMeasurement() {
  uint16_t error;
  char errorMessage[256];

  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.println(F("Start Periodic Measurement."));
  }
}

void getSerialNumber() {
  uint16_t error;
  char errorMessage[256];

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;

  error = scd4x.getSerialNumber(serial0, serial1, serial2);

  if (error) {
    Serial.print(F("Error trying to execute getSerialNumber(): "));
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    printSerialNumber(serial0, serial1, serial2);
  }
}

void getAutomaticSelfCalibration() {
  uint16_t error;
  char errorMessage[256];

  uint16_t ascEnabled;

  error = scd4x.getAutomaticSelfCalibration(ascEnabled);

  if (error) {
    Serial.print(F("Error trying to execute getAutomaticSelfCalibration(): "));
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    //Serial.println(ascEnabled, HEX);
    if (ascEnabled == 1) {
      Serial.println(F("ASC enabled"));
    } else if (ascEnabled == 0) {
      Serial.println(F("ASC not enabled"));
    } else {
      Serial.println(F("ASC unknown!"));
    }
  }
}

void setAutomaticSelfCalibration(uint16_t ascEnabled) {
  uint16_t error;
  char errorMessage[256];

  error = scd4x.setAutomaticSelfCalibration(ascEnabled);

  if (error) {
    Serial.print(F("Error trying to execute setAutomaticSelfCalibration(): "));
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
}

void readMeasurement() {
  uint16_t error;
  char errorMessage[256];

  delay(update_interval);

  uint16_t co2;
  uint16_t temperature;
  uint16_t humidity;

  error = scd4x.readMeasurement(co2, temperature, humidity);

  if (error) {
    Serial.print(F("Error trying to execute readMeasurement(): "));
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else if (co2 == 0) {
    Serial.println(F("Invalid sample detected, skipping."));
  } else {
    Serial.print(F("Co2:"));
    Serial.print(co2);
    Serial.print(F("\tTemperature:"));
    Serial.print(temperature * 175.0 / 65536.0 - 45.0);
    Serial.print(F("\tHumidity:"));
    Serial.println(humidity * 100.0 / 65536.0);
  }
}

void performForcedRecalibration(uint16_t targetCo2Concentration) {
  uint16_t error;
  char errorMessage[256];
  uint16_t frcCorrection;

  error = scd4x.performForcedRecalibration(targetCo2Concentration, frcCorrection);

  if (error) {
    Serial.print(F("Error trying to execute performForcedRecalibration(): "));
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    if (frcCorrection == 0xffff) {
      Serial.print(F("FRC correction failed!"));
    } else {
      Serial.print(F("set FRC:"));
      Serial.print(targetCo2Concentration);
      Serial.print(F(", FRC offset: "));
      Serial.print((frcCorrection - 32768), DEC);
      Serial.println(F("ppm"));
    }
  }
}

void performFactoryReset() {
  uint16_t error;
  char errorMessage[256];

  error = scd4x.performFactoryReset();

  if (error) {
    Serial.print(F("Error trying to execute performFactoryReset(): "));
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.println(F("Perform Factory Reset."));
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  Wire.begin();

  scd4x.begin(Wire);

#if 0
  //-------------------------------
  stopPeriodicMeasurement();
  performFactoryReset();

  getSerialNumber();
  //setAutomaticSelfCalibration(1);
  getAutomaticSelfCalibration();
  //performForcedRecalibration(824);

  //-------------------------------
  startPeriodicMeasurement();
  for (int i = 0; i < 20; i++) {
    readMeasurement();
  }
  stopPeriodicMeasurement();

  //-------------------------------
  performForcedRecalibration(824);
  startPeriodicMeasurement();
  for (int i = 0; i < 20; i++) {
    readMeasurement();
  }
  stopPeriodicMeasurement();
#endif
}

void showMenu() {
  Serial.print(menu_banner);
  for (;;) {
    switch (Serial.read()) {

      case '1':
        stopPeriodicMeasurement();
        getSerialNumber();
        break;

      case '2':
        stopPeriodicMeasurement();
        getAutomaticSelfCalibration();
        break;

      case '3':
        stopPeriodicMeasurement();
        Serial.readStringUntil('\n');
        Serial.println(F("input FRC (ppm)..."));

        for (;;) {
          if (Serial.available() > 0) {
            String str = Serial.readStringUntil('\n');
            int ppm = str.toInt();
            //Serial.println(ppm, DEC);
            performForcedRecalibration(ppm);
            break;
          }
        }
        break;

      case '4':
        stopPeriodicMeasurement();
        performFactoryReset();
        break;


      case '5':
        stopPeriodicMeasurement();
        startPeriodicMeasurement();
        Serial.println(F("q for exit.\n"));
        for (;;) {
          readMeasurement();
          if (Serial.available() > 0) {
            if (Serial.read() == 'q') {
              stopPeriodicMeasurement();
              break;
            }
          }
        }
        break;

      default:
        delay(100);
        continue;  // includes the case 'no input'
    }
    break;
  }
}

void loop() {
  showMenu();
}
