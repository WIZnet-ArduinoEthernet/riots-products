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

#include "Riots_TMD3782x.h"
#include <Riots_BabyRadio.h>

Riots_TMD3782x riots_TMD3782x;
Riots_BabyRadio riots_radio;

double lux = 0;
double colorTemp = 0;
double luxPrev = 0;
double colorTempPrev = 0;
byte measurement_ongoing = 0;
byte radio_status = RIOTS_OK;
byte first_time_reported = 0;

unsigned long batteryTime=0;
unsigned long time_hours=0;
unsigned long time_short=0;

void setup() {

  /* Setup TMD3728 sensor */
  riots_TMD3782x.setup();

  /* Setup Riots radio, with led indications and without debuging. */
  riots_radio.setup(1,0);
}

void loop() {

  /* Hourly reporting */
  if ( (riots_radio.getSeconds() - time_hours) > 3600 && radio_status != RIOTS_SLEEP ) {
    /* get new values as timer has fired */
    riots_TMD3782x.readRgbcData();
    lux = riots_TMD3782x.getLux();

    /* Send brigthness values to the cloud and possible configured RIOTS network */
    riots_radio.send(0, lux*100, -2);

    luxPrev = lux;
    colorTemp = riots_TMD3782x.getColorTemperature();

    /* Send color temperature values to the cloud and possible configured RIOTS network */
    riots_radio.send(1, colorTemp*100, -2);
    colorTempPrev = colorTemp;
    time_hours = riots_radio.getSeconds();
  }

  /* Dynamic reporting every 5 sec */
  if ( (riots_radio.getSeconds() - time_short) > 5 && radio_status == RIOTS_OK ) {
    /* Timer has fired */

    riots_TMD3782x.readRgbcData();
    lux = riots_TMD3782x.getLux();
    colorTemp = riots_TMD3782x.getColorTemperature();

    /* Compare to previous value and send if changed */
    if ( abs(lux-luxPrev) > 5 ) {
      luxPrev = lux;
      /* Send brigthness values to the cloud and possible configured RIOTS network */
      riots_radio.send(0, lux*100, -2);
    }

    if ( abs(colorTemp-colorTempPrev) > 10 ) {
      colorTempPrev = colorTemp;
      /* Send color temperature values to the cloud and possible configured RIOTS network */
      riots_radio.send(1, colorTemp*100, -2);
    }
    /* Report all values once in the begining as connection to the Cloud is now available */
    if ( riots_radio.getCloudStatus() == RIOTS_OK && first_time_reported == 0 ) {
      riots_radio.send(0, lux*100, -2);
      riots_radio.send(1, colorTemp*100, -2);
      first_time_reported = 1;
    }

    time_short = riots_radio.getSeconds();
  }

  /* Battery operation has been enabled */
  if ( radio_status == RIOTS_SLEEP ){

    if ( measurement_ongoing == 1 ) {
      /* there is measurement ongoing */
      riots_TMD3782x.readRgbcData();

      lux = riots_TMD3782x.getLux();
      riots_radio.send(0, lux*100, -2);

      colorTemp = riots_TMD3782x.getColorTemperature();
      riots_radio.send(1, colorTemp*100, -2);

      riots_TMD3782x.stopMeasurement();
      measurement_ongoing = 0;
    }
    else if ( riots_radio.getSeconds()-batteryTime > 3600) {
      /* Measurement has stopped and the timer has fired */
      riots_TMD3782x.startMeasurement();
      batteryTime = riots_radio.getSeconds();
      measurement_ongoing = 1;
    }
  }

  /* Function update() in Riots radio library handles the software routines, such as
   * radio message handling and communication establishing. This function needs to be
   * called constantly as it will reset the watchdog and let the program run */
  radio_status = riots_radio.update();
}
