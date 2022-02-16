#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "cfeeprom.h"
#include <Arduino.h>

const char* type0;

char sg[3];
char mm[4];
char hr[4];

char hr1[4];
char mm1[4];
char sg1[3];

char final_var[15]= "";
char var_tmp[2];

void eepromsave(unsigned int address, byte data){ //comparacion CR1 CR2 enviado
  //save=0;
  //Debug4.print(F("index: "));
  //Debug4.println(index);
  //Debug4.println(tmp);
  //for(int i = 0; i<1024;i++){ //
    //if(tmp[i]== 0xFF || tmp[i] == 0){
    //  break;
  //}
    //EEPROM.write(i+1,tmp[i]);
  i2c_disk1.write(address,0);
    //save++;
    //writeEEPROM(disk1,i+10,tmp[i]); //0-10 prog.default
  //}
}

void ipFileName(){
  //char frag_ip[17]; //variable de almacenamiento para ip
  //char frag_mk[17]; //variable de almacenamiento para mask
  //char frag_gt[17]; //variable de almacenamiento para gateway

  //memset(frag_ip,0,sizeof(frag_ip));
  //memset(frag_mk,0,sizeof(frag_mk));
  //memset(frag_gt,0,sizeof(frag_gt));
  //memset(ipName,0,sizeof(ipName));

  memset(frag_ip,0,17);
  memset(frag_mk,0,17);
  memset(frag_gt,0,17);
  memset(ipName,0,17);

  int val = 0;
  int count = 0;
  int an = 0;
  int x = 0;
  int y = 0;
  int m = 0;

  for(int i=0;i<17;i++){ // lee la ip de la eeprom
    val = i2c_disk2.read(i); //i2c_eeprom.read(i);
    if(val== 0x2C){ //cuando se muestra coma ,
      x = i+1;
      break;
    }
    frag_ip[i] = char(val); // guarda la ip completa en frag_ip
  }
  //ip = frag_ip;
  Debug4.print("ip:");
  Debug4.println(frag_ip);

  for(int i=x;i<x+17;i++){ // lee la mask de la eeprom
    val = i2c_disk2.read(i); //i2c_eeprom.read(i);
    if(val== 0x2C){ //cuando se muestra coma ,
      y = i+1;
      break;
    }
    frag_mk[m] = char(val); // guarda la mask completa en frag_mk
    m++;
  }
  m = 0;
  //mask = frag_mk;
  Debug4.print("mk:");
  Debug4.println(frag_mk);

  for(int i=y;i<y+17;i++){ // lee la gateway de la eeprom
    val = i2c_disk2.read(i); //i2c_eeprom.read(i);
    if(val== 0x2C){ //cuando se muestra coma ,
      break;
    }
    frag_gt[m] = char(val); // guarda la gateway completa en frag_gt
    m++;
  }
  //gateway = frag_gt;
  Debug4.print("gt:");
  Debug4.println(frag_gt);

  for(int n=0;n<16;n++){ //Toma los ultimos dos octetos de frag_ip para el filename
    int data = frag_ip[n];
    if((data == 0x2E) and (count<2)){
      count++;
    }
    if(count>=2){
      //ipName[an] = char(data);
      ipName[an] = frag_ip[n]; //+1
      if(frag_ip[n] == 0x2E){ //Cambia . por _
        ipName[an] = 0x5F;
      }
      an++;
    }
  }
  //return ipName;
}

void jsonSave(char* temp){
  int n = 0;
  int sg0 = 0;
  int mm0 = 0;
  int hr0 = 0;

  float vnew = 0.0;
  float mmf = 0.0;
  float timeAH = 0.0;
  float totalAH = 0.0;
  float totalDuration = 0.0;

  memset(rgt,0,3);
  memset(vartmp,0,3);
  memset(lett_final,0,2);

  for(int x=0; x<5;x++){
      memset(var_tmp,0,2);
      sprintf(var_tmp,"%c",temp[x]);
      strcat(nameProg,var_tmp);
    }
    Debug4.println(nameProg);
    //for(int i=0;i<strlen(temp);i++){
    for(int i=0;;i++){
      if(temp[i]==0x5D)break;

      if(temp[i] == 'T'){
        n+=1;
        type0 = "Type";
        switch (temp[i+4]){
          case 'B':
          type0 = "Begin";
          break;

          case 'P':
          type0 = "Pause";
          break;

          case 'C':
          type0 = "Charge";
          break;

          case 'E':
          type0 = "End";
          for (int x=i+8;x++;){
            if(temp[x]==0x4D || temp[x] == 0x53){
              if(temp[x]==0x4D)
                strcat(lett_final,"M");
              else
                strcat(lett_final,"S");
              break;
            }
            sprintf(vartmp,"%c",temp[x]);
            strcat(rgt,vartmp);
          }
          //hacer converciones para capturar dato cada 
          regT = atoi(rgt);
          Debug4.println(regT);
          Debug4.println(lett_final);
          break;
          default:break;
        }
      }
      else if(temp[i] == 'H' && temp[i+34] == '}'){
          for(int x=7;x<11;x++){
            memset(var_tmp,0,2);
            sprintf(var_tmp,"%c",temp[i+x]);
            strcat(final_var,var_tmp);
          }
          duration[n-1] = (atof(final_var)) * seconds_1hour;
          totalDuration = totalDuration + duration[n-1];
          memset(final_var,0,15);
      }

      else if(temp[i] == 'C' && temp[i+45] == '}'){
          for(int x=4;x<8;x++){
            memset(var_tmp,0,2);
            sprintf(var_tmp,"%c",temp[i+x]);
            strcat(final_var,var_tmp);
          }
          current[n-1] = atof(final_var);
          memset(final_var,0,15);
      }

      else if(temp[i] == 'A' && temp[i+34] == '}'){
          for(int x=4;x<11;x++){
            memset(var_tmp,0,2);
            sprintf(var_tmp,"%c",temp[i+x]);
            strcat(final_var,var_tmp);
          }
          AmperH[n-1] = atof(final_var);
          timeAH = AmperH[n-1] / (0.000277 * current[n-1]);
          totalAH = totalAH + timeAH;
          memset(final_var,0,15);
      }

      else if(temp[i] == 'M' && temp[i+20] == '}'){
          for(int x=4;x<8;x++){
            memset(var_tmp,0,2);
            sprintf(var_tmp,"%c",temp[i+x]);
            strcat(final_var,var_tmp);
          }
          maxtemp[n-1] = atof(final_var);
          memset(final_var,0,15);
      }

      else if(temp[i] == 'm' && temp[i+9] == '}'){
          for(int x=4;x<8;x++){
            memset(var_tmp,0,2);
            sprintf(var_tmp,"%c",temp[i+x]);
            strcat(final_var,var_tmp);
          }
          mintemp[n-1] = atof(final_var);
          memset(final_var,0,15);
      }

      if(type0!="End"){
        //strlcpy(type[n-1], type0,sizeof(type[n-1]));
        memcpy(type[n-1],type0,sizeof(type0[n-1]));
      }
      else{
        memcpy(type[n-1],type0,sizeof(type0[n-1]));
        break;
      }
    }
    totalTime = totalAH + totalDuration;
    flagload = true;
    memset(hr,0,4);
    memset(mm,0,4);
    memset(sg,0,3);

    //currentTime(totalTime);

    if(totalTime < seconds_1hour){ //menor a 1 hora
      vnew = totalTime / seconds_1min;
      mm0 = int(vnew);
      strcat(hr,"00:");

      if (mm0 < less_than10){
        sprintf(mm1,"%d",mm0);
        strcat(mm,"0"); strcat(mm,mm1); strcat(mm,":");
      }
     else{
        sprintf(mm1,"%d",mm0);
        strcat(mm,mm1); strcat(mm,":");
      }
      sg0 = int(((vnew - mm0) * seconds_1min));

      if (sg0 < less_than10){
        sprintf(sg1,"%d",sg0);
        strcat(sg,"0"); strcat(sg,sg1);
      }
      else{
        sprintf(sg1,"%d",sg0);
        strcat(sg,sg1);
      }
    }
    else{ // mayor a 1 hora
      vnew = totalTime / seconds_1hour;
      hr0 = int(vnew);

      if(hr0 < less_than10){
        sprintf(hr1,"%d",hr0);
        strcat(hr,"0"); strcat(hr,hr1); strcat(hr,":");
      }
      else{
        sprintf(hr1,"%d",hr0);
        strcat(hr,hr1); strcat(hr,":");
      }
      mmf = ((vnew - hr0) * seconds_1min);
      mm0 = int(mmf);

      if (mm0 < less_than10){
        sprintf(mm1,"%d",mm0);
        strcat(mm,"0"); strcat(mm,mm1); strcat(mm,":");
      }
      else{
        sprintf(mm1,"%d",mm0);
        strcat(mm,mm1); strcat(mm,":");
      }
      sg0 = int(((mmf - mm0) * seconds_1min));

      if (sg0 < less_than10){
        sprintf(sg1,"%d",sg0);
        strcat(sg,"0"); strcat(sg,sg1);
      }
      else{
        sprintf(sg1,"%d",sg0);
        strcat(sg,sg1);
      }
    }
    memset(totalhms,0,9);
    strcat(totalhms,hr); strcat(totalhms,mm); strcat(totalhms,sg);
    Debug4.print(F("totalhms: "));
    Debug4.println(totalhms);
}

void eepromread(){
  char eeprom[1024];
  memset(eeprom,0,sizeof(eeprom));
  //read=0;
  for(int i = 0;;i++){
    int val = i2c_disk1.read(i);
    if(val==0x5D)break;
    /*if(val==0xFF||val== 0){
      break;
    }*/
    //read++;
    eeprom[i] = char(val);
    Debug4.print(eeprom[i]);
  }
    Debug4.println();
    jsonSave(eeprom);
}


void clearProgram(){
  for(int i=0; i< 17;i++){
    memset(&type[i],0,10*sizeof(char));
    duration[i]=0;
    AmperH[i]=0;
    current[i]=0;
    maxtemp[i]=0;
    mintemp[i]=0;
  }
  Debug4.println(F("clean"));
}


/*void cleanEeprom(){ // i+=100
  for (int i = 0; i <1024 ; ++i) { //1024 1330 //1698
    i2c_disk1.write(i,0);
  }
}*/

void loadProgram(){
  eepromread();
}
