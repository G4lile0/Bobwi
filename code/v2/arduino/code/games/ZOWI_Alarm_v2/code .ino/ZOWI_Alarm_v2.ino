
//----------------------------------------------------------------
//-- Zowi Game: Alarm  v2
//-- (c) BQ. Released under a GPL licencse
//-- 04 December 2015
//-- Authors:  Anita de Prado: ana.deprado@bq.com
//--           Jose Alberca:   jose.alberca@bq.com
//-----------------------------------------------------------------
//-- Experiment with all the features that Zowi have!
//-----------------------------------------------------------------

#include <Servo.h>
#include <Oscillator.h>
#include <EEPROM.h>
#include <BatReader.h>
#include <US.h>
#include <LedMatrix.h>

//-- Library to manage external interruptions
#include <EnableInterrupt.h> 

//-- Library to manage serial commands
#include <ZowiSerialCommand.h>
ZowiSerialCommand SCmd;  //The SerialCommand object

//-- Zowi Library
#include <Zowi.h>
Zowi zowi;  //This is Zowi!!

//---------------------------------------------------------
//-- Configuration of pins where the servos are attached
/*
         --------------- 
        |               |
        |     O   O     |
        |               |
 YR ==> |               | <== YL
         --------------- 
            ||     ||
            ||     ||
            ||     ||
 RR ==>   -----   ------  <== RL
          -----   ------
*/

  #define PIN_YL 2 //servo[0]
  #define PIN_YR 3 //servo[1]
  #define PIN_RL 4 //servo[2]
  #define PIN_RR 5 //servo[3]
//---------------------------------------------------------

//---Zowi Buttons
#define PIN_SecondButton 6
#define PIN_ThirdButton 7


///////////////////////////////////////////////////////////////////
//-- Global Variables -------------------------------------------//
///////////////////////////////////////////////////////////////////

const char programID[]="ZOWI_Alarm_v2"; //Each program will have a ID

//-- Movement parameters
int T=1000;              //Initial duration of movement
int moveId=0;            //Number of movement
int moveSize=15;         //Asociated with the height of some movements


//---------------------------------------------------------
//-- Adivinawi has 5 modes:
//--    * MODE = 0: Zowi is awaiting  
//--    * MODE = 1: Arming alarm system  
//--    * MODE = 2: Zowi Guardian 
//--    * MODE = 3: --   
//--    * MODE = 4: ZowiPAD or any Teleoperation mode (listening SerialPort). 
//--
volatile int MODE=0; //State of zowi in the principal state machine. 
//--------------------------------------------------------- 


volatile bool buttonPushed=false;  //Variable to remember when a button has been pushed
volatile bool buttonAPushed=false; //Variable to remember when A button has been pushed
volatile bool buttonBPushed=false; //Variable to remember when B button has been pushed

unsigned long previousMillis=0;

int randomDance=0;
int randomSteps=0;

bool obstacleDetected = false;

bool alarmActivated = false;
int initDistance = 999;
unsigned long int arming_symbol=   0b00111111100001100001100001111111;
unsigned long int alarm_symbol=    0b00111111111111111111111111111111;

int angryPos2[4]=    {90, 90, 70, 110};
int headLeft2[4]=    {110, 110, 90, 90};
int headRight2[4]=   {70, 70, 90, 90};

///////////////////////////////////////////////////////////////////
//-- Setup ------------------------------------------------------//
///////////////////////////////////////////////////////////////////
void setup() {

  //Serial communication initialization
  Serial.begin(115200);  

  pinMode(PIN_SecondButton,INPUT);
  pinMode(PIN_ThirdButton,INPUT);
  
  //Set the servo pins
  zowi.init(PIN_YL,PIN_YR,PIN_RL,PIN_RR,true);
 
  //Uncomment this to set the servo trims manually and save on EEPROM 
    //zowi.setTrims(TRIM_YL, TRIM_YR, TRIM_RL, TRIM_RR);
    //zowi.saveTrimsOnEEPROM(); //Uncomment this only for one upload when you finaly set the trims.

  //Set a random seed
  randomSeed(analogRead(A6));

  //Interrumptions
  enableInterrupt(PIN_SecondButton, secondButtonPushed, RISING);
  enableInterrupt(PIN_ThirdButton, thirdButtonPushed, RISING);

  //Setup callbacks for SerialCommand commands 
  SCmd.addCommand("S", receiveStop);      //  sendAck & sendFinalAck
  SCmd.addCommand("R", receiveName);      //  sendAck & sendFinalAck
  SCmd.addCommand("E", requestName);
  SCmd.addCommand("B", requestBattery);
  SCmd.addCommand("I", requestProgramId);
  SCmd.addDefaultHandler(receiveStop);


  //Zowi wake up!
  zowi.sing(S_connection);
  zowi.home();


  //Send Zowi name, programID & battery level.
  requestName();
  delay(50);
  requestProgramId();
  delay(50);
  requestBattery();
  
  //Checking battery
  ZowiLowBatteryAlarm();


 // Animation Uuuuuh - A little moment of initial surprise
 //-----
  for(int i=0; i<2; i++){
      for (int i=0;i<8;i++){
        if(buttonPushed){break;}  
        zowi.putAnimationMouth(littleUuh,i);
        delay(150);
      }
  }
 //-----


  //Smile for a happy Zowi :)
  if(!buttonPushed){ 

    zowi.playGesture(ZowiHappy);
    zowi.sing(S_happy);
    delay(200);
  }

  if(!buttonPushed){ 
    zowi.putMouth(happyOpen);
  }

  previousMillis = millis();

}



///////////////////////////////////////////////////////////////////
//-- Principal Loop ---------------------------------------------//
///////////////////////////////////////////////////////////////////
void loop() {


  if (Serial.available()>0 && MODE!=4){

    MODE=4;
    zowi.putMouth(happyOpen);

    //Disable Pin Interruptions
    disableInterrupt(PIN_SecondButton);
    disableInterrupt(PIN_ThirdButton);

    buttonPushed=false;
  }


  //First attemp to initial software
  if (buttonPushed){  

    zowi.home();

    delay(100); //Wait for all buttons 
    zowi.sing(S_buttonPushed);
    delay(200); //Wait for all buttons 

    if      ( buttonAPushed && !buttonBPushed){ MODE=1; zowi.sing(S_mode1);}
    else if (!buttonAPushed && buttonBPushed) { MODE=2; zowi.sing(S_mode2);}
    else if ( buttonAPushed && buttonBPushed) { MODE=2; zowi.sing(S_mode2);} //else

    zowi.putMouth(MODE);
 
    int showTime = 2000;
    while((showTime>0)){ //Wait to show the MODE number 
        
        showTime-=10;
        delay(10);
    }
     
    zowi.clearMouth(); 
    

    buttonPushed=false;
    buttonAPushed=false;
    buttonBPushed=false;
    alarmActivated = false;
    previousMillis = millis();

  }else{

    switch (MODE) {

      //-- MODE 0 - Zowi is awaiting
      //---------------------------------------------------------
      case 0:
      
        //Every 80 seconds in this mode, Zowi falls asleep 
        if (millis()-previousMillis>=80000){
            ZowiSleeping_withInterrupts(); //ZZzzzzz...
            previousMillis=millis();         
        }

        break;
      

      //-- MODE 1 - Zowi Arming alarm system
      //---------------------------------------------------------
      case 1:
        

        //Arming alarm system
        if(alarmActivated == false){
          
          ZowiArmingAlarmSystem();

        }else{

          delay(100);
          int obstacleDistance = zowi.getDistance();
          int noise = zowi.getNoise();
          delay(100);
        
          //ALARM!!!!
          if ((noise>=680)||(obstacleDistance < initDistance)){
  
              delay(50);
              while(!buttonPushed){

                  zowi.putMouth(alarm_symbol,0);
                  zowi.bendTones (note_A5, note_A7, 1.04, 5, 2);  //A5 = 880 , A7 = 3520
          
                  delay(20);

                  zowi.bendTones (note_A7, note_A5, 1.02, 5, 2);  //A5 = 880 , A7 = 3520
                  zowi.clearMouth();
                  delay(300);
              } 
          }

        }
        
        

        
        break;


      //-- MODE 2 - Zowi Guardian
      //---------------------------------------------------------  
      case 2:
       
        //Arming alarm system
        if(alarmActivated == false){
          
          ZowiArmingAlarmSystem();

        }else{

          delay(100);
          int obstacleDistance = zowi.getDistance();
          int noise = zowi.getNoise();
          delay(100);
        
          //ALARM!!!!
          if ((noise>=680)||(obstacleDistance < initDistance)){
  
              delay(50);
              while(!buttonPushed){

                  zowi.putMouth(alarm_symbol,0);
                  zowi.bendTones (note_A5, note_A7, 1.04, 5, 2);  //A5 = 880 , A7 = 3520
          
                  delay(20);

                  zowi.bendTones (note_A7, note_A5, 1.02, 5, 2);  //A5 = 880 , A7 = 3520
                  zowi.clearMouth();
                  delay(300);
              } 
          }

          if (millis()-previousMillis>=8000){ //8sec
             
             ZowiGuardian();
              
              if (!buttonPushed){   

                delay(100);
                initDistance = zowi.getDistance();
                delay(100);
                initDistance -= 10;
              }
                
              previousMillis=millis(); 

          }

        }

        break;

     
      case 4:

        SCmd.readSerial();        
        break;      


      default:
          MODE=4;
          break;
    }

  } 

}  



///////////////////////////////////////////////////////////////////
//-- Functions --------------------------------------------------//
///////////////////////////////////////////////////////////////////

//-- Function executed when second button is pushed
void secondButtonPushed(){ 

    buttonAPushed=true;

    if(!buttonPushed){
        buttonPushed=true;
        zowi.putMouth(smallSurprise);
    }    
}

//-- Function executed when third button is pushed
void thirdButtonPushed(){ 

    buttonBPushed=true;

    if(!buttonPushed){
        buttonPushed=true;
        zowi.putMouth(smallSurprise);
    }
}



//-- Function to receive Stop command.
void receiveStop(){

    sendAck();
    zowi.home();
    sendFinalAck();

}


//-- Function to receive Name command
void receiveName(){

    //sendAck & stop if necessary
    sendAck();
    zowi.home(); 

    char newZowiName[11] = "";  //Variable to store data read from Serial.
    int eeAddress = 5;          //Location we want the data to be in EEPROM.
    char *arg; 
    arg = SCmd.next(); 
    
    if (arg != NULL) {

      //Complete newZowiName char string
      int k = 0;
      while((*arg) && (k<11)){ 
          newZowiName[k]=*arg++;
          k++;
      }
      
      EEPROM.put(eeAddress, newZowiName); 
    }
    else 
    {
      zowi.putMouth(xMouth);
      delay(2000);
      zowi.clearMouth();
    }

    sendFinalAck();

}


//-- Function to send Zowi's name
void requestName(){

    zowi.home(); //stop if necessary

    char actualZowiName[11]= "";  //Variable to store data read from EEPROM.
    int eeAddress = 5;            //EEPROM address to start reading from

    //Get the float data from the EEPROM at position 'eeAddress'
    EEPROM.get(eeAddress, actualZowiName);

    Serial.print(F("&&"));
    Serial.print(F("E "));
    Serial.print(actualZowiName);
    Serial.println(F("%%"));
    Serial.flush();
}



//-- Function to send battery voltage percent
void requestBattery(){

    zowi.home();  //stop if necessary

    //The first read of the batery is often a wrong reading, so we will discard this value. 
    double batteryLevel = zowi.getBatteryLevel();

    Serial.print(F("&&"));
    Serial.print(F("B "));
    Serial.print(batteryLevel);
    Serial.println(F("%%"));
    Serial.flush();
}


//-- Function to send program ID
void requestProgramId(){

    zowi.home();   //stop if necessary

    Serial.print(F("&&"));
    Serial.print(F("I "));
    Serial.print(programID);
    Serial.println(F("%%"));
    Serial.flush();
}


//-- Function to send Ack comand (A)
void sendAck(){

  delay(30);

  Serial.print(F("&&"));
  Serial.print(F("A"));
  Serial.println(F("%%"));
  Serial.flush();
}


//-- Function to send final Ack comand (F)
void sendFinalAck(){

  delay(30);

  Serial.print(F("&&"));
  Serial.print(F("F"));
  Serial.println(F("%%"));
  Serial.flush();
}



//-- Functions with animatics
//--------------------------------------------------------

void ZowiLowBatteryAlarm(){

    double batteryLevel = zowi.getBatteryLevel();

    if(batteryLevel<45){
        
      while(!buttonPushed){

          zowi.putMouth(thunder);
          zowi.bendTones (880, 2000, 1.04, 8, 3);  //A5 = 880
          
          delay(30);

          zowi.bendTones (2000, 880, 1.02, 8, 3);  //A5 = 880
          zowi.clearMouth();
          delay(500);
      } 
    }
}

void ZowiSleeping_withInterrupts(){

  int bedPos_0[4]={100, 80, 60, 120}; //{100, 80, 40, 140}

  if(!buttonPushed){
    zowi._moveServos(700, bedPos_0);  //800  
  }

  for(int i=0; i<4;i++){

    if(buttonPushed){break;}
      zowi.putAnimationMouth(dreamMouth,0);
      zowi.bendTones (100, 200, 1.04, 10, 10);
    
    if(buttonPushed){break;}
      zowi.putAnimationMouth(dreamMouth,1);
      zowi.bendTones (200, 300, 1.04, 10, 10);  

    if(buttonPushed){break;}
      zowi.putAnimationMouth(dreamMouth,2);
      zowi.bendTones (300, 500, 1.04, 10, 10);   

    delay(500);
    
    if(buttonPushed){break;}
      zowi.putAnimationMouth(dreamMouth,1);
      zowi.bendTones (400, 250, 1.04, 10, 1); 

    if(buttonPushed){break;}
      zowi.putAnimationMouth(dreamMouth,0);
      zowi.bendTones (250, 100, 1.04, 10, 1); 
    
    delay(500);
  } 

  if(!buttonPushed){
    zowi.putMouth(lineMouth);
    zowi.sing(S_cuddly);
  }

  zowi.home();
  if(!buttonPushed){zowi.putMouth(happyOpen);}  
}

void ZowiArmingAlarmSystem(){

    int countDown = 10000; //10 sec
    while((countDown>0)&&(!buttonPushed)){ 
  
      countDown-=1000;
      zowi.putMouth(arming_symbol,0);
      zowi._tone(note_A7,50,0); //bip'
      zowi.clearMouth();
      delay(950);   

      if(buttonPushed){break;}

    }

    if(!buttonPushed){
      alarmActivated = true;
      delay(100);
      initDistance = zowi.getDistance();
      delay(100);
      initDistance -= 10;
      previousMillis=millis(); 
    }
}

void ZowiGuardian(){

    int fretfulPos[4]=  {90, 90, 90, 110};

    if (!buttonPushed){ 
      zowi.putMouth(smallSurprise); 
      delay(100);
      zowi.sing(S_cuddly);
      delay(500);
    }  
    
    if (!buttonPushed){ 
      zowi.putMouth(angry);
      zowi.bendTones(note_A5, note_D6, 1.02, 20, 4);
      zowi.bendTones(note_A5, note_E5, 1.02, 20, 4);
      delay(300);
      zowi.putMouth(lineMouth);
    }  

    
    for(int i=0; i<4; i++){
        if (buttonPushed){break;}
        zowi._moveServos(100, fretfulPos);   
        zowi.home();
    }  

    if (!buttonPushed){ 
        zowi.putMouth(angry);
        delay(500); 
    }  


    if (!buttonPushed){ 
        zowi._moveServos(1000, headLeft2);
        delay(400);
    }

    if (!buttonPushed){ 
        zowi.home();
        delay(400);
    }

    if (!buttonPushed){   
        zowi._moveServos(1000, headRight2); 
        delay(400);
    }

    if (!buttonPushed){ 
        delay(300);
        zowi.home();
        zowi.putMouth(smile); 
        delay(100);
    }
      
    if (!buttonPushed){   
        zowi.sing(S_happy_short);
        delay(800);
        zowi.clearMouth();
    }       

}