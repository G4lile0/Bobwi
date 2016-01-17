
//----------------------------------------------------------------
//-- Zowi Game: Adivinawi  v2
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

const char programID[]="ZOWI_Adivinawi_v2"; //Each program will have a ID

//-- Movement parameters
int T=1000;              //Initial duration of movement
int moveId=0;            //Number of movement
int moveSize=15;         //Asociated with the height of some movements


//---------------------------------------------------------
//-- Adivinawi has 5 modes:
//--    * MODE = 0: Zowi is awaiting  
//--    * MODE = 1: Zowi Adivino Adivinawi  
//--    * MODE = 2: Zowi Dado Dice 
//--    * MODE = 3: Rock Paper Scissors   
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

unsigned long int rock_symbol=    0b00000000001100011110011110001100;
unsigned long int paper_symbol=   0b00011110010010010010010010011110;
unsigned long int scissors_symbol=0b00000010010100001000010100000010;


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
    else if ( buttonAPushed && buttonBPushed) { MODE=3; zowi.sing(S_mode3);} //else

    zowi.putMouth(MODE);
 
    int showTime = 2000;
    while((showTime>0)){ //Wait to show the MODE number 
        
        showTime-=10;
        delay(10);
    }
     
    
    zowi.putMouth(interrogation);                     
    zowi.sing(S_happy_short); 
    delay(200);

    buttonPushed=false;
    buttonAPushed=false;
    buttonBPushed=false;

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
      

      //-- MODE 1 - Zowi Adivino Adivinawi
      //---------------------------------------------------------
      case 1:
        
        delay(50);
        
        if (zowi.getNoise()>=680){ //740
  
          delay(50);
          ZowiMagics(2);

          int randomNum = random(1,3); //1,2  - YES, NO

          if (randomNum==1){ //1 = YES

              if (!buttonPushed){ 
                zowi._tone(note_E5,50,30);
                zowi.putMouth(okMouth);
              } 
              
              if (!buttonPushed){  
                zowi.sing(S_happy);
                zowi.swing(1,800,20); 
                zowi.sing(S_superHappy);
              }  


          }else{  //2 = NO

              if (!buttonPushed){ 
                zowi._moveServos(300, angryPos2); 
                zowi.putMouth(xMouth);
              }
              
              if (!buttonPushed){  
                zowi._tone(note_A5,100,30);
                zowi.bendTones(note_A5, note_D6, 1.02, 7, 4);
                zowi.bendTones(note_D6, note_G6, 1.02, 10, 1);
                zowi.bendTones(note_G6, note_A5, 1.02, 10, 1);
                delay(15);
              }

              if (!buttonPushed){ 
                zowi.bendTones(note_A5, note_E5, 1.02, 20, 4);
                delay(400);
                zowi._moveServos(200, headLeft2); 
              }
              
              if (!buttonPushed){   
                zowi.bendTones(note_A5, note_D6, 1.02, 20, 4);
                zowi._moveServos(200, headRight2); 
                zowi.bendTones(note_A5, note_E5, 1.02, 20, 4);
              }
          } 

          zowi.home();

          if (!buttonPushed){
            delay(2000);
          } 

          if (!buttonPushed){ 
            zowi.clearMouth();
            delay(500);
          }  

          if (!buttonPushed){
            zowi.putMouth(interrogation);                     
            zowi.sing(S_happy_short);  
          }

          if (!buttonPushed){
            zowi.clearMouth();
            delay(200); 
            zowi.putMouth(interrogation);
          }  

        }
        break;


      //-- MODE 2 - Zowi Dado Dice
      //---------------------------------------------------------  
      case 2:

        delay(50);

        if (zowi.getNoise()>=680){ //740
          
          delay(50);
          ZowiMagics(1);

          int randomNum = random(1,7); //1-6

          if (!buttonPushed){

            zowi.putMouth(randomNum);

            if(randomNum==1){     zowi.sing(S_sad);}
            else if(randomNum==6){zowi.sing(S_superHappy);}
            else{                 zowi.sing(S_connection);}
          }  
          
          for(int i=0; i<4; i++){
              if (!buttonPushed){ zowi.clearMouth(); delay(200);}
              if (!buttonPushed){ zowi.putMouth(randomNum); delay(200);}
          }
          
          if (!buttonPushed){
            delay(2300);
          }

          if (!buttonPushed){ 
            zowi.clearMouth();
            delay(100);
          }  

          if (!buttonPushed){
            zowi.putMouth(interrogation);                     
            zowi.sing(S_happy_short);  
          }

          if (!buttonPushed){
            zowi.clearMouth();
            delay(200); 
            zowi.putMouth(interrogation);
          } 

        }
 

        break;

      //-- MODE 3 - Rock Paper Scissors
      //---------------------------------------------------------
      case 3:

        delay(50);

        if (zowi.getNoise()>=680){ //740
          
          delay(50);
          
          switch (random(0,3)){

            case 0:
              if (buttonPushed){break;}
              zowi.putMouth(rock_symbol,0);
              zowi.sing(S_fart1);
            break;

            case 1:
              if (buttonPushed){break;}
              zowi.putMouth(paper_symbol,0);
              zowi.sing(S_OhOoh2);
            break;

            case 2:
              if (buttonPushed){break;}
              zowi.putMouth(scissors_symbol,0);
              zowi.sing(S_cuddly);
            break;
          }

          if (!buttonPushed){
            delay(2500);
          }

          if (!buttonPushed){ 
            zowi.clearMouth();
            delay(100);
          }  

          if (!buttonPushed){
            zowi.putMouth(interrogation);                     
            zowi.sing(S_happy_short);  
          }

          if (!buttonPushed){
            zowi.clearMouth();
            delay(200); 
            zowi.putMouth(interrogation);
          } 
        }
        break;

     
      case 4:

        SCmd.readSerial();
        // //If Zowi is moving yet
        // if (zowi.getRestState()==false){  
        //   move(moveId);
        // } 
      
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

//Adivinawi gesture with interruptions
void ZowiMagics(int repetitions){

        //Initial note frecuency = 400
        //Final note frecuency = 1000
        
        // Reproduce the animation four times
        for(int i = 0; i<repetitions; i++){ 

          int noteM = 400; 
          if(buttonPushed){break;}

          for(int index = 0; index<6; index++){
            if(buttonPushed){break;}
            zowi.putAnimationMouth(adivinawi,index);
            zowi.bendTones(noteM, noteM+100, 1.04, 10, 10);    //400 -> 1000 
            noteM+=100;
          }

          if(!buttonPushed){
            zowi.clearMouth();
            zowi.bendTones(noteM-100, noteM+100, 1.04, 10, 10);  //900 -> 1100
          }

          for(int index = 0; index<6; index++){
            if(buttonPushed){break;}
            zowi.putAnimationMouth(adivinawi,index);
            zowi.bendTones(noteM, noteM+100, 1.04, 10, 10);    //1000 -> 400 
            noteM-=100;
          }
        } 
 
        if(!buttonPushed){
          delay(300);
          zowi.putMouth(happyOpen);
        }
        
}


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