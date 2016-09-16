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

Riots_BabyRadio riots_radio;
Riots_RGBLed riots_RGBLed;
unsigned long time;

void setup()
{
  /* Setup Riots radio interface (leds and debug enabled) */
  riots_radio.setup(1, 1);

  /* Store current time */
  time = millis();
}

void loop()
{

  if ( millis() - time > 10000 ) {
    Serial.println("Riots USB Alive");
    time = millis();
  }

  /* Function update() in Riots radio library handles the software routines, such as
   * radio message handling and communication establishing. This function needs to be
   * called constantly as it will reset the watchdog and let the program run */
  riots_radio.update();
}
