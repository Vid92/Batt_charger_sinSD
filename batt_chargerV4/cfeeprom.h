#ifndef CFEEPROM_H_
#define CFEEPROM_H_
#include "I2CEEPROM.h"
#include <ArduinoJson.h>
//#include "globals.h"
#include "crc.h"

//#define DISK1 0x50 // eeprom program default
//#define DISK2 0x57

extern I2CEEPROM i2c_disk1;
extern I2CEEPROM i2c_disk2;

extern int const seconds_1hour;
extern int const seconds_1min;
extern int const less_than10;

extern char type[17][10];
extern float duration[17]; //unsigned long
extern float AmperH[17];
extern float current[17];
extern float maxtemp[17];
extern float mintemp[17];
//extern unsigned int myaddress;
extern float totalTime;

extern char totalhms[9];
extern char nameProg[11];
extern char ipName[10]; //nombre ip
extern char frag_ip[17]; //variable de almacenamiento para ip
extern char frag_mk[17]; //variable de almacenamiento para mask
extern char frag_gt[17]; //variable de almacenamiento para gateway

extern char rgt[3]; //variable almacenamiento
extern char vartmp[3]; //variable de almacenamiento auxiliar
extern char lett_final[2];//variable de almacenamiento para identificar minutos o seg

extern bool flagload;
//extern int disk1;
//extern int disk2;
//extern bool flagpause;
extern int save;
extern int read;
extern int regT;

//extern String ip;
//extern String mask;
//extern String gateway;

void eepromsave(unsigned int address, byte data);
void jsonSave(char* temp);
void eepromread(void);
void clearProgram(void);
void loadProgram(void);
//void currentTime(unsigned long Seg);
//void cleanEeprom(void);
void ipFileName(void);

#endif
