#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "control.h"
#include <Arduino.h>

void Control::begin(){
  int result=dac.begin(xmDAC::SINGLE_CHANNEL_MODE, xmDAC::VREF_VCC);
  dac.write(0); //FFF
}

//----------------------------- Set Current -----------------------------------//
void Control::setCurrent(float val_control){
  this->val_control = val_control;
}

//------------------------------ Set Time ------------------------------------//
void Control::setTime(unsigned long timeout){
  this->timeout = timeout * 1000;
  /*if(this->timeout!=0){
    if (this->timeout <= 60000) //diferenciar entre seg. o min-hor
    {
      this->time1 = 900; //500
      //this->time2 = 1200; //1200
    }
  }*/
}
//------------------------------ Set AmpHour ---------------------------------//
void Control::setAmpHour(float valAmpHour){
  this->valAmpHour = valAmpHour;
  this->timeAH = this->valAmpHour / (this->val_control * 0.000277); //valor en seg
  /*if(this->timeAH !=0){
    /*if(this->timeAH <=60){
      this->time1 = 900; //valor ms 500
      this->time2 = 1200; //900
    }
  }*/
}
//---------------------------- Set Temperature --------------------------------//
void Control::setTemperature(float maxTemp,float minTemp){
  this->maxTemp = maxTemp;
  this->minTemp = minTemp;
}

//----------------------------- Control-Run ------------------------------------//
void Control::run() {
  this->state = 1;
  digitalWrite(LedRelay, HIGH);
  if((this->prevstate == 0 || this->prevstate == 3 || this->prevstate == 4 || this->prevstate == 1) && this->state == 1){
    control_cbuff("R");
    //this->flagP = false; //ojo!!
    controlTime.start();
    Debug4.println(F("RUN"));
  }
}
//----------------------------- Control-Pause ---------------------------------//
void Control::pause() {
  this->state = 2;
  this->flagEnable = true;
}
//----------------------------- Control-Stop -----------------------------------//
void Control::stop() {
  this->state = 3;
  //digitalWrite(LED_BUILTIN, LOW);
}
//--------------------------- Control Run-Pause --------------------------------//
void Control::runPause(){
  this->state = 4;

  valcurrent = 0;
  valvoltage = 0;
  valtemp = 0;

  controlTime.stop();
  while(this->valrampa > 0)
  {
    this->valrampa--;
    //dac.write(0xFFF-this->valrampa);
    //vTaskDelay(1);
    rampa();
  }

  digitalWrite(LedRelay, LOW);

  if((this->prevstate == 0 || this->prevstate == 3 || this->prevstate == 1 || this->prevstate == 4) && this->state == 4){
    controlTime.start();
  }
}

void Control::rampa(){
  dac.write(this->valrampa); //dac.write(0xFFF-this->valrampa);
  vTaskDelay(1);
}

void Control::control_cbuff(const char* stepLetter){
  memset(stepState,0,2);
  strcat(stepState,stepLetter);
}
//------------------------------- Control-StepPause ----------------------------//
void Control::stepPause(unsigned long steptime){
  this->steptime = steptime * 1000;
}
//------------------------------ Control-states --------------------------------//
bool Control::states(){
  return this->flagS;
}
//---------------------------- Control-ReadData --------------------------------//
void Control::readData(){
  this->averageCurrent = 0;
  this->averageVoltage = 0;
  this->averageTemp  = 0;

  for(int i = 0; i < 70; i++) //promedio Current,Voltage,Temperature //40
  {
    this->averageCurrent = this->averageCurrent + analogRead(A0);
    this->averageVoltage = this->averageVoltage + analogRead(A1);
    this->averageTemp = this->averageTemp + analogRead(A2);
  }
  this->averageCurrent = this->averageCurrent / 70;//270 o mas ,rampa lenta
  this->averageVoltage = this->averageVoltage / 70;
  this->averageTemp = this->averageTemp / 70;

  this->valcurrent0 = this->averageCurrent - 70.0; //36** Esto esta por que no da 0, llega como 5mv

  //valcurrent = this->valcurrent0 * 150.0 / 1023.0;
  //valcurrent = this->valcurrent0 * 35.0 / 1023.0;
  valcurrent = this->valcurrent0 * 300.0 / 1023.0;

  this->valvoltage0 = this->averageVoltage - 44.0;
  valvoltage = this->valvoltage0 * 500.0 / 1023.0;
  valtemp = this->averageTemp * 120.0 / 1023.0;

  //valAH = valcurrent * 0.000277 * controlTime.ms() * 0.001;
  this->time = int(controlTime.ms() * 0.001);
  this->valAHact = valcurrent * 0.000277; //* this->time;
  if ((this->valAHact > 0.0) && (this->prevtime != this->time)){
      valAH = this->valAHact + valAH;}

  this->prevtime = this->time;
  //vTaskDelay(1);
}
//------------------------------ Control-event -----------------------------------//
void Control::event() {
  //here your logic to control the current
  if(this->state == 1)
  {
      if(controlTime.isRunning())//Program running
      {
        if(this->timeout!=0){ //Running with time
          if(controlTime.ms() < this->timeout)
          {
              //------------------------- flag Temperature -----------------------//
              /*if(this->flagTemp != false){
                this->flagTemp = false;
                this->t2 = controlTime.ms() + 30000;
              }*/
              //------------------------- control current ------------------------//
              if(valcurrent < this->val_control)
              {
                if (this->valrampa<0xFFF)
                  this->valrampa++;
              }

              if(valcurrent > this->val_control)
              {
                if(this->valrampa > 0)
                  this->valrampa--;
              }
              dac.write(this->valrampa); //dac.write(0xFFF-this->valrampa);
              vTaskDelay(1);
             //------------------------- current Warning -------------------------//
             if (valcurrent<=0 && valrampa >= 1638){ //rampa en 40% - 100% 4095
               Debug4.println(F("save1"));
               control_cbuff("W"); this->flagS = true;
             }
             //-------------------------- control temp -----------------------//
             //else if(this->maxTemp!=0){
             if(this->maxTemp!=0){
               if(valtemp > this->maxTemp){ //se va a pause
                 this->state = 2; Debug4.println(F("Pause-temp"));
                 control_cbuff("T");
                 this->flagEnable = true;
                 this->flagTemp = true;
               }
             }
          }
          else
          {
            Debug4.println(F("timeout-agotado"));
            this->Ttime0 = Ttime;
            Ttime = this->Ttime0 + (controlTime.ms()*0.001);
            totAH = totAH + valAH;
            valAH = 0.0;
            controlTime.stop();
            flagStep=true;      //bandera indica cambio de paso
          }
        }
        //----------------------------- running with Amp-hour --------------------------//
        else
        {
          if(valAH < this->valAmpHour)
          {  //-------------------------- flag Temperature ---------------------//
             /*if(this->flagTemp != false){
               this->flagTemp = false;
               this->t2 = controlTime.ms() + 30000;
               //Debug4.print(F("t2:"));  //Debug4.println(this->t2);
             }*/
             //----------------------- control current -------------------------//
             if(valcurrent < this->val_control)
             {
               if(this->valrampa<0xFFF)
                 this->valrampa++;
             }

             if(valcurrent > this->val_control)
             {
               if(this->valrampa > 0)
                 this->valrampa--;
             }
             dac.write(this->valrampa); //dac.write(0xFFF-this->valrampa);
             vTaskDelay(1);

             //------------------------ current Warning ------------------------//
             if (valcurrent<=0 && valrampa >= 1638){ //rampa en 40% - 100% 4095
               Debug4.println(F("save1"));
               control_cbuff("W"); this->flagS = true;
             }
             //------------------------- control Temp -------------------------//
             if(this->maxTemp!=0){
               if(valtemp > this->maxTemp){ //se va a pause
                 this->state = 2; Debug4.println(F("Pause-temp"));
                 control_cbuff("T");
                 this->flagEnable = true;
                 this->flagTemp = true;
               }
             }
          }
          else
          {
            Debug4.println(F("timeout-agotado"));
            this->Ttime0 = Ttime;
            Ttime = this->Ttime0 + (controlTime.ms()*0.001);
            totAH = totAH + valAH;
            valAH = 0.0;
            controlTime.stop();
            flagStep=true;      //bandera indica cambio de paso
          }
        }
      }
  }
//----------------------------------- Step Pause ----------------------------------//
  if(this->state == 4)
  {
    if(controlTime.isRunning())
    {
      if(controlTime.ms() > this->steptime)
      {
        Debug4.println(F("stepPause out"));
        flagStep=true;        //bandera indica cambio de paso
        this->Ttime0 = Ttime;
        Ttime = this->Ttime0 + (controlTime.ms()*0.001);
        controlTime.stop();  //
      }
    }
      vTaskDelay(1);
  }

//------------------------------------- STOP ---------------------------------------//
  if(this->state == 3)
  {
    if(this->prevstate != 3){
      this->flagTemp = false;
      this->flagEnable = false;

      controlTime.stop();
      Debug4.println(F("STOP"));
    }
  //------------- Control current -------------//
    if(this->valrampa > 0)
    {
      this->valrampa--;  //dac.write(0xFFF-this->valrampa);
      dac.write(this->valrampa);
      vTaskDelay(1);
    }
    else{
      this->valrampa = 0;
      dac.write(0);
      vTaskDelay(1);

      this->flagS = false;
      this->state = 0;
      control_cbuff("S");
      digitalWrite(LedRelay, LOW);
      digitalWrite(LED_BUILTIN, LOW);
    }
  }

//----------------------------------- PAUSE ---------------------------------------//
  if(this->state == 2 && this->flagEnable !=false)
  {
    if(this->prevstate!= 2){
      controlTime.pause();
      Debug4.println(F("PAUSE"));
    }
    //---------------- Warning temperatura -----------------//
    if(this->flagTemp != false){
      this->flagTemp = false;
      this->flagP = false;
    }
    else{ this->flagP = true; }
   //------------------- Control current --------------------//
    if(this->valrampa > 0){
      this->valrampa--;
      dac.write(this->valrampa);  //dac.write(0xFFF-this->valrampa);
      vTaskDelay(1);
    }
    else{
      this->valrampa = 0;
      dac.write(0);
      vTaskDelay(1);
      if(this->flagS != true) // Si es True es Flag Warning Current
        control_cbuff("P");

      this->flagS = false;
      this->flagEnable = false;
      digitalWrite(LedRelay, LOW); //deshabilita relay
    }
  }
  //------------------------------ Temperature good ------------------------------//
  //if(this->prevstate == 2 &&this->state == 2 && this->flagPause !=true && this->flagP != true){
  if(this->prevstate == 2 && this->state == 2 && this->flagP != true){
    if(valtemp <= minTemp){
      this->state = 1;
      this->flagP = true;
      control_cbuff("R");
      Debug4.println(F("good-temp"));
    }
  }
//------------------------------- Run-play after pause ----------------------------//
  if(this->prevstate == 2 && (this->state == 1 || this->state == 4))
  {
    controlTime.play();
    control_cbuff("R");
    if(this->state==4)digitalWrite(LedRelay, LOW);
    if(this->state==1)digitalWrite(LedRelay, HIGH);
  }

  this->prevstate = this->state;
}
