/******************************************************************************
tempmon.c
Raspberry Pi I2C BME280 temp, humidity, barometric pressure 

By Mike Sharps
9/5/2018

Raspberry Pi I2C interface,for BME 280

Resources:

This example makes use of the Wiring Pi library, which streamlines the interface
the the I/O pins on the Raspberry Pi, providing an API that is similar to the
Arduino.  You can learn about installing Wiring Pi here:
http://wiringpi.com/download-and-install/

The I2C API is documented here:
https://projects.drogon.net/raspberry-pi/wiringpi/i2c-library/

The interface uses the Bosch BME280 API 
https://github.com/BoschSensortec/BME280_driver/

Hardware connections:

This file interfaces with the BME280 breakout board:

The board was connected as follows:
(Raspberry Pi)(MCP4725)
GND  -> GND
3.3V -> Vcc
SCL  -> SCL
SDA  -> SDA


Then to run it, first the I2C kernel module needs to be loaded.  This can be 
done using the GPIO utility.
> gpio load i2c 400

Development environment specifics:
Tested on Raspberry Pi V3 hardware, running Raspbian Stretch.
Building with GCC 6.3.0


Distributed as-is; no warranty is given.
******************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>      // Standard input/output definitions
#include <string.h>     // String function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "bme280.h"
#include "bme280I2C.h"
#include "bme280_selftest.h"



void print_sensor_data(struct bme280_data *comp_data)
{
    printf("Temperature, Pressure, Humidity\r\n");

#ifdef BME280_FLOAT_ENABLE
        printf("%0.2f, %0.2f, %0.2f\r\n",comp_data->temperature, comp_data->pressure, comp_data->humidity);
#else
        printf("%d, %d, %d\r\n",comp_data->temperature, comp_data->pressure, comp_data->humidity);
#endif

}

int8_t init_sensor_data_normal_mode(struct bme280_dev *dev)
{
  int8_t rslt;
  uint8_t settings_sel;
  
  /* Recommended mode of operation: Indoor navigation */
  dev->settings.osr_h = BME280_OVERSAMPLING_1X;
  dev->settings.osr_p = BME280_OVERSAMPLING_16X;
  dev->settings.osr_t = BME280_OVERSAMPLING_2X;
  dev->settings.filter = BME280_FILTER_COEFF_16;
  dev->settings.standby_time = BME280_STANDBY_TIME_62_5_MS;

  settings_sel = BME280_OSR_PRESS_SEL;
  settings_sel |= BME280_OSR_TEMP_SEL;
  settings_sel |= BME280_OSR_HUM_SEL;
  settings_sel |= BME280_STANDBY_SEL;
  settings_sel |= BME280_FILTER_SEL;
  rslt = bme280_set_sensor_settings(settings_sel, dev);
  rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, dev);

  
  return rslt;
}

int8_t stream_sensor_data_normal_mode(struct bme280_dev *dev, struct bme280_data *comp_data)
{
    
    int rslt;
    /* Delay while the sensor completes a measurement */
    dev->delay_ms(70);
    rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);
    print_sensor_data(comp_data);
  
  return rslt;
}


int8_t init_sensor_data_forced_mode(struct bme280_dev *dev)
{
    int8_t rslt;
    uint8_t settings_sel;
    struct bme280_data;

    /* Recommended mode of operation: Indoor navigation */
    dev->settings.osr_h = BME280_OVERSAMPLING_1X;
    dev->settings.osr_p = BME280_OVERSAMPLING_16X;
    dev->settings.osr_t = BME280_OVERSAMPLING_2X;
    dev->settings.filter = BME280_FILTER_COEFF_16;

    settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    rslt = bme280_set_sensor_settings(settings_sel, dev);
    // rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);

    return rslt;

}

int8_t stream_sensor_data_forced_mode(struct bme280_dev *dev,struct bme280_data *comp_data)
{
    int8_t rslt;

    // have to set the mode for each FORCED reading
    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
    /* Wait for the measurement to complete and print data @25Hz */
    dev->delay_ms(40);
    rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);
    print_sensor_data(comp_data);
    return rslt;
}


// called from main or run this when set up as a thread

int initTempMon(struct bme280_dev *dev)
{
   int fd;  // file descriptor used with wiringPi I2C library as device identifier

  // Initialize the interface by giving it an external device ID.
  // The BME280 defaults to address 0x77.   
   
  printf("init I2C\n");

  // BME280 API device def
  
  int8_t rslt = BME280_OK;


  fd = wiringPiI2CSetup(0x77);  // default I2C address = 0x77

  if(fd == -1){
     printf("I2C failed to initialise\n");
  }

    dev->dev_id = fd;
    dev->intf = BME280_I2C_INTF;
    dev->read = user_i2c_read;           // wiringPiI2CReadReg8;
    dev->write = user_i2c_write;         // wiringPiI2CWriteReg8;
    dev->delay_ms = user_delay_ms;

    rslt = bme280_init(dev);
    if(rslt == BME280_OK){
      printf("BME initialised OK\n");

    }

  // get ready to roll
  rslt = init_sensor_data_normal_mode(dev);

  return rslt; 
}










