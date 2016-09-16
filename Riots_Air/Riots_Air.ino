/*
 * This file is part of Riots.
 * Copyright © 2016 Riots Global OY; <copyright@myriots.com>
 *
 * Riots is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.
 *
 * Riots is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with Riots.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <Riots_BabyRadio.h>
#include <Riots_SHT21.h>
#include <Riots_BMP280.h>

Riots_BabyRadio riots_radio;
Riots_SHT21 riots_sht21;
Riots_BMP280 riots_bmp280;

unsigned long next_measurement_time;
unsigned long next_reporting_time;
unsigned long next_battery_measurement_time = 0;

int32_t temperature;
int32_t humidity;
int32_t pressure;
int32_t temperature_previous = -99999;
int32_t humidity_previous = -99999;
int32_t pressure_previous = -99999;

byte radio_status = RIOTS_OK;

void setup() {

  /* Setup Riots radio, with led indications and without debuging. */
  riots_radio.setup(1,0);

  /* Setup BMP280 */
  if( riots_bmp280.setup() != RIOTS_OK ) {
    /* did not work, stop here */
    while(1);
  }

  /* Setup SHT21 */
  if( riots_sht21.setup() != RIOTS_OK ) {
    /* did not work, stop here */
    while(1);
  }

  /* Initialize timers */
  next_measurement_time = riots_radio.getSeconds() + RIOTS_AIR_MEASUREMENT_INTERVAL;
  next_reporting_time = riots_radio.getSeconds() + RIOTS_HOURLY_REPORTING_INTERVAL;
}

void loop() {

  /* Dynamic reporting in every 5 secs */
  if(riots_radio.getSeconds() > next_measurement_time) {
    next_measurement_time = riots_radio.getSeconds() + RIOTS_AIR_MEASUREMENT_INTERVAL;
    /* Time has fired */

    if(radio_status != RIOTS_SLEEP) {
      /* This is not battery operated device */
      riots_bmp280.startMeasurement();
      riots_sht21.startMeasurement();

      unsigned long measurement_ready = millis() + 37;
      /* Wait until measurements are ready */
      while ( millis() < measurement_ready ) {
        /* Function update() in Riots radio library handles the software routines, such as
         * radio message handling and communication establishing. This function needs to be
         * called constantly as it will reset the watchdog and let the program run */
         riots_radio.update();
      }
    }
    else {
      /* This is battery operated device */
      riots_bmp280.startMeasurement(RIOTS_LOW_RESOLUTION);
      riots_sht21.startMeasurement(RIOTS_LOW_RESOLUTION);

      /* Function update() in Riots radio library handles the software routines, such as
       * radio message handling and communication establishing. This function needs to be
       * called constantly as it will reset the watchdog and let the program run */
      riots_radio.update(); // returns after 8sec
    }
    /* Check if there is need to report */
    dynamic_reporting();
  }

  /* Hourly reporting */
  if ( riots_radio.getSeconds() > next_reporting_time ) {
    next_reporting_time = riots_radio.getSeconds() + RIOTS_HOURLY_REPORTING_INTERVAL;
    /* report values once per hour */
    report_temperature();
    report_humidity();
    report_pressure();
  }

  /* Function update() in Riots radio library handles the software routines, such as
   * radio message handling and communication establishing. This function needs to be
   * called constantly as it will reset the watchdog and let the program run */
   radio_status = riots_radio.update();
}

void dynamic_reporting() {
  humidity = riots_sht21.readHumidity();
  temperature = riots_sht21.readTemperature();
  pressure = riots_bmp280.getPressure();

  /* Report new values to Cloud if specific value has changed enough */
  if ( abs(temperature-temperature_previous) >= RIOTS_AIR_TEMP_RESOLUTION ) {
    report_temperature();
  }

  if ( abs(humidity-humidity_previous) >= RIOTS_AIR_HUMI_RESOLUTION ) {
    report_humidity();
  }

  if ( abs(pressure-pressure_previous) >= RIOTS_AIR_PRES_RESOLUTION ) {
    report_pressure();
  }
}

void report_temperature() {
  if(temperature != RIOTS_SENSOR_FAIL) {
    /* Send the new temperatue value to cloud and possible configured RIOTS network */
    if(riots_radio.send(RIOTS_IO_0, temperature, -1) == RIOTS_OK) {
      /* Message delivered succesfully, update value */
      temperature_previous = temperature;
    }
  }
}

void report_humidity() {
  if(humidity != RIOTS_SENSOR_FAIL) {
    /* Send the new humidity value to cloud and possible configured RIOTS network */
    if(riots_radio.send(RIOTS_IO_1, humidity, -1) == RIOTS_OK) {
      /* Message delivered succesfully, update value */
      humidity_previous = humidity;
    }
  }
}

void report_pressure() {
  if(pressure != RIOTS_SENSOR_FAIL) {
    /* Send the new pressure value to cloud and possible configured RIOTS network */
    if(riots_radio.send(RIOTS_IO_2, pressure, 0) == RIOTS_OK) {
      /* Message delivered succesfully, update value */
      pressure_previous = pressure;
    }
  }
}
