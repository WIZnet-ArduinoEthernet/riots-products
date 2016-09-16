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
#include <Riots_L3GD20.h>
#include <Riots_LSM303.h>
#include <Riots_BabyRadio.h>

#define RIOTS_MAGNETOMETER_CONSTANT 0.16
#define RIOTS_GYRO_REPORT_FREQUENCY 1000
#define RIOTS_ACCEL_REPORT_TRESSHOLD 200
#define RIOTS_MEASUREMENT_WAIT_PERIOD 400
#define RIOTS_HEADING_TRESSHOLD 10
#define ACCEL_CALIBRATION_TIME 8000

Riots_BabyRadio riots_radio;
Riots_L3GD20 Riots_Gyro;
Riots_LSM303 Riots_Accel;
Riots_RGBLed riots_RGBled;

int ax, ay, az, prev_ax, prev_ay, prev_az, curr_aa, prev_aa, max_a;
int curr_heading, prev_heading;
uint32_t next_hourly_reporting_time, movement_wait_timer;
bool object_is_moving;
bool interrupt_activated;
bool interrupt_fired;

/* Initialize calibration vector with full range */
Riots_LSM303::vector<int16_t> calibrated_min = {32767, 32767, 32767}, calibrated_max = { -32768, -32768, -32768};

void setup()
{
  /* Setup RBG leds */
  riots_RGBled.setup(9,6,10);

  /* Setup Riots radio, without debugging and led indications. */
  riots_radio.setup(0,0);

  /* Initialize accelemeter, with default values */
  Riots_Accel.init();
  Riots_Accel.enableDefault();

  /* Enable Gyroscope, this is used for interrupts only in this product */
  if (!Riots_Gyro.begin(Riots_Gyro.L3DS20_RANGE_250DPS)) {
    /* we were not able to inialize gyroscope, indicate with red light */
    riots_RGBled.setColor(255, 0, 0);
    delay(5000);
  }
  /* clear gyro values in the begining */
  ax = 0;
  ay = 0;
  az = 0;
  prev_ax = 0;
  prev_ay = 0;
  prev_az = 0;
  max_a = 0;
  curr_aa = 0;
  prev_aa = 0;
  next_hourly_reporting_time = 0;
  movement_wait_timer = 0;

  /* Run calibration for the procuct in the begining */
  calibrate_accel();

  /* Configure, but do not active interruptions by default */
  interrupt_activated = false;
  interrupt_fired = false;
  attachInterrupt(digitalPinToInterrupt(3), gyro_interrupt, FALLING);
}

void calibrate_accel(){

  /* use full range to get actual min & max values from calibration */
  Riots_Accel.m_min.x = -32767;
  Riots_Accel.m_min.y = -32767;
  Riots_Accel.m_min.z = -32767;
  Riots_Accel.m_max.x = 32767;
  Riots_Accel.m_max.y = 32767;
  Riots_Accel.m_max.z = 32767;

  while (millis() < ACCEL_CALIBRATION_TIME) {
    /* Read values from accelerometer */
    Riots_Accel.read();
    calibrated_min.x = min(calibrated_min.x, Riots_Accel.m.x);
    calibrated_min.y = min(calibrated_min.y, Riots_Accel.m.y);
    calibrated_min.z = min(calibrated_min.z, Riots_Accel.m.z);

    calibrated_max.x = max(calibrated_max.x, Riots_Accel.m.x);
    calibrated_max.y = max(calibrated_max.y, Riots_Accel.m.y);
    calibrated_max.z = max(calibrated_max.z, Riots_Accel.m.z);

    /* Keep radio alive */
    riots_radio.update();

    /* Indicate that we are still in the midle of the calibration process */
    riots_RGBled.setColor(255, 255, 255);
  }
  /* switch leds to the off */
  riots_RGBled.setColor(0);
  delay(100);

  if ( calibrated_max.x > 500 && calibrated_max.x < 5000 &&
       calibrated_min.x < -500 && calibrated_min.x > -5000 ) {
    /* we have received "realistic" calibration values, use them */
    Riots_Accel.m_max.x = calibrated_max.x;
    Riots_Accel.m_max.y = calibrated_max.y;
    Riots_Accel.m_max.z = calibrated_max.z;

    Riots_Accel.m_min.x = calibrated_min.x;
    Riots_Accel.m_min.y = calibrated_min.y;
    Riots_Accel.m_min.z = calibrated_min.z;
    /* indicate to the user about this calibration */
    riots_RGBled.setColor(0, 255, 0);
    delay(100);
    riots_RGBled.setColor(0);
    delay(100);
    riots_RGBled.setColor(0, 255, 0);
    delay(100);
    riots_RGBled.setColor(0);
    delay(100);
  }
  else {
    /* Values were not realistic, use values measured in the Riots laboratory */
    Riots_Accel.m_max.x = 2589;
    Riots_Accel.m_max.y = 1496;
    Riots_Accel.m_min.x = -2515;
    Riots_Accel.m_min.y = -3312;
    Riots_Accel.m_min.z = -2145;
    Riots_Accel.m_max.z = 2519;
  }

  /* Read new initialization values */
  Riots_Accel.read();

  /* save accelerometer values, Shift 4 bytes to right to get real values */
  ax = Riots_Accel.a.x >> 4;
  ay = Riots_Accel.a.y >> 4;
  az = Riots_Accel.a.z >> 4;

  /* Save current accleration to the vectors */
  prev_ax = ax;
  prev_ay = ay;
  prev_az = az;

  /* calculate sum of abs */
  curr_aa = abs(ax) + abs(ay) + abs(az);
  prev_aa = curr_aa;
}

void loop()
{
  /* Check updates from the network */
  check_radio();

  /* Read accerometer */
  read_accel();

  /* Read heading */
  read_heading();

  /* Hourly reporting */
  if ( riots_radio.getSeconds() > next_hourly_reporting_time ) {
    report_values();
    next_hourly_reporting_time = riots_radio.getSeconds() + RIOTS_HOURLY_REPORTING_INTERVAL;
  }
}

void check_radio() {

  if ( interrupt_fired) {
    /* Interrupt has fired during the radio sleep, report values*/
    report_values();
    /* indicate with the led */
    riots_RGBled.setColor(0, 0, 255);
    /* clear the interrupt status */
    interrupt_fired = false;
  }

  /* Function update() in Riots radio library handles the software routines, such as
   * radio message handling and communication establishing. This function needs to be
   * called constantly as it will reset the watchdog and let the program run */
  if ( riots_radio.update() == RIOTS_SLEEP ) {
    /* Battery mode is active */
    if ( !interrupt_activated ) {
      /* this is the first time when the mode has been activate, activate
       * the interruptions for the gyro */
      interrupt_activated = true;
      Riots_Gyro.enableInterrupts();
    }
  }
}

void read_accel() {

  /* Read new values from the accelerometer */
  Riots_Accel.read();

  /* save accelerometer values, Shift 4 bytes to get real values */
  ax = Riots_Accel.a.x >> 4;
  ay = Riots_Accel.a.y >> 4;
  az = Riots_Accel.a.z >> 4;

  /* calculate sum of abs */
  curr_aa = abs(ax) + abs(ay) + abs(az);

  if ( differs(prev_ax, ax, RIOTS_ACCEL_REPORT_TRESSHOLD) ||
       differs(prev_ay, ay, RIOTS_ACCEL_REPORT_TRESSHOLD) ||
       differs(prev_az, az, RIOTS_ACCEL_REPORT_TRESSHOLD) ) {
    /* There is enough difference in one of the axel */
    if ( !object_is_moving ) {
      /* Enable object moving status */
      object_is_moving = true;
      /* Send the zero acceleration to cloud and possible configured RIOTS network */
      riots_radio.send(1, 0, -3);

      /* Send the sum of current acceleration abs to cloud and possible configured RIOTS network,
       * Roughtly 1G has been removed from the sum */
      riots_radio.send(1, curr_aa - 1000, -3);
    }
    /* Increase the timer */
    movement_wait_timer = millis() + RIOTS_MEASUREMENT_WAIT_PERIOD;
  }
  /* update previous value */
  prev_ax = ax;
  prev_ay = ay;
  prev_az = az;


  if ( object_is_moving ) {
    /* find the highest ABS sum during the movement */
    if ( curr_aa > max_a ) {
      max_a = curr_aa;
    }
  }

  if ( object_is_moving &&
       millis() > movement_wait_timer )
  {
    /* Movement has been stopped */
    object_is_moving = false;

    /* Send the max sum of acceleration abs to cloud and possible configured RIOTS network,
     * Roughtly 1G has been removed from the sum */
    riots_radio.send(1, max_a - 1000, -3);

    /* Send the zero acceleration to cloud and possible configured RIOTS network */
    riots_radio.send(1, 0, 0);
    max_a = 0;
  }
}

void read_heading() {
  /* Read current heading */
  curr_heading = Riots_Accel.heading();
  if ( differs(curr_heading, prev_heading, RIOTS_HEADING_TRESSHOLD) ) {
    /* Send the current heading to cloud and possible configured RIOTS network as there is enough difference */
    riots_radio.send(0, curr_heading, 0);
    prev_heading = curr_heading;
  }
}

void gyro_interrupt() {
  /* Gyro inttupt has fired */
  interrupt_fired = true;
}

void report_values() {

  /* report all current values to the cloud */
  Riots_Accel.read();

  /* save accelerometer values, Shift 4 bytes to get real values */
  ax = Riots_Accel.a.x >> 4;
  ay = Riots_Accel.a.y >> 4;
  az = Riots_Accel.a.z >> 4;

  /* calculate sum of abs */
  curr_aa = abs(ax) + abs(ay) + abs(az);

  /* Send the sum of current acceleration abs to cloud and possible configured RIOTS network,
   * Roughtly 1G has been removed from the sum */
  riots_radio.send(1, curr_aa - 1000, -3);
  prev_aa = curr_aa;

  /* Read current heading */
  curr_heading = Riots_Accel.heading();

  /* Send the current heading to cloud and possible configured RIOTS network as there is enough difference */
  riots_radio.send(0, curr_heading, 0);
  prev_heading = curr_heading;
}

bool differs(int a, int b, int tresshold) {
  /* check if there is enough difference between the values */
  if ( a > b ) {
    if ( a - b > tresshold ) {
      return true;
    }
  }
  if ( b > a ) {
    if ( b - a > tresshold ) {
      return true;
    }
  }
  return false;
}
