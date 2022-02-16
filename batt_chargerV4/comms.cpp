#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "comms.h"
#include <Arduino.h>
#include "crc.h"

int const lenbuff = 128; // Longitud de buffer, Ajustar 128 512 1024
//int const lenfinal = 128;
//unsigned int myaddress = 1;   //255
const char a = 0x61; //tcp -aT
const char restchar = 0x52;  //restart tcp-AT
const char beginchar = 0x02; //inicio
const char endchar = 0x04;   //fin
const char runchar = 0x33;   //run
const char pausechar = 0x34; //pause
const char stopchar = 0x35;  //stop
const char combchar = 0x43;  //C = I,V,T
const char writechar = 0x57; //W
const char disk2char = 0x45;  //E write->ip,gt,mascara red
const char Ping = 0x64;  //default

const char var_pass[13] = "ACTION: PASS";
//const char var_passrun[17] = "ACTION: PASS,RUN";
const char* var_passrun = "ACTION: PASS,RUN";
const char* var_failrun = "ACTION: FAIL,RUN";
const char* var_passpause = "ACTION: PASS,PAUSE";
const char* var_failpause = "ACTION: FAIL,PAUSE";
const char* var_passtop = "ACTION: PASS,STOP";
const char* var_failtop = "ACTION: FAIL,STOP";

const char let_value[9] = "VALUE: I";
const char let_voltage[3]= ",V";
const char let_temp[3]= ",T";
const char let_Ah[4]= ",AH";
const char let_temptime1[4]= ",AC";
const char let_count[3]= ",P";
const char let_letter[3]= ",S";
const char let_timehms[3]= ",t";
const char let_timehmsa[4]= ",Tt";
const char let_totalhms[4]= ",TT";
const char let_nameProg[3]= ",N";
const char let_coma[2] = ",";

bool flagload = false;
bool flagrun = false;
bool flagpause = false;

bool initWrite = false;
//bool openInit = false;

int n = 0; //cuenta pasos eeprom 17
int x = 0; //control eeprom direcciones
int dato = 0;
int lenbff = 0;
int xbuff=0x00;      // Índice: siguiente char en cbuff

//float TempTime1 = 0; // aux AH acumulado
float totAH = 0.0;   //variable acum.AH
float totalTime;

char crc16_high = 0x00;
char crc16_low = 0x00;
char final[lenbuff];
char cbuff[lenbuff]; // Buffer
unsigned char tmp_crc[lenbuff]={0};

//----------------------------- Variables control prueba-------------------------
//char totalhms[9];// = "00:00:00"; //extern

char var_current[5];
char var_voltage[5];
char var_temp[5];
char var_Ah[5];
char var_temptime1[6];
char var_count[3];

char var_cmd[3];
char str_var[6];

//----------------------------- clean buffer ----------------------------------//
void comms_inicbuff(void){ // Inicia a 0 cbuff
  memset(cbuff,0,lenbuff); //limpia
  xbuff=0;//0x00            // Inicializo el índice de siguiente
  flagbuff=true;                    // carácter
}

//--------------------------- add data to buffer -----------------------------//
int comms_addcbuff(char c){ // Añade a cbuff
  switch(c){
    case endchar:           // Enter -> Habilita Flag para procesar
      cbuff[xbuff++]=c;       // Añade carácter recibido al Buffer
      flagcommand=true;     // Comando en Main
      flagbuff=true;
      break;
    default:
      cbuff[xbuff++]=c;       // Añade carácter recibido al Buffer
      break;
  }
}
//----------------------------------- CRC -------------------------------------//
void doCrc(int var_len){
  lenbff = 0;       //limpia valores
  crc16_high = 0x00;
  crc16_low = 0x00;
  memset(tmp_crc,0,lenbuff);

  for(int i=0;i<var_len;i++){
    tmp_crc[i]=final[i];
    lenbff++;
  }
  dato = crc16_SingleBuf(tmp_crc,lenbff); //crc
  crc16_high = highByte(dato);
  crc16_low = lowByte(dato);
}

void strcmdcat(int cmd,const char* text){
  dtostrf(cmd, 2, 0, str_var);
  sprintf(var_cmd,"%s",str_var);
  strcat(final,var_cmd);
  strcat(final,text);
}

//----------------------------------- TCP -------------------------------------//
void ReadTCP(){
  if(cbuff[0]==0x2B&&cbuff[1]==0x6F&&cbuff[2]==0x6B)//+ok
    {//+ok
      comms_inicbuff();
      Serial.print("AT+WANN\r\n"); //lee wann
    }
  if(cbuff[0]==0x0D&&cbuff[1]==0x0A&&cbuff[2]==0x2B&&cbuff[3]==0x4F&&cbuff[4]==0x4B&&cbuff[xbuff-2]==0x0D&&cbuff[xbuff-1]==0x0A)
    { //+OK=static,ip,mk,gt
      memset(final,0,lenbuff); //limpia antes de concatenar
      for(int i=0;i<xbuff+1;i++){
        final[i] = cbuff[i];
        Debug4.print(final[i]);
      }
      flag = true;
      flagcompar = false;
    }
}

void restartAT(){
  if(cbuff[0]==0x2B&&cbuff[1]==0x6F&&cbuff[2]==0x6B)//+ok
    {//+ok
      comms_inicbuff();
      Serial.print("AT+Z\r\n");
    }
  if(cbuff[0]==0x0D&&cbuff[1]==0x0A&&cbuff[2]==0x2B&&cbuff[3]==0x4F&&cbuff[4]==0x4B&&cbuff[5]==0x0D&&cbuff[6]==0x0A)
    { //+OK=
      comms_inicbuff();
      closeAt();
      flagATZ = true;
    }
}

//bool comparValues(){
void comparValues(){
  //Debug4.println(F("comparValues"));
  memset(ip_tcp,0,17);
  memset(mk_tcp,0,17);
  memset(gt_tcp,0,17);

  int x = 0;
  int y = 0;
  int z = 0;
  int val = 0;
  int n = 0;

  for(int i=0;;i++){ //+OK=STATIC, saca los valores ip,mk,gt
    val = final[i];
    if(val== 0x2C){
      x = i+1;
      val = 0;
      break;
    }
  }
  for(int i=x;i<x+17;i++){ //saca ip
    val = final[i];
    if(val== 0x2C){
      y = i+1;
      val = 0;
      break;
    }
    ip_tcp[n] = char(val);
    n++;
  }
  n=0;
  for(int i=y;;i++){ //saca mask
    val = final[i];
    if(val== 0x2C){
      z = i+1;
      val = 0;
      break;
    }
    mk_tcp[n] = char(val);
    n++;
  }
  n=0;
  for(int i=z;;i++){ //saca gateway
    val = final[i];
    if(val== 0x0D){
      break;
    }
    gt_tcp[n] = char(val);
    n++;
  }
  flagConfig = false;
  //Hace comparaciones en todos
  for(int i=0;i<17;i++){
    if(frag_ip[i]!=ip_tcp[i]){
      flagConfig = true;
    }
  }
  /*
  for(int i=0;i<17;i++){
    if(frag_mk[i]!=mk_tcp[i]){
      flagConfig = true;
    }
  }*/
  /*
  for(int i=0;i<17;i++){
    if(frag_gt[i]!=gt_tcp[i]){
      flagConfig = true;
    }
  }*/

  if(!flagConfig){
    closeAt();
  }
  //Debug4.println(frag_ip);
  //Debug4.println(ip_tcp);

  flagcompar = true;
}

void ConfigTCP(){ //guarda ip,mask,gateway en TCP
  if(cbuff[0]==0x0D&&cbuff[1]==0x0A&&cbuff[2]==0x2B&&cbuff[3]==0x4F&&cbuff[4]==0x4B&&cbuff[5]==0x3D&&cbuff[xbuff-2]==0x0D&&cbuff[xbuff-1]==0x0A)
    {//+ok
      //Debug4.println("CnfgTCP");
      memset(final,0,lenbuff);
      comms_inicbuff();
      strcat(final,"AT+WANN=STATIC,"); strcat(final,frag_ip); strcat(final,",");
      strcat(final,frag_mk); strcat(final,","); strcat(final,frag_gt); strcat(final,"\r\n");
      Serial.print(final);
    }

  if(cbuff[0]==0x0D&&cbuff[1]==0x0A&&cbuff[2]==0x2B&&cbuff[3]==0x4F&&cbuff[4]==0x4B&&cbuff[5]==0x0D&&cbuff[6]==0x0A)
    { //+OK=
      comms_inicbuff();
      closeAt();
      flagConfig = false;
    }
}

void closeAt(){
  Serial.print("at+entm\r\n");
  //Debug4.println(F("closeAT"));
  digitalWrite(rstTcp, LOW);
  vTaskDelay(10);
  digitalWrite(rstTcp, HIGH);
}

void ATRELD(){ //conf.fabrica ip,mask,gt //tal vez no necesaria*
  Serial.print("AT+RELD\r\n");
}

//---------------------------- Procesa comando -------------------------------//
void comms_procesa_comando(void){
  int i;
  int len=0;
  flagcommand=false;  // Desactivo flag de comando pendiente.
  flagbuff =false;

    //Debug4.println(F("Leyendo..."));
    /*for(int z = 0;z<xbuff;z++){
        Debug4.print(cbuff[z],HEX);
		    Debug4.print(F(","));
    }*/
    //Debug4.println(F("contador_carac: "));
    //Debug4.print(xbuff);

    //Debug4.println(F("-------------------"));
    //Debug4.println(cbuff[0],HEX);
    //Debug4.println(cbuff[xbuff-1],HEX);

    if(cbuff[0]==beginchar&&cbuff[xbuff-4]==0x03&&cbuff[xbuff-1]==endchar){
      memset(final,0,lenbuff); //limpia antes de concatenar
      memset(str_var,0,6);
      memset(var_cmd,0,3);

      for(int x=2;; x++){
        if(cbuff[x]==0x03)break;
        len++;
      }
      /*Debug4.println("tbuff: ");
      for(int z = 0; z<len; z++){
        Debug4.println(tbuff[z],HEX);
      }
      Debug4.println("--------------");
      Debug4.println(len);*/

      if(len<lenbuff){
        dato = crc16_SingleBuf((unsigned char*)&cbuff[2],len);
        crc16_high = highByte(dato);
        crc16_low = lowByte(dato);
      }
      //----------------------------- validacion CRC ------------------------------//
      if(cbuff[xbuff-3]==crc16_low && cbuff[xbuff-2]==crc16_high) //validacion CRC
      {
            //Debug4.println(F("valido crc"));
      //--------------------------comandos para mantenimiento----------------------//
            if(cbuff[1]==Ping){ //time stamp
              Debug4.println(F("PING"));
              strcat(final,var_pass);

              doCrc(strlen(final)); //crc
              Serial.write(2); Serial.print(var_pass); Serial.write(3); Serial.write(crc16_low);Serial.write(crc16_high); Serial.write(4);
            }

            if(cbuff[1]==disk2char){ //cambiar valor de ip,gt,mr en eeprom(disk2)
              Debug4.println(F("DISK2CHAR"));

              for(int x=0;x<49;x++){ //borra bloque eeprom 48 elementos
                i2c_disk2.write(x,0);
              }

              for(int i=2;;i++){ //escribe nuevos valores de ip,gt,mr en eeprom
                if(cbuff[i]==0x03)break;
                Debug4.print(cbuff[i]);
                i2c_disk2.write(i-2,cbuff[i]);
              }
              strcmdcat(69,var_pass);
              doCrc(strlen(final));
              Serial.write(2); Serial.write(69); Serial.print(var_pass); Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);
            }
            //hay que probarlo!!
            if(cbuff[1]==restchar){ //restart tcp-AT
              flagATZ = false;
              Serial.write("+++");  //openAt
            }
            /*if(cbuff[1]==){ //cambia el valor de tcp(ip,gt,mk)por lo escrito en eeprom(disk2)
              Debug4.println(F("Chg-TCP"));
              strcmdcat(69,var_pass);
              doCrc(strlen(final));
              Serial.write(2); Serial.write(69); Serial.print(var_pass); Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);
            }*/
      //-------------------------------comandos para usuario---------------------------------------//
            if(cbuff[1]==runchar){  //run
              if(!flagload){Debug4.println(F("cargo"));clearProgram();loadProgram();}
              if(flagload){
                flagrun = true; flagpause = false;
                program.runStep();
                strcmdcat(51,var_passrun);

                doCrc(strlen(final)); //crc
                Serial.write(2); Serial.write(51); Serial.print(var_passrun); Serial.write(3); Serial.write(crc16_low);Serial.write(crc16_high); Serial.write(4);
              }
              else
              {
                strcmdcat(51,var_failrun);
                doCrc(strlen(final));
                Serial.write(2); Serial.write(51); Serial.print(var_failrun); Serial.write(3); Serial.write(crc16_low);Serial.write(crc16_high); Serial.write(4);
              }
            }

            if(cbuff[1]==pausechar){  //pause
              if(flagrun){
                flagpause = true; flagrun=false;
                program.pauseStep();
                strcmdcat(52,var_passpause);

                doCrc(strlen(final));//crc
                Serial.write(2); Serial.write(52); Serial.print(var_passpause); Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);
              }
              else
              {
                strcmdcat(52,var_failpause);
                doCrc(strlen(final));
                Serial.write(2); Serial.write(52); Serial.print(var_failpause); Serial.write(3); Serial.write(crc16_low);Serial.write(crc16_high); Serial.write(4);
              }
            }

            if(cbuff[1]==stopchar){ //stop
              if(flagpause||flagrun){
                program.stopStep();
                strcmdcat(53,var_passtop);

                doCrc(strlen(final));
                Serial.write(2); Serial.write(53); Serial.print(var_passtop); Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);
              }
              if(!flagpause&&!flagrun)
              {
                strcmdcat(53,var_failtop);
                doCrc(strlen(final));
                Serial.write(2); Serial.write(53); Serial.print(var_failtop); Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);
              }
            }

            if(cbuff[1]==combchar){  //show I,V,T
              //Debug4.println(F("Datos"));

              TempTime1 = totAH + valAH;
              dtostrf(valcurrent, 2, 2, str_var);
              sprintf(var_current,"%s",str_var);
              memset(str_var,0,6);
              dtostrf(valvoltage, 2, 2,str_var);
              sprintf(var_voltage,"%s",str_var);
              memset(str_var,0,6);
              dtostrf(valtemp, 2, 2, str_var);
              sprintf(var_temp,"%s",str_var);
              memset(str_var,0,6);
              dtostrf(valAH, 2, 2, str_var);
              sprintf(var_Ah,"%s",str_var);
              memset(str_var,0,6);
              dtostrf(TempTime1, 2, 2,str_var);
              sprintf(var_temptime1,"%s",str_var);
              memset(str_var,0,6);
              dtostrf(count, 2, 0, str_var);
              sprintf(var_count,"%s",str_var);

              strcat(final,let_value);
              strcat(final,var_current);
              strcat(final,let_voltage);
              strcat(final,var_voltage);
              strcat(final,let_temp);
              strcat(final,var_temp);
              strcat(final,let_Ah);
              strcat(final,var_Ah);
              strcat(final,let_temptime1);
              strcat(final,var_temptime1);
              strcat(final,let_count);
              strcat(final,var_count);
              strcat(final,let_letter);
              strcat(final,letter);
              strcat(final,let_timehms);
              strcat(final,timehms);
              strcat(final,let_timehmsa);
              strcat(final,timehmsa);
              strcat(final,let_totalhms);
              strcat(final,totalhms);
              strcat(final,let_nameProg);
              strcat(final,nameProg);
              strcat(final,let_coma);
              strcat(final,stepState);

              doCrc(strlen(final)); //crc

              Serial.write(2);Serial.print(let_value);Serial.print(valcurrent); Serial.print(let_voltage);Serial.print(valvoltage);Serial.print(let_temp); Serial.print(valtemp);Serial.print(let_Ah);Serial.print(valAH);Serial.print(let_temptime1);Serial.print(TempTime1);Serial.print(let_count); Serial.print(count);Serial.print(let_letter);Serial.write(letter);Serial.print(let_timehms); Serial.print(timehms);Serial.print(let_timehmsa); Serial.print(timehmsa); Serial.print(let_totalhms); Serial.print(totalhms); Serial.print(let_nameProg); Serial.print(nameProg); Serial.print(F(",")); Serial.print(stepState);Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);
            }

            if(cbuff[1]==writechar){  //writting eeprom Json
              Debug4.println(F("Write"));
              if(n==0){
                i2c_disk1.write(1050,'F'); //bandera validar escritura eeprom
                Debug4.println("UNICA VEZ");
              }
              //n++; //cuenta los bloques entrantes 17
              if(cbuff[11]==0x5B){ //[ primer bloque 1 begin 10
                n = 1;
                Debug4.println(F("PrimerBloque"));
                for(int i = 2;; i++){ //save name program 8 caracteres max
                  if(cbuff[i]==0x5B)break;
                  //Debug4.print(cbuff[i]);
                  i2c_disk1.write(i-2,cbuff[i]); //0-7 eeprom + 2 espacios
                }
                Debug4.println();
                int t = 1;
                for(int i = 10;; i++){ //save 1 bloque program Begin
                  if(cbuff[i]==0x03)break;
                  if(cbuff[i]!='1'){ //omite 1 - cbuff[8]
                    Debug4.print(cbuff[i]);
                    //Debug4.print(i+t);
                    i2c_disk1.write(i+t,cbuff[i]); //eeprom empieza 11-69
                    x = i+t;
                  }
                  else{
                    t = 0;
                  }
                }
                x++;
                Debug4.println();
                Debug4.println(F("writingEEPROM"));
                Debug4.println(F("------------------"));
                Debug4.println(x);

                strcmdcat(87,var_pass);
                doCrc(strlen(final));//crc
                Serial.write(2); Serial.write(writechar); Serial.print(var_pass); Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);

                //flagload = false;
                /*if(len==save)
                {
                  clearProgram();
                  jsonSave(tmp);
                  //if (save == read){
                  if(flagload){
                    digitalWrite(LedComms, HIGH); Serial1.write(2); Serial1.print(myaddress); Serial1.write(writechar); Serial1.write("ACTION: PASS"); Serial1.write(3); Serial1.write(0); Serial1.write(0); Serial1.write(4); vTaskDelay(2); digitalWrite(LedComms, LOW);
                  }
                  else{
                    digitalWrite(LedComms, HIGH); Serial1.write(2); Serial1.print(myaddress); Serial1.write(writechar); Serial1.write("ACTION: FAIL"); Serial1.write(3);Serial1.write(0); Serial1.write(0); Serial1.write(4);vTaskDelay(2); digitalWrite(LedComms, LOW);
                  }
                }*/
              }
              else if(cbuff[3]==0x7B){ //{ bloques con un digito 2-9
                n++;
                Debug4.println(F("Bloque1Digito"));
                for(int i = 3;; i++){
                  if(cbuff[i]==0x03)break;
                  Debug4.print(cbuff[i]);
                  //Debug4.print(x);
                  i2c_disk1.write(x,cbuff[i]); //eeprom empieza
                  x++;
                }
                Debug4.println();
                Debug4.println(F("writingEEPROM"));
                Debug4.println(F("------------------"));
                Debug4.println(x);
                /*char buff_tmp[2] = {cbuff[2]};
                eepromsave(cbuff,buff_tmp);*/
                strcmdcat(87,var_pass);
                doCrc(strlen(final));//crc
                Serial.write(2); Serial.write(writechar); Serial.print(var_pass); Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);
              }
              else if(cbuff[4]==0x7B){ //{ bloques dos digitos 10-17
                n++;
                Debug4.println(F("Bloque dosDigitos"));
                for(int i = 4;; i++){
                  if(cbuff[i]==0x03)break;
                  Debug4.print(cbuff[i]);
                  //Debug4.print(x);
                  i2c_disk1.write(x,cbuff[i]); //eeprom empieza
                  x++;
                }
                Debug4.println();
                Debug4.println(F("writingEEPROM"));
                Debug4.println(F("------------------"));
                Debug4.println(x);

                strcmdcat(87,var_pass);
                doCrc(strlen(final));//crc
                Serial.write(2); Serial.write(writechar); Serial.print(var_pass); Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);
              }
              Debug4.print("n: ");
              Debug4.println(n);

              //strcmdcat(87,var_pass);
              //doCrc(strlen(final));//crc
              //Serial.write(2); Serial.write(writechar); Serial.print(var_pass); Serial.write(3); Serial.write(crc16_low); Serial.write(crc16_high); Serial.write(4);

              if(n>=17){ n=0; x=0; i2c_disk1.write(1050,'T'); Debug4.println("TODO"); }
            }

      }
      comms_inicbuff(); // Borro buffer.
      //Debug4.println("Procesado"); // Monitorizo procesado.
    }
    if(!flagbuff)comms_inicbuff(); // Borro buffer.
}
