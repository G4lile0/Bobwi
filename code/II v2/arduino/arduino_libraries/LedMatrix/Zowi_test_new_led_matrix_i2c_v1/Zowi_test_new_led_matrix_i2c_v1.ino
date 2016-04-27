
// mini codigo para testear la libreria led Matrix
#include <LedMatrix.h>
#include <Wire.h>
//-- Zowi Library
#include <Zowi.h>
Zowi zowi;  //This is Zowi!!
LedMatrix ledmatrix;

void setup() {
   ledmatrix.initMatrix();
   zowi.putMouth(culito);
  // put your setup code here, to run once:
}

void loop() {
  // put your main code here, to run repeatedly:

  for(int i=0; i<2; i++){
      for (int i=0;i<8;i++){
        zowi.putAnimationMouth(littleUuh,i);
        delay(150);
      }
  }
        delay(300);
  
  for(int i=0; i<2; i++){
      for (int i=0;i<4;i++){
        zowi.putAnimationMouth(dreamMouth,i);
        delay(150);
      }
  }
  
        delay(300);
  
  for(int i=0; i<2; i++){
      for (int i=0;i<6;i++){
        zowi.putAnimationMouth(adivinawi,i);
        delay(150);
      }
  }

        delay(300);
  
  for(int i=0; i<2; i++){
      for (int i=0;i<10;i++){
        zowi.putAnimationMouth(wave,i);
        delay(150);
      }
  }
      
      for (int i=0;i<30;i++){
         zowi.putMouth(i);
         delay(400);
      }
      
}
