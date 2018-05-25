/******************************************************************************
* Copyright (C) 2018 By Mike Sharps
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the copyright holder nor the names of the
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 *

tempmon.c
9/5/2018
Raspberry Pi I2C BME280 temp, humidity, barometric pressure 


Raspberry Pi 
I2C interface,for BME 280
UART Interface for PTower air qulity monitor

*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>   	// Standard input/output definitions
#include <string.h>  	// String function definitions
#include <unistd.h>  	// UNIX standard function definitions
#include <fcntl.h>   	// File control definitions
#include <errno.h>   	// Error number definitions
#include <termios.h> 	// POSIX terminal control definitions
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include "misc.h"
#include "bme280.h"
#include "tempmon.h"
#include "plantower.h"
#include "upload.h"

/* purely to allow testing of functions within this code */
unsigned char	testMessage[] = {0x42,0x4d,0x00,28,0,1,0,2,0,3,0,4,0,5,0,6,0,7,0,8,0,9,0,10,0,11,0,12,0,0};

unsigned char testCommand[] = {0x42,0x4D,0x41,0x42,0x43};
/* purely to allow testing of functions within this code */

/* Plantower messages
*	Active mode sends back readings continuously
*	Passive mode responds only when a readPAssive message is sent
*/

unsigned char readPassiveMode[] = {0x42,0x4D,0xE2,0x00,0x00};
unsigned char setPassiveMode[]  = {0x42,0x4D,0xE1,0x00,0x00};
unsigned char setActiveMode[]   = {0x42,0x4D,0xE1,0x00,0x01};
unsigned char setSleepMode[]    = {0x42,0x4D,0xE4,0x00,0x00};
unsigned char setWakeupMode[]   = {0x42,0x4D,0xE4,0x00,0x01};


// this will pick up from config for different board configs.
char *getSerialDeviceName(void)
{
	// Put this here in case the serial port name changes sometime
	
	return "/dev/ttyS0";
}


unsigned short doPTChecksum(unsigned char * cmdBuffer, int len){
	
	unsigned short chk = 0;

	for(int n=0;n<len;n++){
		chk += cmdBuffer[n];
	}
	return chk;
}

// add a checksum to a command message
void addPTCheckSum(unsigned char * cmdBuffer, int len){
	
	unsigned short chk = 0;
	unsigned char loByte; // lower byte
	unsigned char hiByte; // upper byte

	chk = doPTChecksum(cmdBuffer,len);
	
	
	// Split short into two char

	loByte = chk & 0xFF;
	hiByte = chk >> 8;	

	// store checksum big endian
	cmdBuffer[len+1] = loByte;
	cmdBuffer[len] = hiByte; 

}



// verify a received PT message
int verifyPTMessage(unsigned char * cmdBuffer, int maxLen){
	
	unsigned short 	calcCheckSum = 0;
	unsigned short	messageCheckSum = 0;
	

	if(cmdBuffer[MSG_HEADER_1] == 0x42 && cmdBuffer[MSG_HEADER_2] == 0x4d){
		int msgLen = bytesToInt(cmdBuffer[MSG_LEN_LO],cmdBuffer[MSG_LEN_HI]);
		
		// the message checksum adds all bytes in the message up to the last data byte
		// msgLen = data bytes + checksum bytes (13*2 + 2)
		// adjust the message length so we can calculate the checksum 
		
		msgLen += 2; // add 4 for header + length bytes and subtract 2 for checksum bytes
		if(msgLen <= maxLen){

			calcCheckSum = doPTChecksum(cmdBuffer,msgLen);
			// msgLen = 2 less than total message length so points at the checksum
			messageCheckSum = bytesToInt(cmdBuffer[msgLen+1],cmdBuffer[msgLen]);
			if(messageCheckSum == calcCheckSum){

			return ERR_OK;
			}
			else{
				printf("Message Length %d, Calculated Checksum = %x, Message Checksum = %x",msgLen,calcCheckSum,messageCheckSum);
				return PT_CHECKSUM_ERROR;
			}
		}
		else{
			printf("Message Length %d",msgLen);
			return PT_LEN_ERROR;
		}
	}
	else{
		return PT_HEADER_ERROR;
	}
}


int openSerialPort(char * serialDevice, int baud){
	int fd = serialOpen(serialDevice,baud);
	if(fd == -1){
		printf("Erro opening serial port\n");
	}
	else{
		printf("Serial Port open \n");
		serialFlush(fd);
	}
	return fd;

}

void closeSerialPort(int fd){
	serialClose(fd);
}

// send a planTower command string
void sendPTCommand(int fd, unsigned char * cmdBuffer, int bufferLen){
	
	for (int n=0; n<bufferLen; n++){
		serialPutchar(fd,cmdBuffer[n]);
		//printf("%x,",cmdBuffer[n]);
	}
	//printf("\n");

}

// problem with this is if it gets half a message it has to exit so only pulls one byte off the serial port per call
int readPTDataSafe(int fd,unsigned char *buffer, int bufferLen){
		
	int serialData = 0;
	int count = 0;
	unsigned char serialByte;

	int dataReady = 0;

	dataReady = serialDataAvail(fd);
	if(dataReady >= 0x20){
		serialData = serialGetchar(fd);
		if (serialData == 0x42){
			serialByte = serialData & 0xff;
			buffer[count] = serialByte; 
			count++;
			serialData = serialGetchar(fd);
			if (serialData == 0x4D){
				serialByte = serialData & 0xff;
				buffer[count] = serialByte; 
				count++;
				// get the rest of the message from the buffer				
				while(count < 0x20){
					serialData = serialGetchar(fd);
					serialByte = serialData & 0xff;
					buffer[count] = serialByte; 
					count++;
				}
							
			}


		}

	}
	return count;
	//printf("%x, ", serialByte);		

}

// just read the data but check for header before storing in the buffer
int readPTDataSafe2(int fd,unsigned char *buffer, int bufferLen){
	
	int count = 0;
	int serialData = 0;
	unsigned char serialByte;
	// clear buffer
	memset(buffer,0,bufferLen);
	
	while(count < 0x20){

		// not found a header yet
		if (count==0){

			// pull a byte of the serial port
			serialData = serialGetchar(fd);
			// exit if error
			if(serialData == -1){
				return -1;
			}
			// check for a header bytes
			if (serialData == 0x42){
				// store it in the buffer
				serialByte = serialData & 0xff;
				buffer[count] = serialByte; 
				count++;
				// check the next byte is also a header byte	
			
				serialData = serialGetchar(fd);
				if(serialData == -1){
					return -1;
				}
				
				if (serialData == 0x4D){
					serialByte = serialData & 0xff;
					buffer[count] = serialByte; 
					count++;
				}
			}
			// if not found  a header reset the count and try again 
			if(count !=2){
				count = 0;
			}
		}

		// get the rest of the message from the buffer				
		else{
			serialData = serialGetchar(fd);
			// exit if error
			if(serialData == -1){
				return -1;
			}
			serialByte = serialData & 0xff;
			buffer[count] = serialByte; 
			count++;
		}
			
	} // while
	return count;
}


int readPTDataBlocking(int fd){
	
	char *respBuffer[0x30];
	int byteCount = 0;
	memset(respBuffer,0,sizeof(respBuffer));
	printf("\n");
	while(1){
		int serialData = serialGetchar(fd);
		if(serialData != -1){
			unsigned char serialByte = serialData & 0xff;
			printf("%x, ", serialByte);
			if(++byteCount>=32){
				printf("\n");
				byteCount=0;
			}

		}

	}


}

// in case we need to set special serial parameters

int setSpecial(int fd, unsigned char bits, unsigned char parity, unsigned char stop_bits){
  int err = 0;
  /*
  tcgetattr (fd, &options) ;   // Read current options
  options.c_cflag &= ~CSIZE ;  // Mask out size
  options.c_cflag |= CS7 ;     // Or in 7-bits
  options.c_cflag |= PARENB ;  // Enable Parity - even by default
  tcsetattr (fd, &options) ;   // Set new options
  */

  return err;

}


int main(void){

	int fd;
	int testMode = 0;
	int cmd = AIRQ_INIT;
	int bmeEnabled = BME_INIT;
	int status =0;
	// poll timers
	int airqPollTimer = 0;
	int	airqTimeout = 0;
	int bmePollTimer = 0;
	int AirQReady = 0;
	// bme280 device structure
	struct bme280_dev dev;
	// bme280 result data structure
	struct bme280_data comp_data;
	// copy all data to this struct
	struct PTMeasuredData measuredData;

	//char *serialDevice ;
	unsigned char cmdBuffer[MAX_BUFFER_LEN];
	unsigned char dataBuffer[MAX_BUFFER_LEN];

	
	memset(cmdBuffer,0,sizeof(cmdBuffer));
	memset(dataBuffer,0,sizeof(dataBuffer));

	fd = openSerialPort(getSerialDeviceName(),9600);
	if(fd == -1){
		printf("error exit\n");
		exit(0);
	}

	while(1){
		
		if(bmeEnabled==BME_INIT){
			status = initTempMon(&dev);
			if(status){
				printf("BME280 init Failure\n");
			}
			else{
				printf("BME280 init OK\n");
				bmeEnabled = BME_READ;
			}
			bmePollTimer = 0;
		}

		if(bmeEnabled == BME_READ){
			if(++bmePollTimer > POLL_DELAY_5){
				bmePollTimer = 0;
				printf("\n");
				printf("Reading BME data\n");
				// take a reading
				status = stream_sensor_data_normal_mode(&dev,&comp_data);
				if (status == 0){
					measuredData.temperature = comp_data.temperature;
					measuredData.pressure = comp_data.pressure;
					measuredData.humidity = comp_data.humidity;

					// copy results into byte buffer MSB first (Big endian)

					longToBytes(measuredData.temperature,&measuredData.rawBMEData[0],ENDIAN_MSB);
					longToBytes(measuredData.pressure,&measuredData.rawBMEData[4],ENDIAN_MSB);
					longToBytes(measuredData.humidity,&measuredData.rawBMEData[8],ENDIAN_MSB);

					//printf("%02x, ",measuredData.temperature);
					//printf("%02x, ",measuredData.pressure);
					//printf("%02x\n",measuredData.humidity);
					//displayHexMessage(measuredData.rawBMEData,12);

				}
			}
		}

		// set the Air Q monitor mode
		// Activ mode causes the device to automatically send data. The ate is dependant on concentration levels.
		//
		
		// Initialise mode for Air Q
		if(cmd == AIRQ_INIT){

			memcpy(cmdBuffer,setPassiveMode,sizeof(setPassiveMode));
		
			addPTCheckSum(cmdBuffer,sizeof(setPassiveMode));
			displayHexMessage(cmdBuffer,CMD_BUFF_LEN);
			sendPTCommand(fd,cmdBuffer,CMD_BUFF_LEN);
			cmd=AIRQ_POLL;

/*
			memcpy(cmdBuffer,setWakeupMode,sizeof(setWakeupMode));
		
			addPTCheckSum(cmdBuffer,sizeof(setWakeupMode));
			displayPTMessage(cmdBuffer,CMD_BUFF_LEN);
			sendPTCommand(fd,cmdBuffer,CMD_BUFF_LEN);

		
			memcpy(cmdBuffer,setActiveMode,sizeof(setActiveMode));
		
			addPTCheckSum(cmdBuffer,sizeof(setActiveMode));
			displayPTMessage(cmdBuffer,CMD_BUFF_LEN);
			sendPTCommand(fd,cmdBuffer,CMD_BUFF_LEN);
*/

		}

		if(cmd == AIRQ_POLL){

			// wait for delay (secs)
			if(airqPollTimer++ == POLL_DELAY_5){
				// take a reading
				// load the command
				memcpy(cmdBuffer,readPassiveMode,sizeof(readPassiveMode));			
				addPTCheckSum(cmdBuffer,sizeof(readPassiveMode));

				//displayPTMessage(cmdBuffer,CMD_BUFF_LEN);
				printf("Polling AIR Q Device\n");
				sendPTCommand(fd,cmdBuffer,CMD_BUFF_LEN);
				cmd = AIRQ_WAITING;
				// reset poll timer
				airqPollTimer=0;
				// reset message received timout
				airqTimeout = 0;
			}
		}
			
		// read data and print to screen
		if(cmd == AIRQ_WAITING){
		

			int dataReady = 0;


			// only read if there is data waiting so we don't get stuck here
			dataReady = serialDataAvail(fd);
			// printf("data bytes available = %d\n", dataReady);
			// we have data ready for reading
			if(dataReady > 0x00){
			
				memset(dataBuffer,0,sizeof(dataBuffer));
				// read the data
				int dataCount = readPTDataSafe2(fd,dataBuffer,0x20);
				
				if (dataCount < 0x20){
					printf("AIR Q - Response incomplete %d \n",dataCount);
				}
				
				else{
					int status = verifyPTMessage(dataBuffer,0x20);
					if(status != ERR_OK){
						printf("Message Invalid \n");
					}
					else{
						// raw message for debug
						displayHexMessage(dataBuffer,dataCount);
						
						// data OK so copy integer values of the data for display 
						unsigned char *dataptr = &dataBuffer[4];

						for(int n=0, m=0; m<12; n+=2,m++){
							int val = (dataptr[n]*256) + dataptr[n+1];
							measuredData.PTData[m] = val;
						}
						// copy the raw message data for upload
						memcpy(measuredData.rawPTData,&dataBuffer[4],sizeof(measuredData.rawPTData));
						//displayByteArray(&dataBuffer[4],24);
						//displayHexMessage(&dataBuffer[4],24);
						//displayHexMessage(measuredData.rawPTData,24);
						// display the data
						printf("Air Q data: ");
						displayIntArray(measuredData.PTData,12);
						AirQReady = 1;						
						
					}
				}
				// request next message
				cmd = AIRQ_POLL;
			}
			
			else{
				// no response yet
				if(++airqTimeout > 3) {
					cmd = AIRQ_POLL;
				}
			}
		
		}

		if(AirQReady){

			sendDataToWeb(&measuredData);

		}

		if(testMode){
			// test verify function
			memcpy(cmdBuffer,testMessage,sizeof(testMessage));
		
			addPTCheckSum(cmdBuffer,sizeof(testMessage));
			displayHexMessage(cmdBuffer,sizeof(testMessage)+2);
			int err = verifyPTMessage(cmdBuffer,0x30);
			printf("Verify Message %d", err);

			// send a test string
		}

		// loop every second
		sleep(1);
		
		
		// wakeup
		
	} // while(1)
	
	closeSerialPort(fd);
	
	return(0);
}




