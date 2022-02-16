#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "comms.h"
//#include "Timer.h"
//#include "globals.h"
#include <SPI.h>
#include <SD.h>

#include <DS3231.h>

#include <Wire.h>
#include <Arduino.h>

#define DISK1 0x50 // eeprom program default
#define DISK2 0x57 // ip,mask, gaterway

void vUARTTask(void *pvParameters);
void vLEDFlashTask1(void *pvParameters);
void vCONTROLTask(void *pvParameters);
void vPROGRAMTask(void *pvParameters);
void vRTCTask(void *pvParameters);

StopWatch controlTime;
Control control;
Program program;

DS3231 clock;
File myFile;
RTCDateTime dts;

I2CEEPROM i2c_disk1(DISK1);
I2CEEPROM i2c_disk2(DISK2);

//int disk1 = 0x50;    //24LC256 eeprom chip program-default
//int disk2 = 0x57;    //24LC256 eeprom chip Address

int const seconds_1hour = 3600;
int const seconds_1min = 60;
int const less_than10 = 10;

int const LedRelay = 20; //D0 activa o desactiva relevador
int const rstTcp = 21;   //D1 activa o desactiva reset TCP
int const ledvolt = 0;

int count = 0;
unsigned int address = 1;
unsigned long Ttime = 0;
unsigned long timeC = 0;
unsigned long timeAc = 0;

unsigned long previousMillis = 0;
const long interval = 3000; //1000 equivale a 1 segundo

bool flageeprom=false;
bool flagcommand=false;
bool flagbuff=false;
bool flagStep=false;
bool flagCapt=true;

double valcurrent = 0.0; //solo para mostrar
double valvoltage = 0.0;
double valtemp = 0.0;
float valAH = 0.0;

char* letter = "Off"; //0x49; //I char

//String ip = "";
//String mask = "";
//String gateway = "";
//String datasTcp = "";

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
char timeTotal[9];
char dateTotal[11];
char timeDate[21];

char rgt[3];
char vartmp[3];
char lett_final[2];

char hor0[4];
char mint0[4];
char seg0[3];
char hor1[4];
char mint1[4];
char seg1[3];

char ss0[3];
char ss1[3];
char mt0[4];
char mt1[4];
char hh0[4];
char hh1[4];

char day[4];
char month[4];
char year[5];

char type[17][10]; //inicio, pausa,carga, fin
float duration[17]; //time
float AmperH[17];
float current[17];
float maxtemp[17];
float mintemp[17];

float TempTime1 = 0;

char fileName[15] = "";
char ipName[10];

char aux[30] = "";
bool flag = false;
bool flagcompar = true;
bool flagConfig = false;
bool openInit = false;
bool flagATZ = true;

int sgd = 0;
int mt = 0;
int hrs = 0;

int nday = 0;
int nmonth = 0;
int nyear = 0;

int regT = 0;

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
  pinMode(rstTcp, OUTPUT);

  digitalWrite(rstTcp, HIGH); //se resetea con high en programa
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LedRelay, LOW);

  delay(2000);
  //lee lo que hay en eeprom ip, gaterway, mask

  ipFileName();
  strcat(fileName,ipName);
  strcat(fileName,".xls");
  //Debug4.println(fileName);
  clearProgram();
  loadProgram();

  clock.begin();      //Initialize DS3231
  clock.setDateTime(__DATE__, __TIME__); //configura reloj tiempo actual

  if(!SD.begin(16)){    //Initializing SD card
    while (1);          //Initialization failed!
  }

  SdFile::dateTimeCallback(dateTime);
  if (SD.exists(fileName)){
     myFile = SD.open(fileName,FILE_WRITE);
  }
  else{
    myFile = SD.open(fileName,FILE_WRITE); //O_APPEND | O_WRITE);
    ReportBegin();
  }

  dts = clock.getDateTime(); //hacer un comando donde se actualice reloj
  //Debug4.println(clock.dateFormat("d-m-Y H:i:s", dts));
  sgd = dts.second;
  mt = dts.minute;
  hrs = dts.hour;

  nday = dts.day;
  nmonth = dts.month;
  nyear = dts.year;

  delay(2500);
  if(!openInit){ Serial.write("+++"); openInit = true;}

  xTaskCreate(vUARTTask,
              (const portCHAR *)"UARTTask",
              512,
              NULL,
              tskIDLE_PRIORITY + 3,
              NULL);

  xTaskCreate(vLEDFlashTask1,
              (const portCHAR *)"Task1",
              configMINIMAL_STACK_SIZE,
              NULL,
              tskIDLE_PRIORITY + 1,
              NULL);

  xTaskCreate(vPROGRAMTask,
              (const portCHAR *)"PROGRAMTask",
              configMINIMAL_STACK_SIZE,
              NULL,
              tskIDLE_PRIORITY + 1,
              NULL);

  xTaskCreate(vRTCTask,
              (const portCHAR *)"RTCTask",
              256, //128
              //configMINIMAL_STACK_SIZE,
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

void vLEDFlashTask1(void *pvParameters){
  (void) pvParameters;
  for (;;) {
    vTaskDelay(500);
    digitalWrite(27, !HIGH);
    vTaskDelay(500);
    digitalWrite(27, !LOW);
    vTaskDelay(1);
  }
}

void vRTCTask(void *pvParameters){
  (void) pvParameters;
  for (;;){
    //noInterrupts(); //interrupts();
    //dts = clock.getDateTime(); //actualiza
    if(!flagCapt){        //flag habilita tiempo
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval){
         previousMillis = currentMillis;
         if(myFile){
           //myFile.print(clock.dateFormat("d-m-Y H:i:s", dts));
           //timeSMH();
           myFile.print(timeDate);
           myFile.print(",");
           myFile.print(valcurrent);
           myFile.print(",");
           myFile.print(valvoltage);
           myFile.print(",");
           myFile.print(valtemp);
           myFile.print(",");
           myFile.print(valAH);
           myFile.print(",");
           myFile.print(TempTime1);
           myFile.print(",");
           myFile.print(count);
           myFile.print(",");
           myFile.print(letter);
           myFile.print(",");
           myFile.print(timehms);
           myFile.print(",");
           myFile.print(timehmsa);
           myFile.print(",");
           myFile.print(totalhms);
           myFile.print(",");
           myFile.println(stepState);
           myFile.flush();
         }
      }

      /*if (currentMillis - previousMillis >= ){ //Para 30 min
        previousMillis = currentMillis
        Debug4.println(F("Actualiza reloj"));
        dts = clock.getDateTime();
        sgd = dts.second;
        mt = dts.minute;
        hrs = dts.hour;

        nday = dts.day;
        nmonth = dts.month;
        nyear = dts.year;
      }*/
    }
    vTaskDelay(1);
  }
}

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

void ReportBegin() {
  if(myFile){
    myFile.print("Date"); myFile.print(","); myFile.print("Current"); myFile.print(","); myFile.print("Voltage"); myFile.print(",");
    myFile.print("Temperature"); myFile.print(","); myFile.print("Amper/Hour"); myFile.print(","); myFile.print("Amper Acu."); myFile.print(",");
    myFile.print("Step"); myFile.print(","); myFile.print("Step State"); myFile.print(","); myFile.print("Time"); myFile.print(",");
    myFile.print("Current Time"); myFile.print(","); myFile.print("Total Time"); myFile.print(","); myFile.println("Status");
    myFile.flush();
  }
}

void dateTime(uint16_t* date, uint16_t* time){
  RTCDateTime dt;
  dt = clock.getDateTime();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(dt.year, dt.month, dt.day);
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(dt.hour, dt.minute, dt.second);
}

//void timeSMH(int sg,int mt, int hr){
void timeSMH(){
  memset(ss0,0,3);  memset(mt0,0,4);    memset(hh0,0,4);    memset(day,0,4);    memset(year,0,5);
  memset(ss1,0,3);  memset(mt1,0,4);    memset(hh1,0,4);    memset(month,0,4);

  sgd += 1;
  if(sgd>59){
    sgd = 0;
    mt += 1;
  }
  if(mt>59){
    mt = 0;
    hrs += 1;
  }
  if(hrs>23){
    hrs = 0;
  }

  if(sgd>9){
    sprintf(ss0,"%d",sgd);
    strcat(ss1,ss0);
  }
  else{
    sprintf(ss0,"%d",sgd);
    strcat(ss1,"0"); strcat(ss1,ss0);
  }
  if(mt>9){
    sprintf(mt0,"%d",mt);
    strcat(mt1,mt0); strcat(mt1,":");
  }
  else{
    sprintf(mt0,"%d",mt);
    strcat(mt1,"0"); strcat(mt1,mt0); strcat(mt1,":");
  }

  if(hrs>9){
    sprintf(hh0,"%d",hrs);
    strcat(hh1,hh0); strcat(hh1,":");
  }
  else{
    sprintf(hh0,"%d",hrs);
    strcat(hh1,"0"); strcat(hh1,hh0); strcat(hh1,":");
  }
  memset(timeTotal,0,9);    memset(dateTotal,0,11);   memset(timeDate,0,21);
  strcat(timeTotal,hh1); strcat(timeTotal,mt1); strcat(timeTotal,ss1);      //almacena time h:m:s

  sprintf(day,"%d",nday);  sprintf(month,"%d",nmonth);  sprintf(year,"%d",nyear);
  strcat(dateTotal,day); strcat(dateTotal,"/");  strcat(dateTotal,month); strcat(dateTotal,"/"); strcat(dateTotal,year);    //almacena date dd/mm/yy

  strcat(timeDate,dateTotal); strcat(timeDate," - "); strcat(timeDate,timeTotal);        //almacena date-time
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

    timeSMH();

    //Debug4.println(F("Mensaje"));
    //Debug4.print(F("time: "));
    //Debug4.print(timehms);
    //Debug4.print(F(", stopwatch : "));
    //Debug4.print(controlTime.ms()*0.001);
    //Debug4.print(F(", Ttime : "));
    //Debug4.println(Ttime);
    //Debug4.print(F(", AH : "));
    //Debug4.println(valAH);
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

/*
void serialEvent() {
  //flagbuff = false;
  rcvchar = 0x00;
  //Debug4.print("Available: ");
  //Debug4.println(Serial.available());
  if (Serial.available()) {
    rcvchar = Serial.peekLast();
    Debug4.println(rcvchar, HEX);
    if (rcvchar == 0x04) {
      int cuenta = 0;
      Debug4.println("--MSG RECEIVED--BEGIN--");
      while (Serial.available()) {
        rcvchar = Serial.read();
        Debug4.println(rcvchar, HEX);
        cuenta ++;
      }
      Debug4.println("--MSG RECEIVED--END--");
      Debug4.print("LEN: ");
      Debug4.println(cuenta);
    }
  }
}*/
