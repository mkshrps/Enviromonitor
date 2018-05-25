/******************************************************************************
* Copyright (C) 2018 By Mike Sharps
*   upload.c
*/

void displayHexMessage(unsigned char * buffer, int len);
void displayByteArray(uint8_t *buffer,int len);
void displayIntArray(int *buffer,int len);
void bytesToHexString(uint8_t *byteBuffer,char *strBuffer,int byteLen); 
int bytesToInt(unsigned char loByte, unsigned char hiByte);
void PTBytesToInts(char *dataptr,int *intptr);
void intsToStr(int *intArray,char *strBuffer,int len);
void uint32ToStr(uint32_t *int32Array,char *strBuffer,int len);
void longToBytes(uint32_t value, uint8_t *buffer,int endian);
