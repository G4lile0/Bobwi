/*
	Configuración de modulo Bluetooth HC-05 para funcionar con la aplicacion de Zowi
	
	Based on the sketch from giltesa.com
	http://giltesa.com/?p=11738
	and the sketch an info from 
	http://www.techbitar.com/modify-the-hc-05-bluetooth-module-defaults-using-at-commands.html
	Licencia:		 (CC) BY-NC-SA: giltesa.com

	El siguiente código permite configurar el modulo Bluetooth HC-05 desde Arduino.
	Se puede ajustar el nombre, la contraseña y los baudios a usar con el puerto serie.
	La configuración Bluetooth se guarda en el modulo, así que solo es necesario configurarlo una vez.
	
	Después de grabar el programa en el Arduino se dispone de 10 segundos de tiempo para conectar el
	modulo, una vez se apague el led comenzara el proceso de configuración y al terminar el led comenzara
	a parpadear.
	
	** La configuración se ha de hacer con el modulo sin emparejar (sin conectar con nada) **


	WIRING

    	HC-05 GND          --- Arduino GND Pin
    	HC-05 VCC (5V)     --- Arduino 5V
    	HC-05 TX           --- Arduino Pin 10 (soft RX)
    	HC-05 RX           --- Arduino Pin11 (soft TX)
    	HC-05 Key (PIN 34) --- Arduino Pin 9


*/



#include <SoftwareSerial.h>

SoftwareSerial BTSerial(10, 11); // RX | TX


void setup()
{

	pinMode(9, OUTPUT);  // this pin will pull the HC-05 pin 34 (key pin) HIGH to switch module to AT mode
  digitalWrite(9, HIGH);
  Serial.begin(9600);
  Serial.println("Enter AT commands:");
  BTSerial.begin(38400);  // HC-05 default speed in AT command more
	
	// Tiempo de espera:
		pinMode(9,OUTPUT);
		digitalWrite(9,HIGH);
		delay(10000);
		digitalWrite(9,LOW);
	
	
	// Ahora se procede a la configuración del modulo:
	
		// Se inicia la configuración:
			Serial.print("AT"); delay(1000);

		// Se ajusta el nombre del Bluetooth:
			Serial.println("AT+NAME=ZOWI"); delay(1000);

		// Se ajustan los baudios:
			Serial.println("AT+UART=115200,1,0"); delay(1000);

		// Se ajusta la contraseña:
			Serial.println("AT+PSWD=2987");  delay(1000);	

}

void loop()
{
	// Al parpadear el led se habrá terminado la configuración:
	digitalWrite(9, !digitalRead(9));
	delay(500);
}
