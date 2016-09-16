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
#include <Riots_Helper.h>
#include <Riots_MamaRadio.h>
#include <Riots_MamaCloud.h>
#include <Riots_Mamadef.h>
#include <avr/wdt.h>

Riots_MamaRadio riots_radio;
Riots_MamaCloud riots_cloud;

uint8_t i;
uint32_t next_cloud_time;
uint8_t intCount = 0;
byte next_cloud_action;
bool previous_send_succeed;
uint8_t fail_attempt_count = 0;
uint32_t next_attempt_time = 0;
uint8_t fail_attempt;

void setup() {

  /* Disable watch dog from Mama, for the initialization */
  wdt_disable();

  /* Setup Riots radio, with led indications and without debuging. */
  riots_radio.setup(1,0);

  /* Read buffer address from radio and inform them to the cloud library */
  riots_cloud.setAddresses(riots_radio.getPlainDataAddress(),
                           riots_radio.getTXCryptBuffAddress(),
                           riots_radio.getRXCryptBuffAddress(),
                           riots_radio.getPrivateKeyAddress());

  /* Setup cloud library with the leds on */
  riots_cloud.setup(1);

  /* Clear the current actions */
  next_cloud_action = NO_ACTION_REQUIRED;
  previous_send_succeed = true;

  next_cloud_time = millis();
}

void loop() {

  if ( millis() > next_cloud_time ) {
    /* We have waited enough, its time to do something and update the timer */
    next_cloud_time = millis() + MAMA_CLOUD_PAUSE;

    /* Check and proceed with the cloud messaging */
    check_cloud_messages();

    /* Send possible earlier saved message to the cloud */
    riots_cloud.processCachedMessage();
  }
  /* Check the messages from the radio */
  check_riots_messages();
}

void check_riots_messages() {

  /* Function update() in Riots radio library handles the software routines, such as
   * radio message handling and communication establishing. This function needs to be
   * called constantly as it will reset the watchdog and let the program run */
  if (riots_radio.update() == RIOTS_OK) {
    /* There is data coming from the RIOTS network */

    if ( RIOTS_OK == riots_radio.checkRiotsMsgValidity() ) {
      /* Data is valid, forward this message to the cloud and inform status of
       * forward operation to the radio */
      riots_radio.messageDelivered(riots_cloud.forwardToCloud());
    }
    else {
      /* Received data did not pass the check, do nothing with it */
    }
  }
}

void check_cloud_messages() {
  /* Check possible data from cloud */
  byte message_status;

  if ( NO_ACTION_REQUIRED == next_cloud_action ) {
    /* There is nothing ongoing, check if there is need to something */
    riots_cloud.update(&next_cloud_action);
  }

  if ( SET_RADIO_RECEIVER == next_cloud_action ) {
    /* New data is received, and we need to first setupt the receiver address */
    riots_radio.setRadioReceiverAddress(riots_cloud.getNextReceiverAddress());
    /* Clear the action, as we have completed this request */
    next_cloud_action = NO_ACTION_REQUIRED;
  }
  else if ( FORWARD_DATA == next_cloud_action ) {
    /* We need to proceed with the received data */
    bool response_needed;
    if ( previous_send_succeed ) {
      /* Previos send was succesfully, we can continue with the new data */
      if ( riots_cloud.getNextDataBlob() > 0 ) {
        /* There is non-processed data available, process with the data and
         * pass the response_needed variable to the function */
        message_status = riots_radio.processMsg(&response_needed);
        if ( response_needed ) {
          /* We need to send a response to the cloud, this means also
           * that we have processed this message */
          if ( message_status != RIOTS_OK ) {
            /* Currently there is no need to do anything in case of our own
             *  config message processing failed, clear the status */
            previous_send_succeed = true;
          }
          /* Inform status of own config file processing to the cloud,
           * and return status of delivaration to the radio */
          if (riots_radio.messageDelivered(riots_cloud.forwardToCloud())) {
            /* As this function returns true, this was IM_ALIVE message,
             * indicate to the library that connection is now fully established */
            riots_cloud.connectionSettingsVerificated();
          }
        }
        else if ( message_status == RIOTS_OK ) {
          /* Process ok, do nothing */
        }
        else {
          /* Failed to reach the destination, try again a bit later */
          previous_send_succeed = false;
          fail_attempt_count = 1;
          next_attempt_time = millis() + MAMA_CLOUD_PAUSE;
        }
      }
      else {
        /* We did not receve any data, it means that buffer is empty and we can
         * stop the processing here */
        next_cloud_action = NO_ACTION_REQUIRED;
      }
    }
    else if ( ( millis() > next_attempt_time ) ) {
      /* It is time to retry message sending */
      if ( fail_attempt_count < MAMA_RETRY_COUNT ) {
        /* we have not yet tried enough hard */

        /* Try to process with the message againg */
        if ( RIOTS_OK == riots_radio.processMsg(&response_needed) ) {
          /* Send was a successfully */
          previous_send_succeed = true;
        }
        else {
          /* Did not manage to deliver the message, increase the counter */
          fail_attempt_count++;
          if ( fail_attempt_count < MAMA_RETRY_COUNT ) {
            /* Lets try to send the message again */
            next_attempt_time = millis() + MAMA_CLOUD_PAUSE;
          }
          else {
            /* Seems that we are not able to reach the destination, give up here
             * and inform the status to the cloud */
            /* Create the message first */
            riots_radio.createCoreNotReachedMsg();
            /* Send the message and clear the rest of messages to this receiver */
            riots_cloud.sendCoreNotReached();
            /* There is no need to do anything right now */
            next_cloud_action = NO_ACTION_REQUIRED;
            previous_send_succeed = true;
          }
        }
      }
    }
  }
}
