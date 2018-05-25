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
#include <curl/curl.h>
#include "base64.h"
#include "misc.h"
#include "plantower.h"

 
size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata){
    return size;
}

int sendDataToWeb(struct PTMeasuredData *measData){

	CURL *curl;
    CURLcode res;
    char curl_error[100];

    /* get a curl handle */
    curl = curl_easy_init(  );
    if ( curl )
    {
        char url[200];
        char base64_data[1000];
        size_t base64_length;
        //SHA256_CTX ctx;
        //unsigned char hash[32];
        //char doc_id[100];
        char json[1000], now[32];
        char uploadBuff[512];
        struct curl_slist *headers = NULL;
        time_t rawtime;
        struct tm *tm;
		int retries;
		long int http_resp;
        char write_data[1024];
        // Get formatted timestamp
        time( &rawtime );
        tm = gmtime( &rawtime );
        strftime( now, sizeof( now ), "%Y-%0m-%0dT%H:%M:%SZ", tm );

        // convert the PT data to a string of decimal values
        char strPTData[256];
        strPTData[0] ='\0';
        
        intsToStr(measData->PTData,strPTData,12);
        
        char tempStr[30];

        sprintf(tempStr,"%d",measData->temperature);
        strcat(strPTData,",");
        strcat(strPTData,tempStr);

        sprintf(tempStr,"%d",measData->pressure);
        strcat(strPTData,",");
        strcat(strPTData,tempStr);

        sprintf(tempStr,"%d",measData->humidity);
        strcat(strPTData,",");
        strcat(strPTData,tempStr);


        //printf("Raw PT data: ");
        //displayByteArray(measData->rawPTData,24);
        
        printf("Data for upload %s\n",strPTData);
        
        strcpy(uploadBuff,strPTData);
        

        // So that the response to the curl PUT doesn't mess up my finely crafted display!
        //curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_data );

        // Set the timeout
        curl_easy_setopt( curl, CURLOPT_TIMEOUT, 15 );

        // RJH capture http errors and report
        // curl_easy_setopt( curl, CURLOPT_FAILONERROR, 1 );
        curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, curl_error );

        // Avoid curl library bug that happens if above timeout occurs (sigh)
        curl_easy_setopt( curl, CURLOPT_NOSIGNAL, 1 );

        
        // Convert sentence to base64 for transmission to the web server
        base64_encode( uploadBuff, strlen(uploadBuff), &base64_length,
                       base64_data );
        base64_data[base64_length] = '\0';



        // Take SHA256 hash of the base64 version and express as hex.  This will be the document ID
        /*
        sha256_init( &ctx );
        sha256_update( &ctx, base64_data, base64_length );
        sha256_final( &ctx, hash );
        hash_to_hex( hash, doc_id );
*/
 
       // char counter[10];
       // sprintf( counter, "%d", t->Packet_Number );

        // Create json with the base64 data in hex, the tracker callsign and the current timestamp
        sprintf( json,
                 "{\"data\": {\"_raw\": \"%s\"},\"receivers\": {\"%s\": {\"time_created\": \"%s\",\"time_uploaded\": \"%s\"}}}",
                 base64_data, "MyData", now, now );
        
        //printf("%s\n",json);

        // LogTelemetryPacket(json);

        // Set the URL that is about to receive our PUT
//        sprintf( url, "http://habitat.habhub.org/habitat/_design/payload_telemetry/_update/add_listener/%s", doc_id);
//        sprintf( url, "http://192.168.1.6:1880/habitat/");

// use this if writing to the raspi local address
        //sprintf( url, "http://127.0.0.1:1880/habitat/");

        // send to the node red http endpoint url 
        sprintf( url, "http://192.168.1.6:1880/habitat/");

        // Set the headers
        headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8" );

        // PUT to http://habitat.habhub.org/habitat/_design/payload_telemetry/_update/add_listener/<doc_id> with content-type application/json
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        //curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);

		retries = 0;
		do
		{
			// Perform the request, res will get the return code
			res = curl_easy_perform( curl );

			// Check for errors
			if ( res == CURLE_OK )
			{
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_resp);
				if (http_resp != 201 && http_resp != 403 && http_resp != 409)
				{
					//LogMessage("Unexpected HTTP response %ld for URL '%s'\n", http_resp, url);
				}
			}
			else
			{
				http_resp = 0;
				//LogMessage("Failed for URL '%s'\n", url);
				//LogMessage("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				//LogMessage("error: %s\n", curl_error);
			}
			
			if (http_resp == 409)
			{
				// conflict between us and another uploader at the same time
				// wait for a random period before trying again
				//delay(random() & 255);		// 0-255 ms
			}
		} while ((http_resp == 409) && (++retries < 5));

        // always cleanup
        curl_slist_free_all( headers );
        curl_easy_cleanup( curl );
    }
    return(0);
}







