/******************************************************************************
* Zowi LED Matrix Library
* 
* @version 20150710
* @author Raul de Pablos Martin
*         José Alberca Pita-Romero (Mouth's definitions)
#ifdef __AVR__
 #include <avr/pgmspace.h>
 #define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#elif defined(ESP8266)
 #include <pgmspace.h>
 #define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
 #define pgm_read_word(addr) (*(const unsigned short *)(addr))
 #define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif

******************************************************************************/

#include "LedMatrix.h"
#include <Wire.h>


#include <avr/pgmspace.h>

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

LedMatrix::LedMatrix(void) {
//	delay(300);
	//init matrix
//	initMatrix();
//      Wire.begin();              // I2C init
//      clearMatrix();		// Blank Display
//      cclear();                   // Blank display
//      ledCmd(0x21);              // Turn on oscillator
//      ledCmd(0xE0 | BRIGHTNESS); // Set brightness
//      ledCmd(0x81);              // Display on, no blink

//	memory = 0x00000000;
//	SER = ser_pin;
//	CLK = clk_pin;
//	RCK = rck_pin;
//	pinMode(SER, OUTPUT);
//	pinMode(CLK, OUTPUT);
//	pinMode(RCK, OUTPUT);
//	digitalWrite(SER, LOW);
//	digitalWrite(CLK, LOW);
//	digitalWrite(RCK, LOW);
//	sendMemory();

}

void LedMatrix::writeFull(unsigned long value) {
	memory = value;
	sendMemory();
}

unsigned long LedMatrix::readFull(void) {
	return memory;
}

void LedMatrix::setLed(char row, char column) {
	if(row >= 1 && row <= ROWS && column >= 1 && column <= COLUMNS) {
		memory |= (1L << (MATRIX_LENGTH - (row-1)*COLUMNS - (column)));
		sendMemory();
	}
}

void LedMatrix::unsetLed(char row, char column) {
	if(row >= 1 && row <= ROWS && column >= 1 && column <= COLUMNS) {
		memory &= ~(1L << (MATRIX_LENGTH - (row-1)*COLUMNS - (column)));
		sendMemory();
	}
}

void LedMatrix::clearMatrix(void) {
      Wire.beginTransmission(I2C_ADDR);
      for(uint8_t i=0; i<17; i++) Wire.write(0);
      Wire.endTransmission();
//	memory = 0x00000000;
//	sendMemory();
}


void LedMatrix::ledCmd(uint8_t x) { // Issue command to LED backback driver
      Wire.beginTransmission(I2C_ADDR);
      Wire.write(x);
      Wire.endTransmission();
    }




void LedMatrix::initMatrix(void) {
	//init matrix
      Wire.begin();              // I2C init
      clearMatrix();                   // Blank display
      ledCmd(0x21);              // Turn on oscillator
      ledCmd(0xE0 | BRIGHTNESS); // Set brightness
      ledCmd(0x81);              // Display on, no blink
}


/*

void LedMatrix::cclear(void) { // Clear display buffer
      Wire.beginTransmission(I2C_ADDR);
      for(uint8_t i=0; i<17; i++) Wire.write(0);
      Wire.endTransmission();
    }
*/



void LedMatrix::setEntireMatrix(void) {
	memory = 0x3FFFFFFF;
	sendMemory();
}

void LedMatrix::sendMemory(void) {
          memory = memory <<1;
          Wire.beginTransmission(I2C_ADDR);
          Wire.write(0);
          Wire.write(pgm_read_byte(&reorder[0B00000000]));
          Wire.write(0);
          for(uint8_t j=0; j<5; j++) { 
          Wire.write(pgm_read_byte(&reorder[(memory >> (24-j*6)  ) & 0B01111110]));
          Wire.write(0);
           }
          
          Wire.write(pgm_read_byte(&reorder[0B00000000]));
          Wire.write(0);
          Wire.write(pgm_read_byte(&reorder[0B00000000]));
          Wire.write(0);
          Wire.endTransmission();
}

