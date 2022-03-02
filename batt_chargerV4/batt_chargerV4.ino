#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "comms.h"
//#include "Timer.h"
//#include "globals.h"
//#include <SPI.h>

#include <Wire.h>
#include <Arduino.h>

#define DISK1 0x50 // eeprom program default
#define DISK2 0x57 // ip,mask, gaterway

void vUARTTask(void *pvParameters);
//void vLEDFlashTask1(void *pvParameters);
void vCONTROLTask(void *pvParameters);
void vPROGRAMTask(void *pvParameters);

StopWatch controlTime;
Control control;
Program program;

I2CEEPROM i2c_disk1(DISK1);
I2CEEPROM i2c_disk2(DISK2);

int const seconds_1hour = 3600;
int const seconds_1min = 60;
int const less_than10 = 10;

int const LedComms = 23; //PD2 muestra led commms
int const LedRelay = 20; //D0 activa o desactiva relevador
int const rstTcp = 21;   //D1 activa o desactiva reset TCP
int const ledvolt = 0;

int count = 0;
unsigned int address = 1;
unsigned long Ttime = 0;
unsigned long timeC = 0;
unsigned long timeAc = 0;

bool flageeprom=false;
bool flagcommand=false;
bool flagbuff=false;
bool flagStep=false;

double valcurrent = 0.0; //solo para mostrar
double valvoltage = 0.0;
double valtemp = 0.0;
float valAH = 0.0;

char* letter = "Off"; //0x49; //I char

char frag_ip[17]; //variable de almacenamiento para ip
char frag_mk[17]; //variable de almacenamiento para mask
char frag_gt[17]; //variable de almacenamiento para gateway

char ip_tcp[17]; //variable de almacenamiento para ip proveniente de TCP
char mk_tcp[17]; //variable de almacenamiento para mask proveniente de TCP
char gt_tcp[17]; //variable de almacenamiento para gateway proveniente de TCP

char rcvchar=0x00;   // último carácter recibido
char stepState[2] = {"I"};
char timehms[9] = {"00:00:00"};
char timehmsa[9] = {"00:00:00"};
char totalhms[9];// = {"00:00:00"};
char nameProg[11];

char timetemp[9];

char hor0[4];
char mint0[4];
char seg0[3];
char hor1[4];
char mint1[4];
char seg1[3];

char type[17][10]; //inicio, pausa,carga, fin
float duration[17]; //time
float AmperH[17];
float current[17];
float maxtemp[17];
float mintemp[17];

char fileName[15] = "";
char ipName[10];

bool flag = false;
bool flagcompar = true;
bool flagConfig = false;
bool openInit = false;
bool flagATZ = true;

void setup()
{
  //baudios
  Wire.begin();
  control.begin();

  Debug4.begin(115200); //115200
  Serial.begin(115200);

  const char compile_date[] = __DATE__ " " __TIME__; //fecha-hora-compilacion
  Debug4.println(compile_date);
  Debug4.flush();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LedRelay, OUTPUT);
  pinMode(LedComms, OUTPUT);
  pinMode(rstTcp, OUTPUT);

  digitalWrite(rstTcp, HIGH); //se resetea con high en programa
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LedRelay, LOW);
  digitalWrite(LedComms, LOW);

  delay(2000);
  //lee lo que hay en eeprom ip, gaterway, mask

  ipFileName();
  strcat(fileName,ipName);
  strcat(fileName,".xls");
  //Debug4.println(fileName);
  clearProgram();
  loadProgram();

  delay(2500);
  if(!openInit){ Serial.write("+++"); openInit = true;}

  xTaskCreate(vUARTTask,
              (const portCHAR *)"UARTTask",
              512,
              NULL,
              tskIDLE_PRIORITY + 3,
              NULL);

/*  xTaskCreate(vLEDFlashTask1,
              (const portCHAR *)"Task1",
              configMINIMAL_STACK_SIZE,
              NULL,
              tskIDLE_PRIORITY + 1,
              NULL);*/

  xTaskCreate(vPROGRAMTask,
              (const portCHAR *)"PROGRAMTask",
              configMINIMAL_STACK_SIZE,
              NULL,
              tskIDLE_PRIORITY + 1,
              NULL);

  xTaskCreate(vCONTROLTask,
              (const portCHAR *)"CONTROLTask",
              256,
              NULL,
              tskIDLE_PRIORITY + 2,
              NULL);

  vTaskStartScheduler();
}

/*
void vLEDFlashTask1(void *pvParameters){
  (void) pvParameters;
  for (;;) {
    vTaskDelay(500);
    digitalWrite(27, !HIGH);
    vTaskDelay(500);
    digitalWrite(27, !LOW);
    vTaskDelay(1);
  }
}*/

void vPROGRAMTask(void *pvParameters){
  (void) pvParameters;
  for (;;) {
    program.process_step();
    if(flagStep)
    {
      flagStep = false;
      program.nextStep();
    }
    if(!flag)ReadTCP();
    if(!flagcompar){comparValues();}
    if(flagConfig){ConfigTCP();}
    //if(!flagATZ){restartAT();}
    vTaskDelay(1);
  }
}

void vCONTROLTask(void *pvParameters){
  (void) pvParameters;
  for (;;) {
    control.readData();
    vTaskDelay(1);
    control.event();
    if(millis()%50==0){ //dato a modificar*
      printMessage();
      digitalWrite(LedComms, LOW);
    }
  }
}

void vUARTTask(void *pvParameters){
  (void) pvParameters;
  for (;;) {
    if (flagcommand)comms_procesa_comando();
    serialEvent();
    vTaskDelay(5);
  }
}

void printMessage()
{
    memset(timehms,0,9);
    memset(timehmsa,0,9);
    timeC = controlTime.ms()*0.001;
    currentTime(timeC);
    strcat(timehms,timetemp);

    timeAc = Ttime + (controlTime.ms()*0.001);
    currentTime(timeAc);
    strcat(timehmsa,timetemp);
}

//---------------------- función para formato hor:min:seg ----------------------//
void currentTime(unsigned long timeSeg){
  int seg = 0;
  int mint = 0;
  int hor = 0;
  int tmp = 0;

  memset(hor0,0,4); memset(mint0,0,4); memset(seg0,0,3);
  memset(hor1,0,4); memset(mint1,0,4); memset(seg1,0,3);

  if(timeSeg > 3599){
    hor = timeSeg / seconds_1hour; // horas
    tmp = timeSeg - (hor * seconds_1hour);

    if(hor < less_than10){
      sprintf(hor1,"%d",hor);
      strcat(hor0,"0"); strcat(hor0,hor1); strcat(hor0,":");
    }
    else{
      sprintf(hor1,"%d",hor);
      strcat(hor0,hor1); strcat(hor0,":");
    }

    if (tmp > 59){
      mint = tmp / seconds_1min; //minutos
      seg = tmp - (mint * seconds_1min); //segundos
      if(seg < less_than10){
        sprintf(seg1,"%d",seg);
        strcat(seg0,"0"); strcat(seg0,seg1);
      }
      //else if(seg > less_than10){
      else{
        sprintf(seg1,"%d",seg);
        strcat(seg0,seg1);
      }
      if(mint < less_than10){
        sprintf(mint1,"%d",mint);
        strcat(mint0,"0");strcat(mint0,mint1); strcat(mint0,":");
      }
      else{
        sprintf(mint1,"%d",mint);
        strcat(mint0,mint1); strcat(mint0,":");
      }
    }
    else{
      seg = tmp;
      if(tmp < less_than10){
        sprintf(seg1,"%d",tmp);
        strcat(seg0,"0"); strcat(seg0,seg1);
      }
      else{
        sprintf(seg1,"%d",tmp);
        strcat(seg0,seg1);
      }
      strcat(mint0,"00:");
    }
  }
  else if (timeSeg > 59){
  //if (timeSeg > 59){
    mint = timeSeg / seconds_1min; //minutos
    seg = timeSeg - (mint * seconds_1min); //segundos

    if (seg < less_than10){
      sprintf(seg1,"%d",seg);
      strcat(seg0,"0"); strcat(seg0,seg1);
    }
    //else if(seg > less_than10){
    else{
      sprintf(seg1,"%d",seg);
      strcat(seg0,seg1);
    }
    //else if(mint < less_than10){
    if(mint < less_than10){
      sprintf(mint1,"%d",mint);
      strcat(mint0,"0"); strcat(mint0,mint1);strcat(mint0,":");
    }
    //else if(mint > less_than10){
    else{
      sprintf(mint1,"%d",mint);
      strcat(mint0,mint1);strcat(mint0,":");
    }
    strcat(hor0,"00:");
  }
  else if(timeSeg < 59){
    seg = timeSeg;
    if (seg < less_than10){
      sprintf(seg1,"%d",seg);
      strcat(seg0,"0"); strcat(seg0,seg1);
    }
    else{
      sprintf(seg1,"%d",seg);
      strcat(seg0,seg1);
    }
    strcat(mint0,"00:");
    strcat(hor0,"00:");
  }
  memset(timetemp,0,9);
  strcat(timetemp,hor0); strcat(timetemp,mint0); strcat(timetemp,seg0);
}

void loop()
{
}

//------------------ Interrupción recepción serie USART ------------------------//
void serialEvent(){
  flagbuff = false;
  rcvchar=0x00;                // Inicializo carácter recibido
  if (Serial.available()) {
        digitalWrite(LedComms, HIGH);
        rcvchar = Serial.peekLast();
        if(rcvchar == 0x6B){
          rcvchar=Serial.read();
          comms_addcbuff(rcvchar);
        }
        if(rcvchar==0x0A){
          rcvchar=Serial.read();
          comms_addcbuff(rcvchar);
        }
        if(rcvchar == 0x61){
          Serial.write("a");
          rcvchar=Serial.read();
          comms_inicbuff();
        }
        if(rcvchar == 0x04){
          while(Serial.available()){  // Si hay algo pendiente de recibir ...
          rcvchar=Serial.read();    // lo descargo y ...
          comms_addcbuff(rcvchar);   // lo añado al buffer y ...
          }
        }
    }
}
