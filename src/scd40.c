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

#include "scd40.h"
#include "scd_git_version.h"
#include "sensirion_arch_config.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"

#define SCD40_CMD_COMMIT_EEPROM 0x3615
#define SCD40_CMD_COMMIT_EEPROM_DURATION_US 6000000
#define SCD40_CMD_FACTORY_RESET 0x3632
#define SCD40_CMD_READ_AUTO_SELF_CALIBRATION 0x2313
#define SCD40_CMD_READ_MEASUREMENT 0xEC05
#define SCD40_CMD_READ_MEASUREMENT_NUM_TRIES 3
#define SCD40_CMD_READ_FEATURE_SET_VERSION 0x202f
#define SCD40_CMD_READ_SERIAL 0x3682
#define SCD40_CMD_RESET 0x3646
#define SCD40_CMD_SET_ALTITUDE 0x2427
#define SCD40_CMD_SET_AMBIENT_PRESSURE 0xE000
#define SCD40_CMD_SET_FORCED_RECALIBRATION 0x362F
#define SCD40_CMD_SET_TEMPERATURE_OFFSET 0x241D
#define SCD40_CMD_START_PERIODIC_MEASUREMENT_HIGH_PERFORMANCE 0x21B1
#define SCD40_CMD_START_PERIODIC_MEASUREMENT_LOW_POWER 0x21AC
#define SCD40_CMD_START_PERIODIC_MEASUREMENT_ULTRA_LOW_POWER 0x21A7
#define SCD40_CMD_STOP_PERIODIC_MEASUREMENT 0x3F86
#define SCD40_CMD_STOP_PERIODIC_MEASUREMENT_DURATION_USEC 30000
#define SCD40_CMD_SINGLE_SHOT_MEASUREMENT 0x2196
#define SCD40_CMD_SINGLE_SHOT_MEASUREMENT_DURATION_US 5000000  // 5 seconds
#define SCD40_CMD_WRITE_AUTO_SELF_CALIBRATION 0x2416

#define SCD40_CMD_SHORT_DURATION_US 10000

#define SCD40_SERIAL_NUM_WORDS 16

#define SCD40_MAX_BUFFER_WORDS 24
#define SCD40_CMD_SINGLE_WORD_BUF_LEN                                          \
    (SENSIRION_COMMAND_SIZE + SENSIRION_WORD_SIZE + CRC8_LEN)

static int16_t scd40_start_periodic_measurement_cmd(uint16_t cmd) {
    int16_t ret;

    ret = sensirion_i2c_write_cmd_with_args(SCD40_I2C_ADDRESS, cmd, NULL, 0);
    if (ret)
        return ret;

    sensirion_sleep_usec(SCD40_CMD_SHORT_DURATION_US);
    return 0;
}

int16_t scd40_start_periodic_measurement() {
    return scd40_start_periodic_measurement_cmd(
        SCD40_CMD_START_PERIODIC_MEASUREMENT_HIGH_PERFORMANCE);
}

int16_t scd40_start_low_power_periodic_measurement() {
    return scd40_start_periodic_measurement_cmd(
        SCD40_CMD_START_PERIODIC_MEASUREMENT_LOW_POWER);
}

int16_t scd40_start_ultra_low_power() {
    return scd40_start_periodic_measurement_cmd(
        SCD40_CMD_START_PERIODIC_MEASUREMENT_ULTRA_LOW_POWER);
}

int16_t scd40_stop_periodic_measurement() {
    int16_t ret;

    ret = sensirion_i2c_write_cmd(SCD40_I2C_ADDRESS,
                                  SCD40_CMD_STOP_PERIODIC_MEASUREMENT);
    if (ret)
        return ret;

    sensirion_sleep_usec(SCD40_CMD_STOP_PERIODIC_MEASUREMENT_DURATION_USEC);
    return 0;
}

int16_t scd40_read_measurement(uint16_t *co2_ppm, int32_t *temperature,
                               int32_t *humidity) {
    uint16_t words[3];
    int16_t ret;
    int16_t tries;

    for (tries = 0; tries < SCD40_CMD_READ_MEASUREMENT_NUM_TRIES; ++tries) {
        ret = sensirion_i2c_read_words(SCD40_I2C_ADDRESS, words,
                                       SENSIRION_NUM_WORDS(words));
        if (ret == 0)
            break;

        sensirion_sleep_usec(100000); /* wait 100ms before retrying */
    }

    if (ret)
        return ret;

    *co2_ppm = words[0];
    /**
     * formulas for conversion of the sensor signals, optimized for fixed point
     * algebra:
     * Temperature = 175 * S_T / 2^16 - 45
     * Relative Humidity = 100 * S_RH / 2^16
     */
    *temperature = ((21875 * (int32_t)words[1]) >> 13) - 45000;
    *humidity = ((12500 * (int32_t)words[2]) >> 13);
    return 0;
}

int16_t scd40_measure_co2_temperature_and_humidity(void) {
    int16_t ret;

    ret = sensirion_i2c_write_cmd(SCD40_I2C_ADDRESS,
                                  SCD40_CMD_SINGLE_SHOT_MEASUREMENT);
    if (ret)
        return ret;

    sensirion_sleep_usec(SCD40_CMD_SHORT_DURATION_US);
    return 0;
}

int16_t scd40_set_temperature_offset(int32_t temperature_offset) {
    int16_t ret;
    uint16_t offset;

    if (temperature_offset < 0 || temperature_offset > 174760)
        return SCD40_INVALID_VALUE;

    /* offset = temperature_offset * 2^16 / (175 * 1000) ->
     * 65536 / 175000 == 0.3745  ->  0.3745 * 2^3 = 2.996 */
    offset = (uint16_t)(temperature_offset >> 3) * 3;

    ret = sensirion_i2c_write_cmd_with_args(
        SCD40_I2C_ADDRESS, SCD40_CMD_SET_TEMPERATURE_OFFSET, &offset,
        SENSIRION_NUM_WORDS(offset));
    if (ret)
        return ret;

    sensirion_sleep_usec(SCD40_CMD_SHORT_DURATION_US);
    return 0;
}

int16_t scd40_set_altitude(uint16_t altitude_meters) {
    int16_t ret;

    ret = sensirion_i2c_write_cmd_with_args(
        SCD40_I2C_ADDRESS, SCD40_CMD_SET_ALTITUDE, &altitude_meters,
        SENSIRION_NUM_WORDS(altitude_meters));
    if (ret)
        return ret;

    sensirion_sleep_usec(SCD40_CMD_SHORT_DURATION_US);
    return 0;
}

int16_t scd40_set_ambient_pressure(uint16_t pressure_pascal) {
    int16_t ret;
    const uint16_t sig_p = pressure_pascal * 100;

    if (pressure_pascal > 655)
        return SCD40_INVALID_VALUE;

    ret = sensirion_i2c_write_cmd_with_args(SCD40_I2C_ADDRESS,
                                            SCD40_CMD_SET_AMBIENT_PRESSURE,
                                            &sig_p, SENSIRION_NUM_WORDS(sig_p));
    if (ret)
        return ret;

    sensirion_sleep_usec(SCD40_CMD_SHORT_DURATION_US);
    return 0;
}

int16_t scd40_get_automatic_self_calibration(uint8_t *asc_enabled) {
    uint16_t word;
    int16_t ret;

    ret = sensirion_i2c_delayed_read_cmd(
        SCD40_I2C_ADDRESS, SCD40_CMD_READ_AUTO_SELF_CALIBRATION,
        SCD40_CMD_SHORT_DURATION_US, &word, SENSIRION_NUM_WORDS(word));
    if (ret != STATUS_OK)
        return ret;

    *asc_enabled = (uint8_t)word;

    return STATUS_OK;
}

int16_t scd40_enable_automatic_self_calibration(uint8_t enable_asc) {
    int16_t ret;
    uint16_t asc = !!enable_asc;

    ret = sensirion_i2c_write_cmd_with_args(
        SCD40_I2C_ADDRESS, SCD40_CMD_WRITE_AUTO_SELF_CALIBRATION, &asc,
        SENSIRION_NUM_WORDS(asc));
    sensirion_sleep_usec(SCD40_CMD_SHORT_DURATION_US);

    return ret;
}

int16_t scd40_set_forced_recalibration(uint16_t co2_ppm) {
    int16_t ret;

    ret = sensirion_i2c_write_cmd_with_args(
        SCD40_I2C_ADDRESS, SCD40_CMD_SET_FORCED_RECALIBRATION, &co2_ppm,
        SENSIRION_NUM_WORDS(co2_ppm));
    sensirion_sleep_usec(SCD40_CMD_SHORT_DURATION_US);

    return ret;
}

int16_t scd40_read_serial(uint64_t *serial) {
    int16_t ret;
    uint16_t words[3];

    ret = sensirion_i2c_delayed_read_cmd(
        SCD40_I2C_ADDRESS, SCD40_CMD_READ_SERIAL, SCD40_CMD_SHORT_DURATION_US,
        words, SENSIRION_NUM_WORDS(words));
    if (ret)
        return ret;

    *serial = (((uint64_t)words[0] << 4) | ((uint64_t)words[1] << 2) |
               (uint64_t)words[2]);
    return 0;
}

int16_t scd40_probe(void) {
    /* try to stop a pending measurement */
    return scd40_stop_periodic_measurement();
}

int16_t scd40_read_feature_set_version(uint16_t *feature_set) {
    return sensirion_i2c_delayed_read_cmd(
        SCD40_I2C_ADDRESS, SCD40_CMD_READ_FEATURE_SET_VERSION,
        SCD40_CMD_SHORT_DURATION_US, feature_set,
        SENSIRION_NUM_WORDS(*feature_set));
}

int16_t scd40_factory_reset(void) {
    int16_t ret;

    ret = sensirion_i2c_write_cmd(SCD40_I2C_ADDRESS, SCD40_CMD_FACTORY_RESET);
    if (ret)
        return ret;

    sensirion_sleep_usec(SCD40_CMD_COMMIT_EEPROM_DURATION_US);
    return 0;
}

int16_t scd40_reset(void) {
    int16_t ret;

    ret = sensirion_i2c_write_cmd(SCD40_I2C_ADDRESS, SCD40_CMD_RESET);
    if (ret)
        return ret;

    sensirion_sleep_usec(SCD40_CMD_SHORT_DURATION_US);
    return 0;
}

int16_t scd40_persist_settings(void) {
    int16_t ret;

    ret = sensirion_i2c_write_cmd(SCD40_I2C_ADDRESS, SCD40_CMD_COMMIT_EEPROM);
    if (ret)
        return ret;

    sensirion_sleep_usec(SCD40_CMD_COMMIT_EEPROM_DURATION_US);
    return 0;
}
