/******************************************************************************
* Copyright (C) 2018 By Mike Sharps
*   upload.c
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
#include "plantower.h"


void displayHexMessage(unsigned char * buffer, int len){
		
		for(int i = 0; i<len; i++){
    		printf("%02x ", buffer[i]);    	
		}
		printf("\n");

}

void displayByteArray(uint8_t *buffer,int len){
		
		for(int i = 0; i<len; i++){
    		printf("%d ", buffer[i]);    	
		}
		printf("\n");

}

void displayIntArray(int *buffer,int len){
		
		for(int i = 0; i<len; i++){
    		printf("%d ", buffer[i]);    	
		}
		printf("\n");

}



void bytesToHexString(uint8_t *byteBuffer,char *strBuffer,int byteLen){

        char *str = strBuffer;
        for(int i=0;i<byteLen;i++){
            sprintf(str, "%2x,", byteBuffer[i]);
            str[2] = ',';
            str+=3;     
        }
        str[0] = 0;
        printf("%s\n",strBuffer);

}

// generic conversion of two bytes to an int
int bytesToInt(unsigned char loByte, unsigned char hiByte){

	int result = (hiByte << 8) + loByte;
	return result;
}


// convert a big endian (MSB FIrst) array of ibytes to an integer array
void byteArrayToInts(uint8_t *bytePtr,int *intPtr,int len ){
	for(int n=0;n<len;n++){
		intPtr[n] = bytesToInt(bytePtr[1],bytePtr[0]); 
		intPtr++;
	}
}

// this does the same as above
void PTBytesToInts(char *dataptr,int *intptr){
		for(int n=0, m=0; m<12; n+=2,m++){
			int val = (dataptr[n]*256) + dataptr[n+1];
			intptr[m] = val;
    }
}

void intsToStr(int *intArray,char *strBuffer,int len){
    
    char str[512];
    //printf("%d",len);
    for(int n=0;n<len;n++){

        sprintf(str,"%d",intArray[n]);
        strcat(strBuffer,str);
        if(n<len-1){
            strcat(strBuffer,",");
        }
        //printf("%s\n",strBuffer);
    }

}

void uint32ToStr(uint32_t *int32Array,char *strBuffer,int len){
    
    char str[100];
    for(int n=0;n<len;n++){

        sprintf(str,"%d",int32Array[n]);
        strcat(strBuffer,str);
        if(n<len-1){
            strcat(strBuffer,",");
            //printf("%s\n",strBuffer);
        }

    }

}

void longToBytes(uint32_t value, uint8_t *buffer,int endian){

	uint8_t tempBuffer[4];

	if(endian == ENDIAN_MSB){
		tempBuffer[3] =  value & 0xff;
		tempBuffer[2] =  value>>8 & 0xff;
		tempBuffer[1] =  value>>16 & 0xff;
		tempBuffer[0] =  value>>24 & 0xff;
	}
	else{
		tempBuffer[0] =  value & 0xff;
		tempBuffer[1] =  value>>8 & 0xff;
		tempBuffer[2] =  value>>16 & 0xff;
		tempBuffer[3] =  value>>24 & 0xff;

	}
	for(int n=0;n<4;n++){
		buffer[n] = tempBuffer[n];
	}
}



/*
	test for longToBytes
	uint32_t v = 0x557e89f3;
	uint8_t testBuff[4];
	longToBytes(v,testBuff,ENDIAN_MSB);

	printf("%2x",testBuff[0]);
	printf("%2x",testBuff[1]);
	printf("%2x",testBuff[2]);
	printf("%2x",testBuff[3]);
	printf("\n");

	longToBytes(v, testBuff,ENDIAN_LSB);

	printf("%2x",testBuff[0]);
	printf("%2x",testBuff[1]);
	printf("%2x",testBuff[2]);
	printf("%2x",testBuff[3]);
	printf("\n");

*/


