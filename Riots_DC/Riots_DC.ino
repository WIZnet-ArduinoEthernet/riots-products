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
#include "Riots_DC_Control.h"
#include <Riots_BabyRadio.h>
#include <Riots_RGBLed.h>

Riots_DC_Control riots_dc_control;
Riots_BabyRadio riots_radio;
Riots_RGBLed riots_RGBLed;
uint32_t data;
uint8_t index;

void setup() {

  /* Setup RGB led */
  riots_RGBLed.setup();

  /* Setup Riots radio, with led indications and without debuging. */
  riots_radio.setup(1,0);

  /* Setup DC controller, by default the DC state is off */
  riots_dc_control.setup();
}

void loop() {
  /* Function update() in Riots radio library handles the software routines, such as
   * radio message handling and communication establishing. This function needs to be
   * called constantly as it will reset the watchdog and let the program run */
  if ( riots_radio.update() == RIOTS_DATA_AVAILABLE ) {
    /* We have received data from the riots network,
     * it means that we need follow ordered states */
    data = riots_radio.getData();
    index = riots_radio.getIndex();

    if ( index == 0 ) {
      /* On/off controlling */
      if ( data == 0 ) {
        /* Switch DC to the off */
        riots_dc_control.setState(0);
        /* Send DC state to the cloud and possible configured RIOTS network */
        riots_radio.send(1, (int32_t)0, 0);
      }
      else {
        /* Switch DC to the max */
        riots_dc_control.setState(255);
        /* Send DC state to the cloud and possible configured RIOTS network */
        riots_radio.send(1, (int32_t)1, 0);
      }
    }
  }
}
