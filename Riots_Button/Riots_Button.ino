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
#include <Riots_RGBLed.h>
#include <Riots_Button.h>

bool button_0, button_1, button_2;
byte current_buttons;
uint32_t data;
uint8_t index;

Riots_BabyRadio riots_radio;
Riots_RGBLed riots_RGBLed;
Riots_Button riots_button;

void setup()
{
  /* Setup RGB led */
  riots_RGBLed.setup();

  /* indicate with the buttons that Base is booting */
  riots_RGBLed.setColor(0, 220, 0);
  delay(200);
  riots_RGBLed.setColor(0, 0, 0);
  delay(200);
  riots_RGBLed.setColor(0, 220, 0);
  delay(200);
  riots_RGBLed.setColor(0, 0, 0);
  delay(200);
  riots_RGBLed.setColor(0, 220, 0);
  delay(200);
  riots_RGBLed.setColor(0, 0, 0);

  /* Setup Riots radio, without debugging and led indications. */
  riots_radio.setup(0,0);

  /* Clear status of buttons */
  current_buttons = 0;
  button_0 = false;
  button_1 = false;
  button_2 = false;

  /* Setup the buttons */
  riots_button.setup();
}
void loop()
{
  /* Function update() in Riots radio library handles the software routines, such as
   * radio message handling and communication establishing. This function needs to be
   * called constantly as it will reset the watchdog and let the program run */
  if ( riots_radio.update() == RIOTS_DATA_AVAILABLE ) {
    /* We have received data from the riots network,
     * it means that our buttons needs to follow other (button) states */

    /* Get index and data */
    index = riots_radio.getIndex();
    data = riots_radio.getData();
    if ( index == 0 ) {
      if ( data > 0 ) {
        button_0 = true;
      }
      else {
        button_0 = false;
      }
    }
    else if ( index == 1 ) {
      if ( data > 0 ) {
        button_1 = true;
      }
      else {
        button_1 = false;
      }
    }
    else if ( index == 2 ) {
      if ( data > 0 ) {
        button_2 = true;
      }
      else {
        button_2 = false;
      }
    }
    else {
      /* Do nothing, as the index was not valid */
    }
    /* Activate leds to indicate with buttons (working like switches) are active */
    riots_RGBLed.setColor(button_0*255, button_1*255, button_2*255);
  }

  /* Check status of the own buttons */
  check_buttons();
}

void check_buttons() {

  current_buttons = riots_button.read();
  if (current_buttons != 0 ) {
    /* Status of buttons has changed */

    if ( bitRead(current_buttons, 0) ) {
      /* Button 0 status has changed */
      button_0 = !button_0;

      /* Indicate with the change of button with the RGB led */
      riots_RGBLed.setColor(button_0*255, button_1*255, button_2*255);

      /* Send the new value of button to cloud and possible configured RIOTS network */
      riots_radio.send(0, (int32_t)button_0, 0);
    }
    if ( bitRead(current_buttons, 1) ) {
      /* Button 1 status has changed */
      button_1 = !button_1;

      /* Indicate with the change of button with the RGB led */
      riots_RGBLed.setColor(button_0*255, button_1*255, button_2*255);

      // inform new value as button 0 status is changed
      riots_radio.send(1, (int32_t)button_1, 0);
    }
    if ( bitRead(current_buttons, 2) ) {
      /* Button 2 status has changed */
      button_2 = !button_2;

      /* Indicate with the change of button with the RGB led */
      riots_RGBLed.setColor(button_0*255, button_1*255, button_2*255);

      /* Send the new value of button to cloud and possible configured RIOTS network */
      riots_radio.send(2, (int32_t)button_2, 0);
    }
  }
}
