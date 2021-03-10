#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include "U8glib.h"

U8GLIB_SSD1306_128X64   u8g(13, 11, 10, 9); // SW SPI Com: SCK = 13, MOSI = 11, CS = 10, A0/DC = 9, RES/RST = arduino RST pin
#define RST_PIN         8                   // if you want to manually control by GPIO pin

SensirionI2CScd4x       scd4x;

uint8_t     opMode          = 1;    // { 1:high performance mode,   0:Low Power operation}
uint16_t    updateInterval  = 5000; // {5s:high performance mode, 30s:Low Power operation}
int         ascState        = 1;    // { 1: enabled, 0: disabled }

uint16_t    co2;
float       temperature;
float       humidity;

void updateOpMode(uint8_t mode) {
  if (mode == 1) {
    opMode = 1;
    updateInterval = 5000;
    Serial.println(F("INFO> switch to High Performance Mode"));
  } else {
    opMode = 0;
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

  if (opMode == 1) {
    error = scd4x.startPeriodicMeasurement();
  } else {
    error = scd4x.startLowPowerPeriodicMeasurement();
  }

  if (error) {
    printErrorMsg(__func__, error);
  } else {
    tmp = (opMode == 1) ? "(High Performance)" : "(Low Power)";
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

  delay(updateInterval);

  error = scd4x.readMeasurement(co2, temperature, humidity);

  if (error) {
    printErrorMsg(__func__, error);
    co2 = 0;    /* unsigned */
    temperature = -1;
    humidity = -1;
  } else if (co2 == 0) {
    Serial.println(F("WARN> Invalid sample detected, skipping."));
    temperature = 0;
    humidity = 0;
  } else {
    Serial.print(F("Co2:"));
    Serial.print(co2);
    Serial.print(F("\tTemperature:"));
    Serial.print(temperature);
    Serial.print(F("\tHumidity:"));
    Serial.println(humidity);
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

void configSCDx() {
  //performFactoryReset();

  //setAutomaticSelfCalibration(1);
  //performForcedRecalibration(405);
  //setTemperatureOffset(5);
  updateOpMode(1);

  getSerialNumber();
  getAutomaticSelfCalibration();
  getTemperatureOffset();

  //persistSettings();
  //reinit();
}

void clearScreen(void) {
  u8g.firstPage();
  do {
  } while (u8g.nextPage());
}

void drawData(void) {
  u8g.firstPage();
  do {
    //u8g.setFont(u8g_font_unifont);
    u8g.setFont(u8g_font_9x18B);

    u8g.setPrintPos(0, 20);
    u8g.print("CO2:");
    if (co2 == 0) {
      u8g.print("err ");
    } else {
      u8g.print(co2);
    }
    u8g.print("ppm");

    u8g.setPrintPos(0, 40);
    u8g.print("TMP:");
    if (temperature < 0) {
      u8g.print("err ");
    } else {
      u8g.print(temperature);
    }
    u8g.print("C");

    u8g.setPrintPos(0, 60);
    u8g.print("HUM:");
    if (humidity < 0) {
      u8g.print("err ");
    } else {
      u8g.print(humidity);
    }
    u8g.print("%");
  } while (u8g.nextPage());
}

void drawTest(void) {
  u8g.firstPage();
  do {
    // graphic commands to redraw the complete screen should be placed here
    u8g.setFont(u8g_font_unifont);
    u8g.setPrintPos(0, 20);
    char *tmp = "Hello World !";
    u8g.print(tmp);
    u8g.setPrintPos(0, 40);
    u8g.print(millis());
    u8g.setPrintPos(0, 60);
    u8g.print("123456789ABCDEF");
  } while ( u8g.nextPage() );
  delay(1000);
}

void resetOLED() {
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);
  delay(100);
  digitalWrite(RST_PIN, HIGH);
}

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  Wire.begin();
  scd4x.begin(Wire);

  stopPeriodicMeasurement();
  configSCDx();
  startPeriodicMeasurement();


  resetOLED();
}

void loop() {
  readMeasurement();
  drawData();
}
