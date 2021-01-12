#include <Arduino.h>
#include "scd40.h"

uint16_t co2_ppm;
int32_t  temperature, relative_humidity;
int16_t  err;
uint16_t interval_in_seconds = 2;

void setup() {
  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }

  Serial.print(F("Initialize I2C..."));
  sensirion_i2c_init();
  Serial.println(F("done."));

  /* Busy loop for initialization, because the main loop does not work without
    a sensor.
  */
  while (scd40_probe() != STATUS_OK) {
    Serial.println(F("SCD40 sensor probing failed!\n"));
    sensirion_sleep_usec(1000000u);
  }
  Serial.println(F("SCD40 sensor probing successful.\n"));

  sensirion_sleep_usec(20000u);
  scd40_start_periodic_measurement();
  sensirion_sleep_usec(2 * interval_in_seconds * 1000000u);

  /* co2 concentration: ppm
     temperature: °C
     humidity: %RH
  */
  Serial.println(F("ppm\t°C\t%RH"));
  Serial.println(F("----------------------------"));
}

void stop_scd40() {
  scd40_stop_periodic_measurement();
  sensirion_i2c_release();
}

void loop() {
  // adapt the code from the main() method content from sxx30_example_usage.c
  // make sure to replace any sleep(x) calls with delay(x * 1000)

  err = scd40_read_measurement(&co2_ppm, &temperature, &relative_humidity);
  if (err != STATUS_OK) {
    Serial.println(F("error reading measurement!\n"));

  } else {
    Serial.print(co2_ppm);
    Serial.print(F("\t"));
    Serial.print(temperature / 1000.0f);
    Serial.print(F("\t"));
    Serial.print(relative_humidity / 1000.0f);
    Serial.println(F("\t"));
  }
  sensirion_sleep_usec(interval_in_seconds * 1000000u);
}
