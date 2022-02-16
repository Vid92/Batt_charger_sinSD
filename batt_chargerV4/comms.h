#ifndef COMMS_H_
#define COMMS_H_
#include "control.h"
#include "cfeeprom.h"
#include "program.h"
//#include "Timer.h"
//#include "globals.h"

extern Program program;
extern int const rstTcp;
extern char rcvchar;
extern char timehms[9];
extern char timehmsa[9];
extern char ip_tcp[17]; //variable de almacenamiento para ip proveniente de TCP
extern char mk_tcp[17]; //variable de almacenamiento para mask proveniente de TCP
extern char gt_tcp[17]; //variable de almacenamiento para gateway proveniente de TCP

extern bool flageeprom;
extern bool flagcommand;
extern bool flagbuff;

extern bool flag;
extern bool flagcompar;
extern bool flagConfig;
extern bool flagATZ;
extern float TempTime1;

void comms_inicbuff(void);        // Borra buffer
int comms_addcbuff(char c);       // añade carácter recibido al buffer
void comms_procesa_comando(void); // Procesa comando
void doCrc(int var_len);
void strcmdcat(void);

void ReadTCP(void);
void comparValues();
void ConfigTCP();
void openAt(void);
void closeAt(void);
void ATRELD(void);
void restartAT(void);

#endif
