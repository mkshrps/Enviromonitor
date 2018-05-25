/******************************************************************************
plantower.h
Raspberry Pi I2C BME280 temp, humidity, barometric pressure 

By Mike Sharps
9/5/2018

Raspberry Pi 
I2C interface,for BME 280 using Bosch BME280 API and wiringpi I2C drivers
UART Interface for PTower air qulity monitor


*/
#ifndef PLANTOWER 
#define PLANTOWER 

#define CMDSIZE			5
#define	CMD_BUFF_LEN	7
#define STARTBYTE1		0x42
#define	STARTBYTE2		0x4D
#define FRAME_LEN_HI	0x02
#define FRAME_LEN_LO	0x03
#define PT_HEADER_ERROR		0x01
#define	PT_CHECKSUM_ERROR	0x02
#define	PT_LEN_ERROR		0x03
#define	ERR_OK				0x00
#define	MAX_BUFFER_LEN 		0x30
#define	MSG_HEADER_1		0x00
#define	MSG_HEADER_2		0x01
#define	MSG_LEN_HI			0x02
#define	MSG_LEN_LO			0x03
#define	RESPONSE_MSG_LEN	0x20

#define	AIRQ_INIT			0x00
#define	AIRQ_POLL			0x01 // manually poll the device
#define	AIRQ_AUTO_RX		0x02 // Just wait for data no command required
#define AIRQ_WAITING		0x30 // waiting for response

#define BME_INIT			0x01
#define BME_READ			0x02


#define	POLL_DELAY_5		0x05 // loop delay in seconds
#define	POLL_DELAY_10		0x10
#define	POLL_DELAY_30		0x30
#define	POLL_DELAY_60		0x60

#define	ENDIAN_MSB	1
#define	ENDIAN_LSB	0

struct termios options ;



// possible use for later
/*
struct PTMeasuredData
{
	
	unsigned char lock;
	int 	pm1Standard;
	int 	pm25Standard;
	int 	pm10Standard;
	int 	pm1Atmospheric;
	int 	pm25Atmospheric;
	int 	pm10Atmospheric;
	int 	pm03um;
	int 	pm05um;
	int 	pm10um;
	int 	pm25um;
	int 	pm50um;
	int 	pm100um;
	double	temperature;
	double	humidity;
	double	pressure;
};
*/


struct PTMeasuredData
{	
	unsigned char lock;
	uint8_t rawPTData[24];
	int PTData[12];
	uint32_t	temperature;
	uint32_t	humidity;
	uint32_t	pressure;
	unsigned char rawBMEData[12];
};


#endif

