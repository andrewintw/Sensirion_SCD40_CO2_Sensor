/*
 * Copyright (c) 2019, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SCD40_H
#define SCD40_H
#include "sensirion_arch_config.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCD40_I2C_ADDRESS 0x62

#define SCD40_INVALID_VALUE (-(1 << 14))

#define SCD40_FEATURE_SET_MAJOR_VERSION(fs) (((fs)&0xE0) >> 5)
#define SCD40_FEATURE_SET_MINOR_VERSION(fs) ((fs)&0x1F)

/**
 * scd40_probe() - check if the SCD sensor is available and initialize it
 *
 * Note that as part of the initialization, any ongoing measurements are stopped
 * with `scd40_stop_periodic_measurement()` since the sensor does not accept
 * commands when a periodic measurement is running.
 *
 * @return  0 on success, an error code otherwise.
 */
int16_t scd40_probe(void);

/**
 * scd40_start_periodic_measurement() - Start continuous measurement to measure
 * CO2 concentration in the highest possible accuracy, relative humidity and
 * temperature.
 *
 * Measurement data which is not read from the sensor is continuously
 * overwritten.
 * The continuous measurement status is saved in non-volatile memory. The last
 * measurement mode is resumed after repowering.
 *
 * @return  0 if the command was successful, an error code otherwise
 */
int16_t scd40_start_periodic_measurement(void);

/**
 * scd40_start_low_power_periodic_measurement - Start measurement in low power
 * mode.
 *
 * @return  0 if the command was successful, an error code otherwise
 */
int16_t scd40_start_low_power_periodic_measurement(void);

/**
 * scd40_start_ultra_low_power_periodic_measurement - Start measurement in
 * ultra low power mode.
 *
 * @return  0 if the command was successful, an error code otherwise
 */
int16_t scd40_start_ultra_low_power_periodic_measurement(void);

/**
 * scd40_stop_periodic_measurement() - Stop the continuous measurement
 *
 * @return  0 if the command was successful, an error code otherwise
 */
int16_t scd40_stop_periodic_measurement(void);

/**
 * scd40_read_measurement() - Read out the last measurement from an ongoing
 * periodic measurement or from a single shot measurement
 *
 * Temperature is returned in [degree Celsius], multiplied by 1000,
 * and relative humidity in [percent relative humidity], multiplied by 1000.
 *
 * @param co2_ppm       CO2 concentration in ppm
 * @param temperature   the address for the result of the temperature
 *                      measurement
 * @param humidity      the address for the result of the relative humidity
 *                      measurement
 *
 * @return              0 if the command was successful, an error code otherwise
 */
int16_t scd40_read_measurement(uint16_t *co2_ppm, int32_t *temperature,
                               int32_t *humidity);

/**
 * scd40_measure_co2_temperature_and_humidity - Start a single-shot measurement
 *
 * A single-shot measurement is available after
 * SCD40_READ_CO2_TEMPERATURE_AND_HUMIDITY_DURATION_USEC micoseconds
 * with the method `scd40_read_measurement`
 *
 * @return              0 if the command was successful, an error code otherwise
 */
int16_t scd40_measure_co2_temperature_and_humidity(void);

/**
 * scd40_set_temperature_offset() - Set the temperature offset
 *
 * The on-board RH/T sensor is influenced by thermal self-heating of SCD40 and
 * other electrical components. Design-in alters the thermal properties of SCD40
 * such that temperature and humidity offsets may occur when operating the
 * sensor in end-customer devices. Compensation of those effects is achievable
 * by writing the temperature offset found in continuous operation of the device
 * into the sensor.
 *
 * @param temperature_offset    Temperature offset in [degree Celsius],
 *                              multiplied by 1000, i.e. one tick corresponds to
 *                              0.001 degrees C.
 *                              Only positive values and values < 175C are
 *                              permissible, otherwise SCD40_INVALID_VALUE is
 *                              returned.
 *
 * @return                      0 if the command was successful, an error code
 *                              otherwise
 */
int16_t scd40_set_temperature_offset(int32_t temperature_offset);

/**
 * scd40_set_altitude() - Set the altitude above sea level
 *
 * Measurements of CO2 concentration are influenced by altitude. When a value is
 * set, the altitude-effect is compensated. The altitude setting is disregarded
 * when an ambient pressure is set on the sensor with
 * scd40_start_periodic_measurement.
 * The altitude is saved in non-volatile memory. The last set value will be used
 * after repowering.
 *
 * @param altitude  altitude in meters above sea level, 0 meters is the default
 *                  value and disables altitude compensation
 *
 * @return          0 if the command was successful, an error code otherwise
 */
int16_t scd40_set_altitude(uint16_t altitude_meters);

/**
 * scd40_set_ambient_pressure() - Set the ambient pressure
 *
 * Measurements of CO2 concentration are influenced by pressure. When a value is
 * set, the pressure-effect is compensated. The altitude setting is disregarded
 * when an ambient pressure is set on the sensor.
 *
 * @param pressure_pascal   The ambient pressure in Pascal
 *
 * @return              0 if the command was successful, an error code otherwise
 */
int16_t scd40_set_ambient_pressure(uint16_t pressure_pascal);

/**
 * scd40_get_automatic_self_calibration() - Read if the sensor's automatic self
 * calibration is enabled or disabled
 *
 * See scd40_enable_automatic_self_calibration() for more details.
 *
 * @param asc_enabled   Pointer to memory of where to set the self calibration
 *                      state. 1 if ASC is enabled, 0 if ASC disabled. Remains
 *                      untouched if return is non-zero.
 *
 * @return              0 if the command was successful, an error code otherwise
 */
int16_t scd40_get_automatic_self_calibration(uint8_t *asc_enabled);

/**
 * scd40_enable_automatic_self_calibration() - Enable or disable the sensor's
 * automatic self calibration
 *
 * When activated for the first time a period of minimum 7 days is needed so
 * that the algorithm can find its initial parameter set for ASC.
 * The sensor has to be exposed to fresh air for at least 1 hour every day.
 * Refer to the datasheet for further conditions
 *
 * ASC status is saved in non-volatile memory. When the sensor is powered down
 * while ASC is activated SCD40 will continue with automatic self-calibration
 * after repowering without sending the command.
 *
 * @param enable_asc    enable ASC if non-zero, disable otherwise
 *
 * @return              0 if the command was successful, an error code otherwise
 */
int16_t scd40_enable_automatic_self_calibration(uint8_t enable_asc);

/**
 * scd40_set_forced_recalibration() - Forcibly recalibrate the sensor to a known
 * value.
 *
 * Forced recalibration (FRC) is used to compensate for sensor drifts when a
 * reference value of the CO2 concentration in close proximity to the SCD40 is
 * available.
 *
 * For best results the sensor has to be run in a stable environment in
 * continuous mode at a measurement rate of 2s for at least two minutes before
 * applying the calibration command and sending the reference value.
 * Setting a reference CO2 concentration will overwrite the settings from ASC
 * (see scd40_enable_automatic_self_calibration) and vice-versa. The reference
 * CO2 concentration has to be in the range 400..2000 ppm.
 *
 * FRC value is saved in non-volatile memory, the last set FRC value will be
 * used for field-calibration after repowering.
 *
 * @param co2_ppm   recalibrate to this specific co2 concentration
 *
 * @return          0 if the command was successful, an error code otherwise
 */
int16_t scd40_set_forced_recalibration(uint16_t co2_ppm);

/**
 * scd40_read_serial() - Read out the serial number
 *
 * Note that only the lower 48 bits are returned from the sensor, the upper bits
 * are always 0.
 *
 * @param serial    the address for the result of the serial number.
 * @return          0 if the command was successful, else an error code.
 */
int16_t scd40_read_serial(uint64_t *serial);

/**
 * scd40_read_feature_set_version() - Read the feature set version
 *
 * @param feature_set   the address for the result of the feature set version.
 * @return              0 on success, an error code otherwise.
 */
int16_t scd40_read_feature_set_version(uint16_t *feature_set);

/**
 * scd40_factory_reset - Reset all settings to factory default
 *
 * @return  0 on success, an error code otherwise.
 */
int16_t scd40_factory_reset(void);

/**
 * scd40_reset - Reset (aka restart or reinit) the sensor
 *
 * Imitates a soft reset of the sensor. Before sending the command, the stop
 * measurement command must be issued.
 *
 * @return  0 on success, an error code otherwise.
 */
int16_t scd40_reset(void);

/**
 * scd40_persist_settings - Commit all settings to the sensor's internal
 * persistance
 *
 * Configurations on the SCD40 are by default stored in the volatile memory only
 * and will be lost after a power-cycle. This command stores current
 * configurations in the EEPROM of the SCD40, making them resistant to
 * power-cycling.
 * Note that the command should only be sent after all configuration are set to
 * minimize the number of write/erase cycles on the EEPROM.
 *
 * @return  0 on success, an error code otherwise.
 */
int16_t scd40_persist_settings(void);

#ifdef __cplusplus
}
#endif

#endif /* SCD40_H */
