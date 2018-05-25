/******************************************************************************
* Copyright (C) 2018 By Mike Sharps
*   upload.h
*/

void sendDataToWeb(struct PTMeasuredData *measData);
int sendAirQDataToWeb(uint8_t *measData);
void bytesToHexString(uint8_t *byteBuffer,char *strBuffer,int byteLen);

