#include "program.h"
#include <Arduino.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

void Program::runStep(){  //inicia proceso
  this->state0 = 1;
  state0 = 1;
  //flagCapt = false;
  control.control_cbuff("R");
  //*digitalWrite(LED_BUILTIN, HIGH); //E1
  Debug4.println(F("Run"));
  Ttime=0;
  if((type[count][0]) == 'E'){
    count = 0;
  }
}

void Program::pauseStep(){  //pausa proceso
  this->state0 = 2;
  control.control_cbuff("P");;
  Debug4.println(F("Pause"));
}

void Program::stopStep(){   //detiene proceso
  this->state0 = 3;
  //flagCapt = true;    //deshabilita captura de datos
  Debug4.println(F("Stop"));
}

void Program::nextStep(){
  this->state0 = 1;
  state0 = 1;
  count = count + 1;

  if(count>16) count = 0;
}

void Program::process_step()
{
  if(this->state0 == 1)
  {
    Debug4.println(F("entro-program"));
    inicio:
    switch (type[count][0]){
      case 'B':  //Begin
      letter = "B";
      count = count + 1;
      Debug4.println(F("Step-Begin"));
      goto inicio;
      break;

      case 'C':  //Charge
      letter = "Charge"; //C
      Debug4.println(F("Step-Charge"));
      control.setCurrent(current[count]);
      control.setTime(duration[count]);
      control.setAmpHour(AmperH[count]);
      control.setTemperature(maxtemp[count],mintemp[count]);
      control.run();
      break;

      case 'P':  //Pause
      letter = "Pause";
      Debug4.println(F("Step-Pause"));
      control.stepPause(duration[count]);
      control.runPause();
      break;

      case 'E':  //End
      letter = "Ended";
      Ttime = 0;
      totAH = 0.0;
      control.stop();
      control.control_cbuff("E");
      Debug4.println(F("Step-End"));
      break;
      default:break;
    }
    this->state0=4;
  }

  if(control.states())
  {
    this->state0 = 3;
  }

  if(this->prevstate0!=3 && this->state0 == 3){
    control.stop();
    Ttime=0;
    count = 0;
    totAH = 0.0;
    letter = "Off";
  }

  if(this->prevstate0!= 2 && this->state0 == 2){
     control.pause();
  }

  this->prevstate0 = this->state0;
}
