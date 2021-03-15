#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

SensirionI2CScd4x scd4x;

typedef enum {
  LOW_POWER,
  HIGH_PERF,
} opMode_t;

opMode_t    opMode          = HIGH_PERF;    // { 1:high performance mode,   0:Low Power operation}
uint16_t    updateInterval  = 5000; // {5s:high performance mode, 30s:Low Power operation}
int         ascState        = 1;    // { 1: enabled, 0: disabled }

void updateOpMode(opMode_t mode) {
  if (mode == HIGH_PERF) {
    opMode = HIGH_PERF;
    updateInterval = 5000;
    Serial.println(F("INFO> switch to High Performance Mode"));
  } else {
    opMode = LOW_POWER;
    updateInterval = 30000;
    Serial.println(F("INFO> switch to Low Power Mode"));
  }
}

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("INFO> SN: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void printErrorMsg(String fucName, uint16_t err) {
  char errMsg[256];

  Serial.print("ERRO> " + fucName + "(): ");
  errorToString(err, errMsg, 256);
  Serial.println(errMsg);
}

void stopPeriodicMeasurement() {
  uint16_t error;

  error = scd4x.stopPeriodicMeasurement();

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    Serial.println(F("INFO> stop periodic measurement"));
  }
}

void startPeriodicMeasurement() {
  uint16_t error;
  String tmp = "";

  if (opMode == HIGH_PERF) {
    error = scd4x.startPeriodicMeasurement();
  } else {
    error = scd4x.startLowPowerPeriodicMeasurement();
  }

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    tmp = (opMode == HIGH_PERF) ? "(High Performance)" : "(Low Power)";
    Serial.print(F("INFO> start periodic measurement "));
    Serial.println(tmp);
  }
}

void getSerialNumber() {
  uint16_t error;
  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;

  error = scd4x.getSerialNumber(serial0, serial1, serial2);

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    printSerialNumber(serial0, serial1, serial2);
  }
}

void getAutomaticSelfCalibration() {
  uint16_t error;
  uint16_t ascEnabled;

  error = scd4x.getAutomaticSelfCalibration(ascEnabled);

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    if (ascEnabled == 1) {
      ascState = 1;
      Serial.println(F("INFO> ASC is enabled"));
    } else if (ascEnabled == 0) {
      ascState = 0;
      Serial.println(F("INFO> ASC is disabled"));
    } else {
      ascState = -1;
      Serial.println(F("WARN> Unknown ASC state"));
    }
  }
}

void setAutomaticSelfCalibration(uint16_t ascEnabled) {
  uint16_t error;

  error = scd4x.setAutomaticSelfCalibration(ascEnabled);

  if (error) {
    printErrorMsg(__func__, error);
  }
}

void readMeasurement() {
  uint16_t error;
  uint16_t co2;
  float temperature;
  float humidity;

#if 0
  delay(updateInterval);
#else
  delay(updateInterval);
  if (getDataReadyStatus() != 1) {
    return;
  }
#endif

  error = scd4x.readMeasurement(co2, temperature, humidity);

  if (error) {
    printErrorMsg(__func__, error);
  } else if (co2 == 0) {
    Serial.println(F("WARN> Invalid sample detected, skipping."));
  } else {
#if 0
    Serial.print(F("Co2:"));
    Serial.print(co2);
    Serial.print(F("\tTemperature:"));
    Serial.print(temperature);
    Serial.print(F("\tHumidity:"));
    Serial.println(humidity);
#else
    Serial.print(co2);
    Serial.print(F(", "));
    Serial.print(temperature);
    Serial.print(F(", "));
    Serial.println(humidity);
#endif
  }
}

void performForcedRecalibration(uint16_t targetCo2Concentration) {
  uint16_t error;
  uint16_t frcCorrection;

  error = scd4x.performForcedRecalibration(targetCo2Concentration, frcCorrection);

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    if (frcCorrection == 0xffff) {
      Serial.print(F("WARN> FRC correction failed!"));
    } else {
      Serial.print(F("INFO> set correction:"));
      Serial.print(targetCo2Concentration);
      Serial.print(F(", offset: "));
      Serial.print((frcCorrection - 32768.0), 2);
      Serial.println(F(" ppm"));
    }
  }
}

void performFactoryReset() {
  uint16_t error;

  error = scd4x.performFactoryReset();

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    Serial.println(F("INFO> perform factory reset"));
  }
}

void getTemperatureOffset() {
  uint16_t error;
  float tOffset;

  error = scd4x.getTemperatureOffset(tOffset);

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    //Serial.println(tOffset);
    Serial.print(F("INFO> offset temperature:"));
    Serial.print(tOffset, 2);
    Serial.println(F("°C"));
  }
}

void setTemperatureOffset(uint16_t tOffset) {
  uint16_t error;

  error = scd4x.setTemperatureOffset(tOffset);

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    Serial.print(F("INFO> set temperature offset: 0x"));
    Serial.println(tOffset, HEX);
  }
}

void persistSettings() {
  uint16_t error;

  error = scd4x.persistSettings();

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    Serial.println(F("INFO> persist settings"));
  }
}

void reinit() {
  uint16_t error;

  error = scd4x.reinit();

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    Serial.println(F("INFO> reinit"));
  }
}

bool getDataReadyStatus() {
  uint16_t error;
  uint16_t dataReady;

  error = scd4x.getDataReadyStatus(dataReady);

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    if ((dataReady & 0xFFF) == 0x0) {
      Serial.println(F("INFO> data NOT ready"));
      return 0;
    } else {
      //Serial.println(F("INFO> data ready"));
      return 1;
    }
  }
}

void quickTest() {
  stopPeriodicMeasurement();
  performFactoryReset();

  getSerialNumber();
  //setAutomaticSelfCalibration(1);
  getAutomaticSelfCalibration();
  //performForcedRecalibration(824);

  getTemperatureOffset();

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
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  Wire.begin();
  scd4x.begin(Wire);

  //quickTest();
  showMenu();
}

void showMenu() {
  String tmp = "";
  Serial.print(
    "\n"
    "\t---------------------------------\n"
    "\t\tSCD40 Menu\n"
    "\t---------------------------------\n"
    "\t1: Sensor Info\n"
    "\t2: Read Measurement\n"
    "\t3: Persist Settings\n"
    "\t4: Factory Reset\n"
  );

  tmp = (ascState == 1) ? "(Enabled)\n" : "(Disabled)\n";
  Serial.print(
    "\t5: Toggle ASC (Automatic self-calibration) " + tmp
  );


  Serial.print(
    "\t6: FRC (Forced Re-Calibration)\n"
    "\t7: Set Temperature Offset\n"
  );


  tmp = (opMode == 1) ? "(High Performance)\n" : "(Low Power)\n";
  Serial.print(
    "\t8: Toggle operation mode " + tmp
  );

  Serial.println();
}

void loop() {
  if (Serial.available() > 0) {
    switch (Serial.read()) {
      case '1': // Sensor Info
        Serial.readStringUntil('\n'); // clean the buffer in serial
        stopPeriodicMeasurement();
        getSerialNumber();
        getAutomaticSelfCalibration();
        getTemperatureOffset();
        break;

      case '2': // Read Measurement
        Serial.readStringUntil('\n'); // clean the buffer in serial
        stopPeriodicMeasurement();
        startPeriodicMeasurement();
        Serial.println(F("INFO> press q to quit\n"));
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

      case '3': // Persist Settings
        Serial.readStringUntil('\n'); // clean the buffer in serial
        stopPeriodicMeasurement();
        persistSettings();
        break;

      case '4': // Factory Reset
        Serial.readStringUntil('\n'); // clean the buffer in serial
        stopPeriodicMeasurement();
        performFactoryReset();
        break;

      case '5': // Toggle ASC (Automatic self-calibration)
        Serial.readStringUntil('\n'); // clean the buffer in serial
        stopPeriodicMeasurement();
        getAutomaticSelfCalibration();

        if (ascState == 1) {
          Serial.println(F("INFO> disable ASC..."));
        } else if (ascState == 0) {
          Serial.println(F("INFO> enable ASC..."));
        }
        ascState = 1 - ascState;
        setAutomaticSelfCalibration(ascState);
        break;

      case '6': // FRC (Forced Re-Calibration)
        Serial.readStringUntil('\n'); // clean the buffer in serial
        stopPeriodicMeasurement();
        Serial.println(F("INFO> input FRC (ppm) from serial..."));

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

      case '7': // Set Temperature Offset
        Serial.readStringUntil('\n'); // clean the buffer in serial
        stopPeriodicMeasurement();
        Serial.println(F("INFO> input temperature offset from serial..."));

        for (;;) {
          if (Serial.available() > 0) {
            String str = Serial.readStringUntil('\n');
            float tOffset = str.toFloat();
            //Serial.println((int)((tOffset * 65536.0) / 175.0));
            setTemperatureOffset(tOffset);
            break;
          }
        }
        break;

      case '8': // Toggle operation mode
        opMode = 1 - opMode;
        updateOpMode(opMode);
        break;

      default: // includes the case 'no input'
        Serial.readStringUntil('\n'); // clean the buffer in serial
        showMenu();
    }
  }
}
